#include <string.h>
#include <stdio.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"
#include "engine-gst.h"
#include "meego-media-player-control.h"
#include "param-table.h"



static void meego_media_player_control_init (MeegoMediaPlayerControl* iface);

G_DEFINE_TYPE_WITH_CODE (EngineGst, engine_gst, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEEGO_TYPE_MEDIA_PLAYER_CONTROL, meego_media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ENGINE_TYPE_GST, EngineGstPrivate))

#define GET_PRIVATE(o) ((EngineGst *)o)->priv

#define RESET_STR(str) \
     if (str) {       \
       g_free(str);   \
       str = NULL;    \
     }

#define TEARDOWN_ELEMENT(ele)                      \
    if (ele) {                                     \
      gst_element_set_state (ele, GST_STATE_NULL); \
      g_object_unref (ele);                        \
      ele = NULL;                                  \
    }

#define INVALID_PLANE_ID -1

static const gchar *gst_state[] = {
  "GST_STATE_VOID_PENDING",
  "GST_STATE_NULL",
  "GST_STATE_READY",
  "GST_STATE_PAUSED",
  "GST_STATE_PLAYING"
};


/* list of URIs that we consider to be live source. */
static gchar *live_src_uri[] = {"mms://", "mmsh://", "rtsp://",
    "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://",
    NULL
                               };

#define IS_LIVE_URI(uri)                                                           \
({                                                                                 \
  gboolean ret = FALSE;                                                            \
  gchar ** src = live_src_uri;                                                     \
  while (*src) {                                                                   \
    if (!g_ascii_strncasecmp (uri, *src, strlen(*src))) {                          \
      ret = TRUE;                                                                  \
      break;                                                                       \
    }                                                                              \
    src++;                                                                         \
  }                                                                                \
  ret;                                                                             \
})

struct _EngineGstPrivate {
  GstElement *pipeline;

  gchar *uri;
  gint  seekable;

  GstElement *source;

  //buffering stuff
  gboolean buffering;
  gint buffer_percent;

  //UMMS defined player state
  PlayerState player_state;//current state
  PlayerState pending_state;//target state, for async state change(*==>PlayerStatePaused, PlayerStateNull/Stopped==>PlayerStatePlaying)

  gboolean is_live;
  gint64 duration;//ms
  gint64 total_bytes;

  gboolean target_initialized;

  //XWindow target stuff
  gboolean xwin_initialized;
  gint     target_type;
  Window   app_win_id;//top-level window
  Window   video_win_id;
  Display  *disp;
  GThread  *event_thread;
  gboolean event_thread_running;

  //http proxy
  gchar    *proxy_uri;
  gchar    *proxy_id;
  gchar    *proxy_pw;

  //resource management
  UmmsResourceManager *res_mngr;//no need to unref, since it is global singleton.
  GList    *res_list;

  gboolean resource_prepared;
  gboolean uri_parsed;
  GstElement *uri_parse_pipe;
  //flags to indicate resource needed by uri, should be reset before setting a new uri.
  gboolean has_video;
  gint     hw_viddec;//number of HW viddec needed
  gboolean has_audio;
  gint     hw_auddec;//number of HW auddec needed
  gboolean has_sub;//FIXME: what resource needed by subtitle?

  //suspend/restore
  gboolean suspended;//child state of PlayerStateStopped

  //snapshot of suspended execution
  gint64   pos;

  /* Use it to buffer the tag of the stream. */
  GstTagList *tag_list;
  gchar *title;
  gchar *artist;
};

static gboolean _query_buffering_percent (GstElement *pipe, gdouble *percent);
static void _source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data);
static gboolean _stop_pipe (MeegoMediaPlayerControl *control);
static gboolean engine_gst_set_video_size (MeegoMediaPlayerControl *self, guint x, guint y, guint w, guint h);
static gboolean create_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean destroy_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean parse_uri_async (MeegoMediaPlayerControl *self, gchar *uri);
static void release_resource (MeegoMediaPlayerControl *self);

static gboolean
engine_gst_set_uri (MeegoMediaPlayerControl *self,
                    const gchar           *uri)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstState state, pending;
  GstStateChangeReturn ret;

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("SetUri called: old = %s new = %s",
              priv->uri,
              uri);

  if (priv->uri) {
    _stop_pipe(self);
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);
  g_object_set (priv->pipeline, "uri", uri, NULL);

  //reset flags and object specific to uri.
  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  TEARDOWN_ELEMENT (priv->source);

  priv->total_bytes = -1;
  priv->duration = -1;
  priv->seekable = -1;
  priv->buffering = FALSE;
  priv->buffer_percent = -1;
  priv->player_state = PlayerStateStopped;
  priv->pending_state = PlayerStateNull;

  priv->resource_prepared = FALSE;
  priv->uri_parsed = FALSE;
  priv->has_video = FALSE;
  priv->hw_viddec = 0;
  priv->has_audio = FALSE;
  priv->hw_auddec = FALSE;

  if (priv->tag_list)
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;
  RESET_STR(priv->title);
  RESET_STR(priv->artist);

  //URI is one of metadatas whose change should be notified.
  meego_media_player_control_emit_metadata_changed (self);
  return parse_uri_async(self, priv->uri);
}

static gboolean
get_video_rectangle (MeegoMediaPlayerControl *self, gint *ax, gint *ay, gint *w, gint *h, gint *rx, gint *ry)
{
  XWindowAttributes video_win_attr, app_win_attr;
  gint app_x, app_y;
  Window junkwin;
  Status status;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  status = XGetWindowAttributes (priv->disp, priv->app_win_id, &app_win_attr);
  if (!status) {
    UMMS_DEBUG ("Get top-level window attributes failed");
    return FALSE;
  }
  status = XGetWindowAttributes (priv->disp, priv->video_win_id, &video_win_attr);
  if (!status) {
    UMMS_DEBUG ("Get top-level window attributes failed");
    return FALSE;
  }

  (void) XTranslateCoordinates (priv->disp, priv->app_win_id, app_win_attr.root,
      -app_win_attr.border_width,
      -app_win_attr.border_width,
      &app_x, &app_y, &junkwin);
  UMMS_DEBUG ("app window app_absolute_x = %d, app_absolute_y = %d", app_x, app_y);

  (void) XTranslateCoordinates (priv->disp, priv->video_win_id, video_win_attr.root,
      -video_win_attr.border_width,
      -video_win_attr.border_width,
      ax, ay, &junkwin);

  UMMS_DEBUG ("video window video_absolute_x = %d, video_absolute_y = %d", *ax, *ay);

  *rx = *ax - app_x;
  *ry = *ay - app_y;
  *w = video_win_attr.width;
  *h = video_win_attr.height;
  UMMS_DEBUG ("video window video_relative_x = %d, video_relative_y = %d", *rx, *ry);
  UMMS_DEBUG ("video w=%d,y=%d", *w, *h);
  return TRUE;
}

static gboolean
cutout (MeegoMediaPlayerControl *self, gint x, gint y, gint w, gint h)
{
  Atom property;
  gchar data[256];
  gint status;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  property = XInternAtom (priv->disp, "_MUTTER_HINTS", 0);
  if (!property) {
    UMMS_DEBUG ("XInternAtom failed");
    return FALSE;
  }

  g_sprintf (data, "meego-tv-cutout-x=%d:meego-tv-cutout-y=%d:meego-tv-cutout-width=%d:"
             "meego-tv-cutout-height=%d:meego-tv-half-trans=0:meego-tv-full-window=0", x, y, w, h);

  UMMS_DEBUG ("Hints to mtv-mutter = \"%s\"", data);

  XChangeProperty(priv->disp, priv->app_win_id, property, XA_STRING, 8, PropModeReplace,
                  (unsigned char *)data, strlen(data));
  return TRUE;
}


static Window
get_app_win (MeegoMediaPlayerControl *self, Window win)
{

  Window root_win, parent_win, grandparent_win;
  unsigned int num_children;
  Window *child_list;

  EngineGstPrivate *priv = GET_PRIVATE (self);
  Display *dpy = priv->disp;

  if (!XQueryTree(dpy, win, &root_win, &parent_win, &child_list,
                  &num_children)) {
    UMMS_DEBUG("Can't query window(%lx)'s parent.", win);
    return 0;
  }
  UMMS_DEBUG("root=(%lx),window(%lx)'s parent = (%lx)", root_win, win, parent_win);
  if (child_list) XFree((gchar *)child_list);
  if (root_win == parent_win) {
    UMMS_DEBUG("Parent is root, so we got the app window(%lx)", win);
    return win;
  }

  if (!XQueryTree(dpy, parent_win, &root_win, &grandparent_win, &child_list,
                  &num_children)) {
    UMMS_DEBUG("Can't query window(%lx)'s grandparent.", win);
    return 0;
  }
  if (child_list) XFree((gchar *)child_list);
  UMMS_DEBUG("root=(%lx),window(%lx)'s grandparent = (%lx)", root_win, win, grandparent_win);

  if (grandparent_win == root_win) {
    UMMS_DEBUG("Grandpa is root, so we got the app window(%lx)", win);
    return win;
  } else {
    return get_app_win (self, parent_win);
  }
}

static gboolean
unset_xwindow_target (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->xwin_initialized)
    return TRUE;

  destroy_xevent_handle_thread (self);
  priv->xwin_initialized = FALSE;
  return TRUE;
}

static void
engine_gst_handle_xevents (MeegoMediaPlayerControl *control)
{
  XEvent e;
  gint x, y, w, h, rx, ry;
  EngineGstPrivate *priv = GET_PRIVATE (control);

  g_return_if_fail (control);

  /* Handle Expose */
  while (XCheckWindowEvent (priv->disp,
         priv->app_win_id, StructureNotifyMask, &e)) {
    switch (e.type) {
      case ConfigureNotify:
        get_video_rectangle (control, &x, &y, &w, &h, &rx, &ry);
        cutout (control, rx, ry, w, h);
        engine_gst_set_video_size (control, x, y, w, h);
        UMMS_DEBUG ("Got ConfigureNotify, video window abs_x=%d,abs_y=%d,w=%d,y=%d", x, y, w, h);
        break;
      default:
        break;
    }
  }
}

static gpointer
engine_gst_event_thread (MeegoMediaPlayerControl* control)
{

  EngineGstPrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ("Begin");
  while (priv->event_thread_running) {

    if (priv->app_win_id) {
      engine_gst_handle_xevents (control);
    }
    /* FIXME: do we want to align this with the framerate or anything else? */
    g_usleep (G_USEC_PER_SEC / 20);

  }

  UMMS_DEBUG ("End");
  return NULL;
}

static gboolean
create_xevent_handle_thread (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (!priv->event_thread) {
    /* Setup our event listening thread */
    UMMS_DEBUG ("run xevent thread");
    priv->event_thread_running = TRUE;
    priv->event_thread = g_thread_create (
          (GThreadFunc) engine_gst_event_thread, self, TRUE, NULL);
  }
  return TRUE;
}

static gboolean
destroy_xevent_handle_thread (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (priv->event_thread) {
    priv->event_thread_running = FALSE;
    g_thread_join (priv->event_thread);
    priv->event_thread = NULL;
  }
  UMMS_DEBUG ("End");
  return TRUE;
}

static gboolean setup_ismd_vbin(MeegoMediaPlayerControl *self, gchar *rect, gint plane)
{
  GstElement *vsink;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  vsink = gst_element_factory_make ("ismd_vidrend_bin", NULL);
  if (!vsink) {
    UMMS_DEBUG ("Failed to make ismd_vidrend_bin");
    return FALSE;
  }

  if (rect)
    g_object_set (vsink, "rectangle", rect, NULL);

  if (plane != INVALID_PLANE_ID)
    g_object_set (vsink, "gdl-plane", plane, NULL);

  g_object_set (priv->pipeline, "video-sink", vsink, NULL);

  UMMS_DEBUG ("Set ismd_vidrend_bin to playbin2");
  return TRUE;
}


static gboolean setup_gdl_plane_target (MeegoMediaPlayerControl *self, GHashTable *params)
{
  gchar *rect = NULL;
  gint  plane = INVALID_PLANE_ID;
  GValue *val = NULL;

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
  if (val) {
    rect = (gchar *)g_value_get_string (val);
    UMMS_DEBUG ("rectangle = '%s'", rect);
  }

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_PlANE_ID);
  if (val) {
    plane = g_value_get_int (val);
    UMMS_DEBUG ("gdl plane = '%d'", plane);
  }

  return setup_ismd_vbin (self, rect, plane);
}

/*
 * To get the top level window according to subwindow.
 * Some prerequisites should be satisfied:
 * 1. The X server uses 29 bits(i.e. bit1--bit29) to represents a window id.
 * 2. The X server uses bit22--bit29 to represents a connection/client, it means X server supports 256 connections/clients at maximum.
 * 3. The top-level window and subwindow is requested from the same connection.
 * 1 and 2 are the case of xorg server.
 * 3 is always satisfied, unless subwindow is requested from a process which is not the same process hosting the top-level window.
*/
static Window get_top_level_win (MeegoMediaPlayerControl *self, Window sub_win)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  Display *disp = priv->disp;
  Window root_win, parent_win, cur_win;
  unsigned int num_children;
  Window *child_list = NULL;
  Window top_win = 0;
  gboolean done = FALSE;

  if (!disp)
    return 0;

  printf ("============%s:Begin, sub_win=%lx ==========\n", __FUNCTION__, sub_win);
#define CLIENT_MASK (0x1fe00000)
#define FROM_THE_SAME_PROC(w1,w2) ((w1&CLIENT_MASK) == (w2&CLIENT_MASK))

  cur_win = sub_win;
  while (!done) {
    if (!XQueryTree(disp, cur_win, &root_win, &parent_win, &child_list, &num_children)) {
      UMMS_DEBUG ("Can't query window tree.");
      return 0;
    }

    UMMS_DEBUG ("cur_win(%lx), parent_win(%lx)", cur_win, parent_win);
    if (child_list) XFree((char *)child_list);

    if (!FROM_THE_SAME_PROC(cur_win, parent_win)) {
      UMMS_DEBUG ("Got the top-level window(%lx)", cur_win);
      top_win = cur_win;
      done = TRUE;
      break;
    };
    cur_win = parent_win;
  }
  printf ("============%s:End==========\n", __FUNCTION__);
  return top_win;
}

static gboolean setup_datacopy_target (MeegoMediaPlayerControl *self, GHashTable *params)
{
  GstElement *shmvbin = NULL;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  shmvbin = gst_element_factory_make ("shmvidrendbin", NULL);
  if (!shmvbin) {
    UMMS_DEBUG ("Making \"shmvidrendbin\" failed");
    return FALSE;
  }

  g_object_set (priv->pipeline, "video-sink", shmvbin, NULL);
  UMMS_DEBUG ("Set \"shmvidrendbin\" to playbin2");
  return TRUE;
}

static int x_print_error(
  Display *dpy,
  XErrorEvent *event,
  FILE *fp)
{
  char buffer[BUFSIZ];
  char mesg[BUFSIZ];
  char number[32];
  const char *mtype = "XlibMessage";
  XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
  XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
  (void) fprintf(fp, "%s:  %s\n  ", mesg, buffer);
  XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
      mesg, BUFSIZ);
  (void) fprintf(fp, mesg, event->request_code);
  if (event->request_code < 128) {
    sprintf(number, "%d", event->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
  }

  (void) fprintf(fp, " (%s)\n", buffer);
  if (event->request_code >= 128) {
    XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
        mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->minor_code);

    fputs("\n", fp);
  }
  if (event->error_code >= 128) {
    strcpy(buffer, "Value");
    XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
    if (mesg[0]) {
      fputs("  ", fp);
      (void) fprintf(fp, mesg, event->resourceid);
      fputs("\n", fp);
    }

  } else if ((event->error_code == BadWindow) ||
             (event->error_code == BadPixmap) ||
             (event->error_code == BadCursor) ||
             (event->error_code == BadFont) ||
             (event->error_code == BadDrawable) ||
             (event->error_code == BadColor) ||
             (event->error_code == BadGC) ||
             (event->error_code == BadIDChoice) ||
             (event->error_code == BadValue) ||
             (event->error_code == BadAtom)) {
    if (event->error_code == BadValue)
      XGetErrorDatabaseText(dpy, mtype, "Value", "Value 0x%x",
          mesg, BUFSIZ);
    else if (event->error_code == BadAtom)
      XGetErrorDatabaseText(dpy, mtype, "AtomID", "AtomID 0x%x",
          mesg, BUFSIZ);
    else
      XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
          mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->resourceid);
    fputs("\n", fp);
  }
  XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d",
      mesg, BUFSIZ);
  fputs("  ", fp);
  (void) fprintf(fp, mesg, event->serial);
  XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
      mesg, BUFSIZ);
  fputs("\n  ", fp);
  if (event->error_code == BadImplementation) return 0;
  return 1;
}

//Mostly the same as default handler, but not exit process.
int xerror_handler (
  Display *dpy,
  XErrorEvent *event)
{
  return x_print_error (dpy, event, stderr);
}


/*
 * 1. Calculate top-level window according to video window.
 * 2. Cutout video window geometry according to its relative position to top-level window.
 * 3. Setup underlying ismd_vidrend_bin element.
 * 4. Create xevent handle thread.
 */

static gboolean setup_xwindow_target (MeegoMediaPlayerControl *self, GHashTable *params)
{
  GValue *val;
  gint x, y, w, h, rx, ry;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (priv->xwin_initialized) {
    unset_xwindow_target (self);
  }

  if (!priv->disp)
    priv->disp = XOpenDisplay (NULL);

  if (!priv->disp) {
    UMMS_DEBUG ("Could not open display");
    return FALSE;
  }

  XSetErrorHandler (xerror_handler);

  val = g_hash_table_lookup (params, "window-id");
  if (!val) {
    UMMS_DEBUG ("no window-id");
    return FALSE;
  }
  priv->video_win_id = (Window)g_value_get_int (val);
  priv->app_win_id = get_top_level_win (self, priv->video_win_id);

  if (!priv->app_win_id) {
    UMMS_DEBUG ("Get top-level window failed");
    return FALSE;
  }

  get_video_rectangle (self, &x, &y, &w, &h, &rx, &ry);
  cutout (self, rx, ry, w, h);

  //make ismd video sink and set its rectangle property
  gchar *rectangle_des = NULL;
  rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);

  UMMS_DEBUG ("set rectangle damension :'%s'", rectangle_des);
  setup_ismd_vbin (self, rectangle_des, INVALID_PLANE_ID);
  g_free (rectangle_des);

  //Monitor top-level window's structure change event.
  XSelectInput (priv->disp, priv->app_win_id,
                StructureNotifyMask);
  create_xevent_handle_thread (self);

  priv->target_type = XWindow;
  priv->xwin_initialized = TRUE;

  return TRUE;
}

static gboolean
unset_target (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->target_initialized)
    return TRUE;

  /*
   *For DataCopy and ReservedType0, gstreamer will handle the video-sink element elegantly.
   *Nothing need to do here.
   */
  switch (priv->target_type) {
    case XWindow:
      unset_xwindow_target (self);
      break;
    case DataCopy:
      break;
    case Socket:
      g_assert_not_reached ();
      break;
    case ReservedType0:
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  priv->target_type = TargetTypeInvalid;
  priv->target_initialized = FALSE;
  return TRUE;
}

static gboolean
engine_gst_set_target (MeegoMediaPlayerControl *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  /*
   * Set target can only happen at Null or Stopped. Two reasons:
   * 1. Gstreamer don't support switching sink on the fly.
   * 2. PlayerStateNull/PlayerStateStopped means all target related resources have been released.
   *    It is more convenience for resource management implementing.
   */
  if (priv->player_state != PlayerStateNull && priv->player_state != PlayerStateStopped) {
    UMMS_DEBUG ("Ignored, can only set target at PlayerStateNull or PlayerStateStopped");
    return FALSE;
  }

  if (!priv->pipeline) {
    UMMS_DEBUG ("Engine not loaded, reason may be SetUri not invoked");
    return FALSE;
  }

  if (priv->target_initialized) {
    unset_target (self);
  }

  switch (type) {
    case XWindow:
      ret = setup_xwindow_target (self, params);
      break;
    case DataCopy:
      setup_datacopy_target(self, params);
      break;
    case Socket:
      UMMS_DEBUG ("unsupported target type: Socket");
      ret = FALSE;
      break;
    case ReservedType0:
      /*
       * ReservedType0 represents the gdl-plane video output in our UMMS implementation.
       */
      ret = setup_gdl_plane_target (self, params);
      break;
    default:
      break;
  }

  if (ret) {
    priv->target_type = type;
    priv->target_initialized = TRUE;
  }

  return ret;
}

static gboolean
prepare_plane (MeegoMediaPlayerControl *self)
{
  GstElement *vsink_bin;
  gint plane;
  gboolean ret = TRUE;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_object_get (G_OBJECT(priv->pipeline), "video-sink", &vsink_bin, NULL);

  if (vsink_bin) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "gdl-plane") == NULL) {
      UMMS_DEBUG ("vsink has no gdl-plane property, which means target type is not XWindow or ReservedType0");
      return ret;
    }

    g_object_get (G_OBJECT(vsink_bin), "gdl-plane", &plane, NULL);

    //request plane resource
    ResourceRequest req = {ResourceTypePlane, plane};
    Resource *res = NULL;

    res = umms_resource_manager_request_resource (priv->res_mngr, &req);

    if (!res) {
      UMMS_DEBUG ("Failed");
      ret = FALSE;
    } else if (plane != res->handle) {
      g_object_set (G_OBJECT(vsink_bin), "gdl-plane", res->handle, NULL);
      UMMS_DEBUG ("Plane changed '%d'==>'%d'", plane,  res->handle);
    } else {
      //Do nothing;
    }

    priv->res_list = g_list_append (priv->res_list, res);
    gst_object_unref (vsink_bin);
  }

  return ret;
}

//both Xwindow and ReservedType0 target use ismd_vidrend_sink, so that need clock.
#define NEED_CLOCK(target_type) \
  ((target_type == XWindow || target_type == ReservedType0)?TRUE:FALSE)

#define REQUEST_RES(self, t, p, e_msg)                                        \
  do{                                                                         \
    EngineGstPrivate *priv = GET_PRIVATE (self);                              \
    ResourceRequest req = {0,};                                               \
    Resource *res = NULL;                                                     \
    req.type = t;                                                             \
    req.preference = p;                                                       \
    res = umms_resource_manager_request_resource (priv->res_mngr, &req);      \
    if (!res) {                                                               \
      release_resource(self);                                                 \
      meego_media_player_control_emit_error (self,                            \
                                             UMMS_RESOURCE_ERROR_NO_RESOURCE, \
                                             e_msg);                          \
      return FALSE;                                                           \
    }                                                                         \
    priv->res_list = g_list_append (priv->res_list, res);                     \
    }while(0)


static gboolean
request_resource (MeegoMediaPlayerControl *self)
{
  gint i;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv->uri_parsed, FALSE);//uri should already be parsed.

  if (priv->resource_prepared)
    return TRUE;

  /*
   * For live source, we create and set clock to pipeline manually,
   * ismd audio/video sink will not create and provide a clock.
   * So, just need one clock.
   */
  if (priv->is_live) {
    REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
  } else {
    if (priv->has_video && NEED_CLOCK(priv->target_type)) {
      REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
    }
    if (priv->has_audio) {
      REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
    }
  }

  for (i = 0; i < priv->hw_viddec; i++) {
    REQUEST_RES(self, ResourceTypeHwViddec, INVALID_RES_HANDLE, "No HW video decoder resource");
  }

  /*
  for (i=0; i<priv->hw_auddec; i++) {
    REQUEST_RES(self, ResourceTypeHwAuddec, INVALID_RES_HANDLE, "No HW video decoder resource");
  }
  */
  priv->resource_prepared = TRUE;
  return TRUE;
}

static void
release_resource (MeegoMediaPlayerControl *self)
{
  GList *g;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  for (g = priv->res_list; g; g = g->next) {
    Resource *res = (Resource *) (g->data);
    umms_resource_manager_release_resource (priv->res_mngr, res);
  }
  g_list_free (priv->res_list);
  priv->res_list = NULL;
  priv->resource_prepared = FALSE;

  return;
}

//Unref returned clock after usage
static GstClock *
get_hw_clock(void)
{
  GstClock* clock = NULL;
  GstElement *vsink = NULL;

  UMMS_DEBUG ("Begin");
  vsink = gst_element_factory_make ("ismd_vidrend_sink", NULL);
  if (!vsink)
    return NULL;

  clock = gst_element_provide_clock (vsink);

  if (clock)
    g_object_ref (clock);

  g_object_unref (vsink);
  UMMS_DEBUG ("End");

  return clock;
}

static gboolean
activate_engine (MeegoMediaPlayerControl *self, GstState target_state)
{
  gboolean ret;
  PlayerState old_pending;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv->uri, FALSE);
  g_return_val_if_fail (target_state == GST_STATE_PAUSED || target_state == GST_STATE_PLAYING, FALSE);

  old_pending = priv->pending_state;
  priv->pending_state = ((target_state == GST_STATE_PAUSED) ? PlayerStatePaused : PlayerStatePlaying);

  if (ret = request_resource(self)) {
    if (target_state == GST_STATE_PLAYING) {
      if (IS_LIVE_URI(priv->uri)) {
        //    ISmdGstClock *ismd_clock = NULL;
        /* For the special case of live source.
           Becasue our hardware decoder and sink need the special clock type and if the clock type is wrong,
           the hardware can not work well.  In file case, the provide_clock will be called after all the elements
           have been made and added in pause state, so the element which represent the hardware will provide the
           right clock. But in live source case, the state change from NULL to playing is continous and the provide_clock
           function may be called before hardware element has been made.
           So we need to set the clock of pipeline statically here.*/
        //    ismd_clock = g_object_new (ISMD_GST_TYPE_CLOCK, NULL);
        //    gst_pipeline_use_clock (GST_PIPELINE_CAST(priv->pipeline), GST_CLOCK_CAST(ismd_clock));
        GstClock *clock = get_hw_clock ();
        if (clock) {
          gst_pipeline_use_clock (GST_PIPELINE_CAST(priv->pipeline), clock);
          g_object_unref (clock);
        } else {
          UMMS_DEBUG ("Can't get HW clock");
          meego_media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, "Can't get HW clock for live source");
          ret = FALSE;
          goto OUT;
        }

      }
    }

    if (gst_element_set_state(priv->pipeline, target_state) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("set pipeline to %d failed", target_state);
      ret = FALSE;
      goto OUT;
    }
  }

OUT:
  if (!ret) {
    priv->pending_state = old_pending;
  }

  return ret;
}

static gboolean
engine_gst_pause (MeegoMediaPlayerControl *self)
{
  return activate_engine (self, GST_STATE_PAUSED);
}

static gboolean
engine_gst_play (MeegoMediaPlayerControl *self)
{
  return activate_engine (self, GST_STATE_PLAYING);
}

static gboolean
_stop_pipe (MeegoMediaPlayerControl *control)
{
  EngineGstPrivate *priv = GET_PRIVATE (control);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Unable to set NULL state");
    return FALSE;
  }


  release_resource (control);

  UMMS_DEBUG ("gstreamer engine stopped");
  return TRUE;
}

static gboolean
engine_gst_stop (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  PlayerState old_state;

  if (!_stop_pipe (self))
    return FALSE;

  old_state = priv->player_state;
  priv->player_state = PlayerStateStopped;
  priv->suspended = FALSE;

  if (old_state != priv->player_state) {
    meego_media_player_control_emit_player_state_changed (self, old_state, priv->player_state);
  }
  meego_media_player_control_emit_stopped (self);

  return TRUE;
}

static gint
_is_ismd_vidrend_bin (GstElement * element, gpointer user_data)
{
  gint ret = 1; //0: match , non-zero: dismatch
  gchar *ele_name = NULL;

  if (!GST_IS_BIN(element)) {
    goto unref_ele;
  }

  ele_name = gst_element_get_name (element);
  UMMS_DEBUG ("element name='%s'", ele_name);

  //ugly solution, check by element metadata will be better
  ele_name = gst_element_get_name (element);
  if (g_strrstr(ele_name, "ismdgstvidrendbin") != NULL || g_strrstr(ele_name, "ismd_vidrend_bin") != NULL)
    ret = 0;

  g_free (ele_name);
  if (ret) {//dismatch
    goto unref_ele;
  }

  return ret;

unref_ele:
  gst_object_unref (element);
  return ret;
}

/*
 * This function is used to judge whether we are using ismd_vidrend_bin
 * which allow video rectangle and plane settings through properties.
 * Deprecated.
 */
static GstElement *
_get_ismd_vidrend_bin (GstBin *bin)
{
  GstIterator *children;
  gpointer result;

  g_return_val_if_fail (GST_IS_BIN (bin), NULL);

  GST_INFO ("[%s]: looking up ismd_vidrend_bin  element",
            GST_ELEMENT_NAME (bin));

  children = gst_bin_iterate_recurse (bin);
  result = gst_iterator_find_custom (children,
           (GCompareFunc)_is_ismd_vidrend_bin, (gpointer) NULL);
  gst_iterator_free (children);

  if (result) {
    GST_INFO ("found ismd_vidrend_bin." );
  }

  return GST_ELEMENT_CAST (result);
}

static gboolean
engine_gst_set_video_size (MeegoMediaPlayerControl *self,
    guint x, guint y, guint w, guint h)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");

  //We use ismd_vidrend_bin as video-sink, so we can set rectangle property.
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;

    rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("set rectangle damension :'%s'", rectangle_des);
    g_object_set (G_OBJECT(vsink_bin), "rectangle", rectangle_des, NULL);
    g_free (rectangle_des);
    gst_object_unref (vsink_bin);
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    ret = FALSE;
  }

  return ret;
}

static gboolean
engine_gst_get_video_size (MeegoMediaPlayerControl *self,
    guint *w, guint *h)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  guint x[1], y[1];
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;
    g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
    sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("got rectangle damension :'%u,%u,%u,%u'", *x, *y, *w, *h);
    gst_object_unref (vsink_bin);
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    ret = FALSE;
  }
  return ret;
}

static gboolean
engine_gst_is_seekable (MeegoMediaPlayerControl *self, gboolean *seekable)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gboolean res = FALSE;
  gint old_seekable;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (seekable != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);


  if (priv->uri == NULL)
    return FALSE;

  old_seekable = priv->seekable;

  if (priv->seekable == -1) {
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_TIME);
    if (gst_element_query (pipe, query)) {
      gst_query_parse_seeking (query, NULL, &res, NULL, NULL);
      UMMS_DEBUG ("seeking query says the stream is%s seekable", (res) ? "" : " not");
      priv->seekable = (res) ? 1 : 0;
    } else {
      UMMS_DEBUG ("seeking query failed, set seekable according to is_live flag");
      priv->seekable = priv->is_live ? FALSE : TRUE;
    }
    gst_query_unref (query);
  }

  if (priv->seekable != -1) {
    *seekable = (priv->seekable != 0);
  }

  UMMS_DEBUG ("stream is%s seekable", (*seekable) ? "" : " not");
  return TRUE;
}

static gboolean
engine_gst_set_position (MeegoMediaPlayerControl *self, gint64 in_pos)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gboolean ret;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);


  ret = gst_element_seek (pipe, 1.0,
        GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, in_pos * GST_MSECOND,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
  UMMS_DEBUG ("Seeking to %" GST_TIME_FORMAT " %s", GST_TIME_ARGS (in_pos * GST_MSECOND), ret ? "succeeded" : "failed");
  if (ret)
    meego_media_player_control_emit_seeked (self);
  return ret;
}

static gboolean
engine_gst_get_position (MeegoMediaPlayerControl *self, gint64 *cur_pos)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  GstFormat fmt;
  gint64 cur = 0;
  gboolean ret = TRUE;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (cur_pos != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  fmt = GST_FORMAT_TIME;
  if (gst_element_query_position (pipe, &fmt, &cur)) {
    UMMS_DEBUG ("current position = %lld (ms)", *cur_pos = cur / GST_MSECOND);
  } else {
    UMMS_DEBUG ("Failed to query position");
    ret = FALSE;
  }
  return ret;
}

static gboolean
engine_gst_set_playback_rate (MeegoMediaPlayerControl *self, gdouble in_rate)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstFormat fmt;
  gint64 cur;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("set playback rate to %f ", in_rate);
  return gst_element_seek (pipe, in_rate,
         GST_FORMAT_TIME, GST_SEEK_FLAG_NONE,
         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

static gboolean
engine_gst_get_playback_rate (MeegoMediaPlayerControl *self, gdouble *out_rate)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstQuery *query;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");
  query = gst_query_new_segment (GST_FORMAT_TIME);

  if (gst_element_query (pipe, query)) {
    gst_query_parse_segment (query, out_rate, NULL, NULL, NULL);
    UMMS_DEBUG ("current rate = %f", *out_rate);
  } else {
    UMMS_DEBUG ("Failed to query segment");
  }
  gst_query_unref (query);

  return TRUE;
}

static gboolean
engine_gst_set_volume (MeegoMediaPlayerControl *self, gint vol)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  volume = CLAMP ((((gdouble)vol) / 100), 0.0, 1.0);
  UMMS_DEBUG ("set volume to = %f", volume);
  gst_stream_volume_set_volume (GST_STREAM_VOLUME (pipe),
      GST_STREAM_VOLUME_FORMAT_CUBIC,
      volume);

  return TRUE;
}

static gboolean
engine_gst_get_volume (MeegoMediaPlayerControl *self, gint *volume)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gdouble vol;

  *volume = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipe),
        GST_STREAM_VOLUME_FORMAT_CUBIC);

  *volume = vol * 100;
  UMMS_DEBUG ("cur volume=%f(double), %d(int)", vol, *volume);

  return TRUE;
}

static gboolean
engine_gst_get_media_size_time (MeegoMediaPlayerControl *self, gint64 *media_size_time)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint64 duration;
  gboolean ret;
  GstFormat fmt = GST_FORMAT_TIME;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  if (gst_element_query_duration (pipe, &fmt, &duration)) {
    *media_size_time = duration / GST_MSECOND;
    UMMS_DEBUG ("media size = %lld (ms)", *media_size_time);
  } else {
    UMMS_DEBUG ("query media_size_time failed");
    *media_size_time = -1;
  }

  return ret;
}


static gboolean
engine_gst_get_media_size_bytes (MeegoMediaPlayerControl *self, gint64 *media_size_bytes)
{
  GstElement *source;
  EngineGstPrivate *priv;
  gint64 length = 0;
  gboolean ret;
  GstFormat fmt = GST_FORMAT_BYTES;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  source = priv->source;
  g_return_val_if_fail (GST_IS_ELEMENT (source), FALSE);

  UMMS_DEBUG ("invoked");

  if (!gst_element_query_duration (source, &fmt, &length)) {

    // Fall back to querying the source pads manually.
    // See also https://bugzilla.gnome.org/show_bug.cgi?id=638749
    GstIterator* iter = gst_element_iterate_src_pads(source);
    gboolean done = FALSE;
    length = 0;
    while (!done) {
      gpointer data;

      switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
          GstPad* pad = GST_PAD_CAST(data);
          gint64 padLength = 0;
          if (gst_pad_query_duration(pad, &fmt, &padLength)
               && padLength > length)
            length = padLength;
          gst_object_unref(pad);
          break;
        }
        case GST_ITERATOR_RESYNC:
          gst_iterator_resync(iter);
          break;
        case GST_ITERATOR_ERROR:
          // Fall through.
        case GST_ITERATOR_DONE:
          done = TRUE;
          break;
      }
    }
    gst_iterator_free(iter);
  }

  *media_size_bytes = length;
  UMMS_DEBUG ("Total bytes = %lld", length);

  return TRUE;
}

static gboolean
engine_gst_has_video (MeegoMediaPlayerControl *self, gboolean *has_video)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint n_video;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("'%d' videos in stream", n_video);
  *has_video = (n_video > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
engine_gst_has_audio (MeegoMediaPlayerControl *self, gboolean *has_audio)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint n_audio;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("'%d' audio tracks in stream", n_audio);
  *has_audio = (n_audio > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
engine_gst_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
  return TRUE;
}

static gboolean
engine_gst_is_streaming (MeegoMediaPlayerControl *self, gboolean *is_streaming)
{
  GstElement *pipe;
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_return_val_if_fail (priv->uri, FALSE);
  /*For now, we consider live source to be streaming source , hence unseekable.*/
  *is_streaming = priv->is_live;
  UMMS_DEBUG ("uri:'%s' is %s streaming source", priv->uri, (*is_streaming) ? "" : "not");
  return TRUE;
}

static gboolean
engine_gst_get_player_state (MeegoMediaPlayerControl *self,
    gint *state)
{
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  *state = priv->player_state;
  return TRUE;
}

static gboolean
engine_gst_get_buffered_bytes (MeegoMediaPlayerControl *self,
    gint64 *buffered_bytes)
{
  gint64 total_bytes;
  gdouble percent;

  g_return_val_if_fail (self != NULL, FALSE);
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->total_bytes == -1) {
    engine_gst_get_media_size_bytes (self, &total_bytes);
    priv->total_bytes = total_bytes;
  }

  *buffered_bytes = (priv->total_bytes * percent) / 100;

  UMMS_DEBUG ("Buffered bytes = %lld", *buffered_bytes);

  return TRUE;
}

static gboolean
_query_buffering_percent (GstElement *pipe, gdouble *percent)
{
  GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

  if (!gst_element_query(pipe, query)) {
    gst_query_unref(query);
    UMMS_DEBUG ("failed");
    return FALSE;
  }

  gint64 start, stop;
  gdouble fillStatus = 100.0;

  gst_query_parse_buffering_range(query, 0, &start, &stop, 0);
  gst_query_unref(query);

  if (stop != -1) {
    fillStatus = 100.0 * stop / GST_FORMAT_PERCENT_MAX;
  }

  UMMS_DEBUG ("[Buffering] Download buffer filled up to %f%%", fillStatus);
  *percent = fillStatus;

  return TRUE;
}

static gboolean
engine_gst_get_buffered_time (MeegoMediaPlayerControl *self, gint64 *buffered_time)
{
  gint64 duration;
  gdouble percent;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (self != NULL, FALSE);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->duration == -1) {
    engine_gst_get_media_size_time (self, &duration);
    priv->duration = duration;
  }

  *buffered_time = (priv->duration * percent) / 100;
  UMMS_DEBUG ("duration=%lld, buffered_time=%lld", priv->duration, *buffered_time);

  return TRUE;
}

static gboolean
engine_gst_get_current_video (MeegoMediaPlayerControl *self, gint *cur_video)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-video", &c_video, NULL);
  UMMS_DEBUG ("the current video stream is %d", c_video);

  *cur_video = c_video;

  return TRUE;
}


static gboolean
engine_gst_get_current_audio (MeegoMediaPlayerControl *self, gint *cur_audio)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-audio", &c_audio, NULL);
  UMMS_DEBUG ("the current audio stream is %d", c_audio);

  *cur_audio = c_audio;

  return TRUE;
}


static gboolean
engine_gst_set_current_video (MeegoMediaPlayerControl *self, gint cur_video)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_video ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("The total video numeber is %d, we want to set to %d", n_video, cur_video);
  if ((cur_video < 0) || (cur_video >= n_video)) {
    UMMS_DEBUG ("The video we want to set is %d, invalid one.", cur_video);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-video", cur_video, NULL);

  return TRUE;
}


static gboolean
engine_gst_set_current_audio (MeegoMediaPlayerControl *self, gint cur_audio)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_audio ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("The total audio numeber is %d, we want to set to %d", n_audio, cur_audio);
  if ((cur_audio < 0) || (cur_audio >= n_audio)) {
    UMMS_DEBUG ("The audio we want to set is %d, invalid one.", cur_audio);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-audio", cur_audio, NULL);

  return TRUE;
}


static gboolean
engine_gst_get_video_num (MeegoMediaPlayerControl *self, gint *video_num)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("the video number of the stream is %d", n_video);

  *video_num = n_video;

  return TRUE;
}


static gboolean
engine_gst_get_audio_num (MeegoMediaPlayerControl *self, gint *audio_num)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d", n_audio);

  *audio_num = n_audio;

  return TRUE;
}


static gboolean
engine_gst_set_subtitle_uri (MeegoMediaPlayerControl *self, gchar *sub_uri)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  GstElement *sub_sink = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We first set the subtitle render element of playbin2 using ismd_subrend_bin.
     If failed, we just use the default subrender logic in playbin2. */
  sub_sink = gst_element_factory_make ("ismd_subrend_bin", NULL);
  if (sub_sink) {
    UMMS_DEBUG ("succeed to make the ismd_subrend_bin, set it to playbin2");
    g_object_set (priv->pipeline, "text-sink", sub_sink, NULL);
  } else {
    UMMS_DEBUG ("Unable to make the ismd_subrend_bin");
  }

  /* It seems that the subtitle URI need to set before activate the group, and
     can not dynamic change it of current group when playing.
     So calling this API when stream is playing may have no effect. */
  g_object_set (G_OBJECT (pipe), "suburi", sub_uri, NULL);

  return TRUE;
}


static gboolean
engine_gst_get_subtitle_num (MeegoMediaPlayerControl *self, gint *sub_num)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("the subtitle number of the stream is %d", n_sub);

  *sub_num = n_sub;

  return TRUE;
}


static gboolean
engine_gst_get_current_subtitle (MeegoMediaPlayerControl *self, gint *cur_sub)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-text", &c_sub, NULL);
  UMMS_DEBUG ("the current subtitle stream is %d", c_sub);

  *cur_sub = c_sub;

  return TRUE;
}


static gboolean
engine_gst_set_current_subtitle (MeegoMediaPlayerControl *self, gint cur_sub)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_sub ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("The total subtitle numeber is %d, we want to set to %d", n_sub, cur_sub);
  if ((cur_sub < 0) || (cur_sub >= n_sub)) {
    UMMS_DEBUG ("The subtitle we want to set is %d, invalid one.", cur_sub);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-text", cur_sub, NULL);

  return TRUE;
}


static gboolean
engine_gst_set_proxy (MeegoMediaPlayerControl *self, GHashTable *params)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  GValue *val = NULL;

  g_return_val_if_fail ((self != NULL) && (params != NULL), FALSE);
  priv = GET_PRIVATE (self);

  if (g_hash_table_lookup_extended (params, "proxy-uri", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_uri);
    priv->proxy_uri = g_value_dup_string (val);
  }

  if (g_hash_table_lookup_extended (params, "proxy-id", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_id);
    priv->proxy_id = g_value_dup_string (val);
  }

  if (g_hash_table_lookup_extended (params, "proxy-pw", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_pw)
    priv->proxy_pw = g_value_dup_string (val);
  }

  UMMS_DEBUG ("proxy=%s, id=%s, pw=%s", priv->proxy_uri, priv->proxy_id, priv->proxy_pw);
  return TRUE;
}


static gboolean
engine_gst_set_buffer_depth (MeegoMediaPlayerControl *self, gint format, gint64 buf_val)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  if (buf_val < 0) {
    UMMS_DEBUG("The buffer depth value is %lld, invalid one", buf_val);
    return FALSE;
  }

  if (format == BufferFormatByTime) {
    g_object_set (G_OBJECT (pipe), "buffer-duration", buf_val, NULL);
    UMMS_DEBUG("Set the buffer-duration to %lld", buf_val);
  } else if (format == BufferFormatByBytes) {
    g_object_set (G_OBJECT (pipe), "buffer-size", buf_val, NULL);
    UMMS_DEBUG("Set the buffer-size to %lld", buf_val);
  } else {
    UMMS_DEBUG("Pass the wrong format:%d to buffer depth setting", format);
    return FALSE;
  }

  return TRUE;
}


static gboolean
engine_gst_set_mute (MeegoMediaPlayerControl *self, gint mute)
{
  GstElement *pipe;
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("set mute to = %d", mute);
  gst_stream_volume_set_mute (GST_STREAM_VOLUME (pipe), mute);

  return TRUE;
}


static gboolean
engine_gst_is_mute (MeegoMediaPlayerControl *self, gint *mute)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gboolean is_mute;

  *mute = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  is_mute = gst_stream_volume_get_mute (GST_STREAM_VOLUME (pipe));
  UMMS_DEBUG("Get the mute %d", is_mute);
  *mute = is_mute;

  return TRUE;
}


static gboolean
engine_gst_set_scale_mode (MeegoMediaPlayerControl *self, gint scale_mode)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstElement *vsink_bin;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "scale-mode");
    if (pspec == NULL) {
      ret = FALSE;
      UMMS_DEBUG("can not get the scale-mode feature");
      goto OUT;
    }

    g_value_init (&val, pspec->value_type);
    g_object_get_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    eclass = G_ENUM_CLASS (g_type_class_peek (G_VALUE_TYPE (&val)));
    if (eclass == NULL) {
      ret = FALSE;
      goto OUT;
    }

    switch (scale_mode) {
      case ScaleModeNoScale:
        eval = g_enum_get_value_by_nick (eclass, "none");
        break;
      case ScaleModeFill:
        eval = g_enum_get_value_by_nick (eclass, "scale2fit");
        break;
      case ScaleModeKeepAspectRatio:
        eval = g_enum_get_value_by_nick (eclass, "zoom2fit");
        break;
      case ScaleModeFillKeepAspectRatio:
        eval = g_enum_get_value_by_nick (eclass, "zoom2fill");
        break;
      default:
        UMMS_DEBUG("Invalid scale mode: %d", scale_mode);
        ret = FALSE;
        goto OUT;
    }

    if (eval == NULL) {
      ret = FALSE;
      goto OUT;
    }

    g_value_set_enum (&val, eval->value);
    g_object_set_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    g_value_unset (&val);
  } else {
    UMMS_DEBUG("No a vidrend_bin set, scale not support now!");
    ret = FALSE;
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}


static gboolean
engine_gst_get_scale_mode (MeegoMediaPlayerControl *self, gint *scale_mode)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstElement *vsink_bin;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;

  *scale_mode = ScaleModeInvalid;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "scale-mode");
    if (pspec == NULL) {
      ret = FALSE;
      UMMS_DEBUG("can not get the scale-mode feature");
      goto OUT;
    }

    g_value_init (&val, pspec->value_type);
    g_object_get_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    eclass = G_ENUM_CLASS (g_type_class_peek (G_VALUE_TYPE (&val)));
    if (eclass == NULL) {
      ret = FALSE;
      goto OUT;
    }

    eval = g_enum_get_value (eclass, g_value_get_enum(&val));
    if (eval == NULL) {
      ret = FALSE;
      goto OUT;
    }

    if (strcmp(eval->value_nick, "none")) {
      *scale_mode = ScaleModeNoScale;
      goto OUT;
    } else if (strcmp(eval->value_nick, "scale2fit")) {
      *scale_mode = ScaleModeFill;
      goto OUT;
    } else if (strcmp(eval->value_nick, "zoom2fit")) {
      *scale_mode = ScaleModeKeepAspectRatio;
      goto OUT;
    } else if (strcmp(eval->value_nick, "zoom2fill")) {
      *scale_mode = ScaleModeFillKeepAspectRatio;
      goto OUT;
    } else {
      UMMS_DEBUG("Error scale mode");
      ret = FALSE;
    }
  } else {
    UMMS_DEBUG("No a vidrend_bin set, scale not support now!");
    ret = FALSE;
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}

static gboolean
engine_gst_suspend (MeegoMediaPlayerControl *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (priv->suspended)
    return TRUE;

  if (priv->player_state == PlayerStatePaused || priv->player_state == PlayerStatePlaying) {
    engine_gst_get_position (self, &priv->pos);
    engine_gst_stop (self);
  } else if (priv->player_state == PlayerStateStopped) {
    priv->pos = 0;
  }
  priv->suspended = TRUE;
  UMMS_DEBUG ("meego_media_player_control_emit_suspended");
  meego_media_player_control_emit_suspended (self);
  return TRUE;
}

//restore asynchronously.
//Set to pause here, and do actual retoring operation in bus_message_state_change_cb().
static gboolean
engine_gst_restore (MeegoMediaPlayerControl *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (priv->player_state != PlayerStateStopped || !priv->suspended) {
    return FALSE;
  }

  return engine_gst_pause (self);
}


static gboolean
engine_gst_get_video_codec (MeegoMediaPlayerControl *self, gint channel, gchar ** video_codec)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *video_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (size = gst_tag_list_get_tag_size(tag_list, GST_TAG_VIDEO_CODEC) > 0) {
    gchar *st = NULL;

    for (i = 0; i < size; ++i) {
      if (gst_tag_list_get_string_index (tag_list, GST_TAG_VIDEO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the video codec named: %s", channel, st);
        if (codec_name) {
          codec_name = g_strconcat(codec_name, st);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if (codec_name)
    *video_codec = codec_name;

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
engine_gst_get_audio_codec (MeegoMediaPlayerControl *self, gint channel, gchar ** audio_codec)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *audio_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (size = gst_tag_list_get_tag_size(tag_list, GST_TAG_AUDIO_CODEC) > 0) {
    gchar *st = NULL;

    for (i = 0; i < size; ++i) {
      if (gst_tag_list_get_string_index (tag_list, GST_TAG_AUDIO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the audio codec named: %s", channel, st);
        if (codec_name) {
          codec_name = g_strconcat(codec_name, st);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if (codec_name)
    *audio_codec = codec_name;

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}

static gboolean
engine_gst_get_video_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *video_rate)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  guint32 bit_rate = 0;

  *video_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate / 1000;
  } else if (gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate / 1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
engine_gst_get_audio_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *audio_rate)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  guint32 bit_rate = 0;

  *audio_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate / 1000;
  } else if (gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate / 1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
engine_gst_get_encapsulation(MeegoMediaPlayerControl *self, gchar ** encapsulation)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gchar *enca_name = NULL;

  *encapsulation = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  if (priv->tag_list) {
    gst_tag_list_get_string (priv->tag_list, GST_TAG_CONTAINER_FORMAT, &enca_name);
    if (enca_name) {
      UMMS_DEBUG("get the container name: %s", enca_name);
      *encapsulation = enca_name;
    } else {
      UMMS_DEBUG("no infomation about the container.");
    }
  }

  return TRUE;
}


static gboolean
engine_gst_get_audio_samplerate(MeegoMediaPlayerControl *self, gint channel, gint * sample_rate)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *sample_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_int (s, "rate", sample_rate);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
engine_gst_get_video_framerate(MeegoMediaPlayerControl *self, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *frame_rate_num = 0;
  *frame_rate_denom = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "framerate", frame_rate_num, frame_rate_denom);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
engine_gst_get_video_resolution(MeegoMediaPlayerControl *self, gint channel, gint * width, gint * height)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;

  *width = 0;
  *height = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    gst_structure_get_int (s, "width", width);
    gst_structure_get_int (s, "height", height);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return TRUE;
}


static gboolean
engine_gst_get_video_aspect_ratio(MeegoMediaPlayerControl *self, gint channel,
    gint * ratio_num, gint * ratio_denom)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *ratio_num = 0;
  *ratio_denom = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "pixel-aspect-ratio", ratio_num, ratio_denom);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
engine_gst_get_protocol_name(MeegoMediaPlayerControl *self, gchar ** prot_name)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gchar * uri = NULL;

  *prot_name = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  g_object_get (G_OBJECT (pipe), "uri", &uri, NULL);

  if (!uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipe);
    return FALSE;
  }

  if (!gst_uri_is_valid(uri)) {
    UMMS_DEBUG("uri: %s  is invalid", uri);
    g_free(uri);
    return FALSE;
  }

  UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri is %s", pipe, uri);

  *prot_name = gst_uri_get_protocol(uri);
  UMMS_DEBUG("Get the protocol name is %s", *prot_name);

  g_free(uri);
  return TRUE;
}


static gboolean
engine_gst_get_current_uri(MeegoMediaPlayerControl *self, gchar ** uri)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gchar * s_uri = NULL;

  *uri = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_object_get (G_OBJECT (pipe), "uri", &s_uri, NULL);

  if (!s_uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipe);
    return FALSE;
  }

  if (!gst_uri_is_valid(s_uri)) {
    UMMS_DEBUG("uri: %s  is invalid", s_uri);
    g_free(s_uri);
    return FALSE;
  }

  *uri = s_uri;
  return TRUE;
}

static gboolean
engine_gst_get_title(MeegoMediaPlayerControl *self, gchar ** title)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *title = g_strdup (priv->title);
  UMMS_DEBUG ("title = %s", *title);

  return TRUE;
}

static gboolean
engine_gst_get_artist(MeegoMediaPlayerControl *self, gchar ** artist)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *artist = g_strdup (priv->artist);
  UMMS_DEBUG ("artist = %s", *artist);
  return TRUE;
}

static void
meego_media_player_control_init (MeegoMediaPlayerControl *iface)
{
  MeegoMediaPlayerControlClass *klass = (MeegoMediaPlayerControlClass *)iface;

  meego_media_player_control_implement_set_uri (klass,
      engine_gst_set_uri);
  meego_media_player_control_implement_set_target (klass,
      engine_gst_set_target);
  meego_media_player_control_implement_play (klass,
      engine_gst_play);
  meego_media_player_control_implement_pause (klass,
      engine_gst_pause);
  meego_media_player_control_implement_stop (klass,
      engine_gst_stop);
  meego_media_player_control_implement_set_video_size (klass,
      engine_gst_set_video_size);
  meego_media_player_control_implement_get_video_size (klass,
      engine_gst_get_video_size);
  meego_media_player_control_implement_is_seekable (klass,
      engine_gst_is_seekable);
  meego_media_player_control_implement_set_position (klass,
      engine_gst_set_position);
  meego_media_player_control_implement_get_position (klass,
      engine_gst_get_position);
  meego_media_player_control_implement_set_playback_rate (klass,
      engine_gst_set_playback_rate);
  meego_media_player_control_implement_get_playback_rate (klass,
      engine_gst_get_playback_rate);
  meego_media_player_control_implement_set_volume (klass,
      engine_gst_set_volume);
  meego_media_player_control_implement_get_volume (klass,
      engine_gst_get_volume);
  meego_media_player_control_implement_get_media_size_time (klass,
      engine_gst_get_media_size_time);
  meego_media_player_control_implement_get_media_size_bytes (klass,
      engine_gst_get_media_size_bytes);
  meego_media_player_control_implement_has_video (klass,
      engine_gst_has_video);
  meego_media_player_control_implement_has_audio (klass,
      engine_gst_has_audio);
  meego_media_player_control_implement_support_fullscreen (klass,
      engine_gst_support_fullscreen);
  meego_media_player_control_implement_is_streaming (klass,
      engine_gst_is_streaming);
  meego_media_player_control_implement_get_player_state (klass,
      engine_gst_get_player_state);
  meego_media_player_control_implement_get_buffered_bytes (klass,
      engine_gst_get_buffered_bytes);
  meego_media_player_control_implement_get_buffered_time (klass,
      engine_gst_get_buffered_time);
  meego_media_player_control_implement_get_current_video (klass,
      engine_gst_get_current_video);
  meego_media_player_control_implement_get_current_audio (klass,
      engine_gst_get_current_audio);
  meego_media_player_control_implement_set_current_video (klass,
      engine_gst_set_current_video);
  meego_media_player_control_implement_set_current_audio (klass,
      engine_gst_set_current_audio);
  meego_media_player_control_implement_get_video_num (klass,
      engine_gst_get_video_num);
  meego_media_player_control_implement_get_audio_num (klass,
      engine_gst_get_audio_num);
  meego_media_player_control_implement_set_proxy (klass,
      engine_gst_set_proxy);
  meego_media_player_control_implement_set_subtitle_uri (klass,
      engine_gst_set_subtitle_uri);
  meego_media_player_control_implement_get_subtitle_num (klass,
      engine_gst_get_subtitle_num);
  meego_media_player_control_implement_set_current_subtitle (klass,
      engine_gst_set_current_subtitle);
  meego_media_player_control_implement_get_current_subtitle (klass,
      engine_gst_get_current_subtitle);
  meego_media_player_control_implement_set_buffer_depth (klass,
      engine_gst_set_buffer_depth);
  meego_media_player_control_implement_set_mute (klass,
      engine_gst_set_mute);
  meego_media_player_control_implement_is_mute (klass,
      engine_gst_is_mute);
  meego_media_player_control_implement_suspend (klass,
      engine_gst_suspend);
  meego_media_player_control_implement_restore (klass,
      engine_gst_restore);
  meego_media_player_control_implement_set_scale_mode (klass,
      engine_gst_set_scale_mode);
  meego_media_player_control_implement_get_scale_mode (klass,
      engine_gst_get_scale_mode);
  meego_media_player_control_implement_get_video_codec (klass,
      engine_gst_get_video_codec);
  meego_media_player_control_implement_get_audio_codec (klass,
      engine_gst_get_audio_codec);
  meego_media_player_control_implement_get_video_bitrate (klass,
      engine_gst_get_video_bitrate);
  meego_media_player_control_implement_get_audio_bitrate (klass,
      engine_gst_get_audio_bitrate);
  meego_media_player_control_implement_get_encapsulation (klass,
      engine_gst_get_encapsulation);
  meego_media_player_control_implement_get_audio_samplerate (klass,
      engine_gst_get_audio_samplerate);
  meego_media_player_control_implement_get_video_framerate (klass,
      engine_gst_get_video_framerate);
  meego_media_player_control_implement_get_video_resolution (klass,
      engine_gst_get_video_resolution);
  meego_media_player_control_implement_get_video_aspect_ratio (klass,
      engine_gst_get_video_aspect_ratio);
  meego_media_player_control_implement_get_protocol_name (klass,
      engine_gst_get_protocol_name);
  meego_media_player_control_implement_get_current_uri (klass,
      engine_gst_get_current_uri);
  meego_media_player_control_implement_get_title (klass,
      engine_gst_get_title);
  meego_media_player_control_implement_get_artist (klass,
      engine_gst_get_artist);
}

static void
engine_gst_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
engine_gst_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
engine_gst_dispose (GObject *object)
{
  EngineGstPrivate *priv = GET_PRIVATE (object);

  _stop_pipe ((MeegoMediaPlayerControl *)object);

  TEARDOWN_ELEMENT (priv->source);
  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  TEARDOWN_ELEMENT (priv->pipeline);

  if (priv->target_type == XWindow) {
    unset_xwindow_target ((MeegoMediaPlayerControl *)object);
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL;
  }

  if (priv->tag_list)
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;

  G_OBJECT_CLASS (engine_gst_parent_class)->dispose (object);
}

static void
engine_gst_finalize (GObject *object)
{
  EngineGstPrivate *priv = GET_PRIVATE (object);

  RESET_STR(priv->uri);
  RESET_STR(priv->title);
  RESET_STR(priv->artist);
  RESET_STR(priv->proxy_uri);
  RESET_STR(priv->proxy_id);
  RESET_STR(priv->proxy_pw);

  G_OBJECT_CLASS (engine_gst_parent_class)->finalize (object);
}

static void
engine_gst_class_init (EngineGstClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (EngineGstPrivate));

  object_class->get_property = engine_gst_get_property;
  object_class->set_property = engine_gst_set_property;
  object_class->dispose = engine_gst_dispose;
  object_class->finalize = engine_gst_finalize;
}

static void
bus_message_state_change_cb (GstBus     *bus,
    GstMessage *message,
    EngineGst  *self)
{

  GstState old_state, new_state;
  PlayerState old_player_state;
  gpointer src;
  gboolean seekable;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);

  old_player_state = priv->player_state;
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
    if (old_player_state == PlayerStateStopped && priv->suspended) {
      UMMS_DEBUG ("restoring suspended execution, pos = %lld", priv->pos);
      engine_gst_is_seekable (MEEGO_MEDIA_PLAYER_CONTROL(self), &seekable);
      if (seekable) {
        engine_gst_set_position (MEEGO_MEDIA_PLAYER_CONTROL(self), priv->pos);
      }
      engine_gst_play(MEEGO_MEDIA_PLAYER_CONTROL(self));
      priv->suspended = FALSE;
      UMMS_DEBUG ("meego_media_player_control_emit_restored");
      meego_media_player_control_emit_restored (self);
    }
  } else if (new_state == GST_STATE_PLAYING) {
    priv->player_state = PlayerStatePlaying;
  } else {
    if (new_state < old_state)//down state change to GST_STATE_READY
      priv->player_state = PlayerStateStopped;
  }

  if (priv->pending_state == priv->player_state)
    priv->pending_state = PlayerStateNull;

  if (priv->player_state != old_player_state) {
    UMMS_DEBUG ("emit state changed, old=%d, new=%d", old_player_state, priv->player_state);
    meego_media_player_control_emit_player_state_changed (self, old_player_state, priv->player_state);
  }
}

static void
bus_message_get_tag_cb (GstBus *bus, GstMessage *message, EngineGst  *self)
{
  gpointer src;
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstPad * src_pad = NULL;
  GstTagList * tag_list = NULL;
  guint32 bit_rate;
  gchar * pad_name = NULL;
  gchar * element_name = NULL;
  gchar * title = NULL;
  gchar * artist = NULL;
  gboolean metadata_changed = FALSE;

  src = GST_MESSAGE_SRC (message);

  if (message->type != GST_MESSAGE_TAG) {
    UMMS_DEBUG("not a tag message in a registered tag signal, strange");
    return;
  }

  gst_message_parse_tag_full (message, &src_pad, &tag_list);
  if (src_pad) {
    pad_name = g_strdup (GST_PAD_NAME (src_pad));
    UMMS_DEBUG("The pad name is %s", pad_name);
  }

  if (message->src) {
    element_name = g_strdup (GST_ELEMENT_NAME (message->src));
    UMMS_DEBUG("The element name is %s", element_name);
  }

  priv->tag_list =
    gst_tag_list_merge(priv->tag_list, tag_list, GST_TAG_MERGE_REPLACE);

  //cache the title
  if (gst_tag_list_get_string_index (tag_list, GST_TAG_TITLE, 0, &title)) {
    UMMS_DEBUG("Element: %s, provide the title: %s", element_name, title);
    RESET_STR(priv->title);
    priv->title = title;
    metadata_changed = TRUE;
  }

  //cache the artist
  if (gst_tag_list_get_string_index (tag_list, GST_TAG_ARTIST, 0, &artist)) {
    UMMS_DEBUG("Element: %s, provide the artist: %s", element_name, artist);
    RESET_STR(priv->artist);
    priv->artist = artist;
    metadata_changed = TRUE;
  }

  //only care about artist and title
  if (metadata_changed) {
    meego_media_player_control_emit_metadata_changed (self);
  }

#if 0
  gint size, i;
  gchar * video_codec = NULL;
  gchar * audio_codec = NULL;
  int out_of_channel = 0;

  /* This logic may be used when the inputselector is not included.
     Now we just get the video and audio codec from inputselector's pad. *

  /* We are now interest in the codec, container format and bit rate. */
  if (size = gst_tag_list_get_tag_size(tag_list, GST_TAG_VIDEO_CODEC) > 0) {
    video_codec = g_strdup_printf("%s-->%s Video Codec: ",
                  element_name ? element_name : "NULL", pad_name ? pad_name : "NULL");

    for (i = 0; i < size; ++i) {
      gchar *st = NULL;

      if (gst_tag_list_get_string_index (tag_list, GST_TAG_VIDEO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Element: %s, Pad: %s provide the video codec named: %s", element_name, pad_name, st);
        video_codec = g_strconcat(video_codec, st);
        g_free (st);
      }
    }

    /* store the name for later use. */
    g_strlcpy(priv->video_codec, video_codec, ENGINE_GST_MAX_VIDEOCODEC_SIZE);
    UMMS_DEBUG("%s", video_codec);
  }

  if (size = gst_tag_list_get_tag_size(tag_list, GST_TAG_AUDIO_CODEC) > 0) {
    audio_codec = g_strdup_printf("%s-->%s Audio Codec: ",
                  element_name ? element_name : "NULL", pad_name ? pad_name : "NULL");

    for (i = 0; i < size; ++i) {
      gchar *st = NULL;

      if (gst_tag_list_get_string_index (tag_list, GST_TAG_AUDIO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Element: %s, Pad: %s provide the audio codec named: %s", element_name, pad_name, st);
        audio_codec = g_strconcat(audio_codec, st);
        g_free (st);
      }
    }

    UMMS_DEBUG("%s", audio_codec);

    /* need to consider the multi-channel audio case. The demux and decoder
     * will both send this message. We prefer codec info from decoder now. Need to improve */
    if (element_name && (g_strstr_len(element_name, strlen(element_name), "demux") ||
         g_strstr_len(element_name, strlen(element_name), "Demux") ||
         g_strstr_len(element_name, strlen(element_name), "DEMUX"))) {
      if (priv->audio_codec_used < ENGINE_GST_MAX_AUDIO_STREAM) {
        g_strlcpy(priv->audio_codec[priv->audio_codec_used], audio_codec, ENGINE_GST_MAX_AUDIOCODEC_SIZE);
        priv->audio_codec_used++;
      } else {
        UMMS_DEBUG("audio_codec need to discard because too many steams");
        out_of_channel = 1;
      }
    } else {
      UMMS_DEBUG("audio_codec need to discard because it not come from demux");
    }
  }

  if (gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    /* Again, the bitrate info may come from demux and audio decoder, we use demux now. */
    UMMS_DEBUG("Element: %s, Pad: %s provide the bitrate: %d", element_name, pad_name, bit_rate);
    if (element_name && (g_strstr_len(element_name, strlen(element_name), "demux") ||
         g_strstr_len(element_name, strlen(element_name), "Demux") ||
         g_strstr_len(element_name, strlen(element_name), "DEMUX"))) {
      gchar * codec = NULL;
      int have_found = 0;

      /* first we check whether it is the bitrate of video. */
      if (video_codec) { /* the bitrate sent with the video codec, easy one. */
        have_found = 1;
        priv->video_bitrate = bit_rate;
        UMMS_DEBUG("we set the bitrate: %u for video", bit_rate);
      }

      if (!have_found) { /* try to compare the element and pad name. */
        codec = g_strdup_printf("%s-->%s Video Codec: ",
                element_name ? element_name : "NULL", pad_name ? pad_name : "NULL");
        if (g_strncasecmp(priv->video_codec, codec, strlen(codec))) {
          have_found = 1;
          priv->video_bitrate = bit_rate;
          UMMS_DEBUG("we set the bitrate: %u for video", bit_rate);
        }

        g_free(codec);
      }

      /* find it in the audio codec stream. */
      if (!have_found) {
        if (audio_codec) {
          /* the bitrate sent with the audio codec, easy one. */
          have_found = 1;
          if (!out_of_channel) {
            priv->audio_bitrate[priv->audio_codec_used -1] = bit_rate;
            UMMS_DEBUG("we set the bitrate: %u for audio stream: %d", bit_rate, priv->audio_codec_used - 1);
          } else {
            UMMS_DEBUG("audio bitrate need to discard because too many steams");
          }
        }

        if (!have_found) { /* last try, use audio element and pad to index. */
          codec = g_strdup_printf("%s-->%s Audio Codec: ",
                  element_name ? element_name : "NULL", pad_name ? pad_name : "NULL");

          for (i = 0; i < priv->audio_codec_used; i++) {
            if (g_strncasecmp(priv->audio_codec[i], codec, strlen(codec)))
              break;
          }

          have_found = 1; /* if not find, we use audio channel as defaule, so always find. */
          if (i < priv->audio_codec_used) {
            priv->audio_bitrate[i] = bit_rate;
            UMMS_DEBUG("we set the bitrate: %u for audio stream: %d", bit_rate, i);
          } else {
            UMMS_DEBUG("we can not find the stream for this bitrate: %u, set to the fist stream", bit_rate);
            priv->audio_bitrate[0] = bit_rate;
          }

          g_free(codec);
        }
      }
    }
  }

  if (video_codec)
    g_free(video_codec);
  if (audio_codec)
    g_free(audio_codec);
#endif

  if (src_pad)
    g_object_unref(src_pad);
  gst_tag_list_free (tag_list);
  if (pad_name)
    g_free(pad_name);
  if (element_name)
    g_free(element_name);

}


static void
bus_message_eos_cb (GstBus     *bus,
                    GstMessage *message,
                    EngineGst  *self)
{
  UMMS_DEBUG ("message::eos received on bus");

  meego_media_player_control_emit_eof (self);
}


static void
bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    EngineGst  *self)
{
  GError *error = NULL;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("message::error received on bus");

  gst_message_parse_error (message, &error, NULL);
  _stop_pipe (MEEGO_MEDIA_PLAYER_CONTROL(self));

  meego_media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("Error emitted with message = %s", error->message);

  g_clear_error (&error);
}

static void
bus_message_buffering_cb (GstBus *bus,
    GstMessage *message,
    EngineGst *self)
{
  const GstStructure *str;
  gboolean res;
  gint buffer_percent;
  GstState state, pending_state;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  str = gst_message_get_structure (message);
  if (!str)
    return;

  res = gst_structure_get_int (str, "buffer-percent", &buffer_percent);
  if (!(buffer_percent % 25))
    UMMS_DEBUG ("buffering...%d%% ", buffer_percent);

  if (res) {
    priv->buffer_percent = buffer_percent;

    if (priv->buffer_percent == 100) {

      UMMS_DEBUG ("Done buffering");
      priv->buffering = FALSE;
      if (priv->pending_state == PlayerStatePlaying)
        gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
      meego_media_player_control_emit_buffered (self);

    } else if (!priv->buffering && priv->pending_state == PlayerStatePlaying) {
      priv->buffering = TRUE;
      UMMS_DEBUG ("Set pipeline to paused for buffering data");
      gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);
      meego_media_player_control_emit_buffering (self);
    }
  }
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus,
                  GstMessage *message,
                  EngineGst *engine)
{
  EngineGstPrivate *priv;
  GstElement *vsink;
  GError *err = NULL;
  GstMessage *msg = NULL;

  if ( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if ( ! gst_structure_has_name( message->structure, "prepare-gdl-plane" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (engine, GST_BUS_DROP);
  g_return_val_if_fail (ENGINE_IS_GST (engine), GST_BUS_DROP);
  priv = GET_PRIVATE (engine);

  UMMS_DEBUG ("sync-handler received on bus: prepare-gdl-plane");
  vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));

  if (!prepare_plane ((MeegoMediaPlayerControl *)engine)) {
    //Since we are in streame thread, let the vsink to post the error message. Handle it in bus_message_error_cb().
    err = g_error_new_literal (UMMS_RESOURCE_ERROR, UMMS_RESOURCE_ERROR_NO_RESOURCE, "Plane unavailable");
    msg = gst_message_new_error (GST_OBJECT_CAST(priv->pipeline), err, "No resource");
    gst_element_post_message (priv->pipeline, msg);
    g_error_free (err);
  }

//  meego_media_player_control_emit_request_window (engine);

  return( GST_BUS_DROP );
}

static void
video_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  EngineGst * priv = (EngineGst *) user_data;
  meego_media_player_control_emit_video_tag_changed(priv, stream_id);
}

static void
audio_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  EngineGst * priv = (EngineGst *) user_data;
  meego_media_player_control_emit_audio_tag_changed(priv, stream_id);
}

static void
text_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  EngineGst * priv = (EngineGst *) user_data;
  meego_media_player_control_emit_text_tag_changed(priv, stream_id);
}

/* GstPlayFlags flags from playbin2 */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

static void
engine_gst_init (EngineGst *self)
{
  EngineGstPrivate *priv;
  GstBus *bus;
  GstPlayFlags flags;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;


  priv->pipeline = gst_element_factory_make ("playbin2", "pipeline");

  g_signal_connect(priv->pipeline, "notify::source", G_CALLBACK(_source_changed_cb), self);

  bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));

  gst_bus_add_signal_watch (bus);

  g_signal_connect_object (bus, "message::error",
      G_CALLBACK (bus_message_error_cb),
      self,
      0);

  g_signal_connect_object (bus, "message::buffering",
      G_CALLBACK (bus_message_buffering_cb),
      self,
      0);

  g_signal_connect_object (bus, "message::eos",
      G_CALLBACK (bus_message_eos_cb),
      self,
      0);

  g_signal_connect_object (bus,
      "message::state-changed",
      G_CALLBACK (bus_message_state_change_cb),
      self,
      0);

  g_signal_connect_object (bus,
      "message::tag",
      G_CALLBACK (bus_message_get_tag_cb),
      self,
      0);

  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
      self);

  gst_object_unref (GST_OBJECT (bus));

  g_signal_connect (priv->pipeline,
                    "video-tags-changed", G_CALLBACK (video_tags_changed_cb), self);

  g_signal_connect (priv->pipeline,
                    "audio-tags-changed", G_CALLBACK (audio_tags_changed_cb), self);

  g_signal_connect (priv->pipeline,
                    "text-tags-changed", G_CALLBACK (text_tags_changed_cb), self);

  /*
   *Use GST_PLAY_FLAG_DOWNLOAD flag to enable Gstreamer Download buffer mode,
   *so that we can query the buffered time/bytes, and further the time rangs.
   */

  g_object_get (priv->pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_DOWNLOAD;
  g_object_set (priv->pipeline, "flags", flags, NULL);

  priv->player_state = PlayerStateNull;
  priv->suspended = FALSE;
  priv->pos = 0;
  priv->res_mngr = umms_resource_manager_new ();
  priv->res_list = NULL;

  priv->tag_list = NULL;

  //Setup default target.
#define FULL_SCREEN_RECT "0,0,0,0"
  setup_ismd_vbin (MEEGO_MEDIA_PLAYER_CONTROL(self), FULL_SCREEN_RECT, UPP_A);
  priv->target_type = ReservedType0;
  priv->target_initialized = TRUE;
}

EngineGst *
engine_gst_new (void)
{
  return g_object_new (ENGINE_TYPE_GST, NULL);
}

static void
_set_proxy (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  g_return_if_fail (priv->source);
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->source), "proxy") == NULL)
    return;

  UMMS_DEBUG ("Setting proxy. proxy=%s, id=%s, pw=%s", priv->proxy_uri, priv->proxy_id, priv->proxy_pw);
  g_object_set (priv->source, "proxy", priv->proxy_uri,
                "proxy-id", priv->proxy_id,
                "proxy-pw", priv->proxy_pw, NULL);
}

static void
_source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data)
{
  GstElement *source;
  EngineGstPrivate *priv = GET_PRIVATE (data);

  g_object_get(priv->pipeline, "source", &source, NULL);
  gst_object_replace((GstObject**) &priv->source, (GstObject*) source);
  UMMS_DEBUG ("source changed");
  _set_proxy ((MeegoMediaPlayerControl *)data);

  return;
}


#define HW_FORMAT_NUM 4
#define HW_H264_CAPS \
             "video/h264, " \
           "  format = (fourcc) H264, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088]; " \
           "video/x-h264, " \
           "  format = (fourcc) X264, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088]; "


#define HW_MPEG2_CAPS \
             "video/mpeg, " \
           "  format = (fourcc) {MPEG, MP2V}, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088], " \
           "  mpegversion = (int) [1, 2], " \
           "  systemstream = (boolean) false, " \
           "  ispacketized = (boolean) false; "

#define HW_MPEG4_CAPS \
             "video/mpeg, " \
           "  mpegversion = (int) 4, " \
           "  systemstream = (boolean) false; " \
           "video/x-xvid;" \
           "video/x-divx, " \
           "  format = (fourcc) {MPEG, MP4V}, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088], " \
           "  divxversion = (int) [4, 5] "

#define HW_VC1_CAPS \
             "video/x-wmv, " \
           "  wmvversion = (int)3, " \
           "  format = (fourcc) { WMV3, WVC1 }"

static GstStaticCaps hw_h264_static_caps = GST_STATIC_CAPS (HW_H264_CAPS);
static GstStaticCaps hw_mpeg2_static_caps = GST_STATIC_CAPS (HW_MPEG2_CAPS);
static GstStaticCaps hw_mpeg4_static_caps = GST_STATIC_CAPS (HW_MPEG4_CAPS);
static GstStaticCaps hw_vc1_static_caps = GST_STATIC_CAPS (HW_VC1_CAPS);

static GstStaticCaps *hw_static_caps[HW_FORMAT_NUM] = {&hw_h264_static_caps,
    &hw_mpeg2_static_caps,
    &hw_mpeg4_static_caps,
    &hw_vc1_static_caps
                                                      };

static gboolean
autoplug_continue_cb (GstElement * element, GstPad * pad, GstCaps * caps,
    MeegoMediaPlayerControl *control)
{
  gint i;
  const gchar *name = NULL;
  GstStructure *s = NULL;
  GstCaps *hw_caps = NULL;
  gboolean is_hw_vcaps = FALSE;
  gboolean ret = TRUE;
  EngineGstPrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ( "pad caps : %" GST_PTR_FORMAT,
               caps);

  //check software a/v caps.
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  if (!strncmp (name, "video", 5)) {
    priv->has_video = TRUE;
    //check HW video caps
    for (i = 0; i < HW_FORMAT_NUM; i++) {
      hw_caps = gst_static_caps_get (hw_static_caps[i]);

      if (gst_caps_can_intersect (hw_caps, caps)) {
        is_hw_vcaps = TRUE;
        priv->hw_viddec++;

        /*
         * Don't continue to autoplug this pad,
         * which avoids the failure of creating and plugin HW video decoder, and hence guarantees "no-more-pads"
         * signal will be emitted.
         *
         */
        ret = FALSE;
      }

      gst_caps_unref (hw_caps);

      if (is_hw_vcaps)
        goto hw_video_caps;
    }
  } else if (!strncmp (name, "audio", 5)) {
    priv->has_audio = TRUE;
    ret = FALSE;//Exposes this pad, and hence guarantees "no-more-pads" signal will be emitted.
    //FIXME: check the HW audio caps.
  } else if (!strncmp (name, "text", 4)) {
    priv->has_sub = TRUE;
  } else {
    //nothing
  }

  return ret;

hw_video_caps:
  UMMS_DEBUG ("new stream with caps(%" GST_PTR_FORMAT ") needs HW video decoder resource", caps);
  return ret;
}

static gboolean
uri_parsing_finished_cb (MeegoMediaPlayerControl * self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG("Begin");

  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  priv->uri_parsed = TRUE;

  UMMS_DEBUG("End, has_video=%d, num of hw_viddec=%d, has_audio=%d, num of hw_auddec=%d",
             priv->has_video, priv->hw_viddec, priv->has_audio, priv->hw_auddec);
  return FALSE;
}

static void no_more_pads_cb (GstElement * uridecodebin, MeegoMediaPlayerControl * self)
{
  UMMS_DEBUG("Begin");
  g_idle_add ((GSourceFunc)uri_parsing_finished_cb, self);
  UMMS_DEBUG("End");
}

/*
 * Parse the av asset spacified by the uri.
 *
 * Called when set state from PlayerStateNull/PlayerStateStopped to PlayerStatePaused/PlayerStatePlaying.
 * Through this parsing, we can get the number of streams and their format, and can determine the
 * resources(HW clock, HW a/v decoder) needed when playing this av asset.
 *
 * Return TRUE if successfully initializeithe parsing action.
 *        FALSE if not.
 *
 * Note: This is a asynchronous invoking, so, requesting resources and setting pipeline to Pause/Playing will be
 * done in uri_parsing_finished_cb().
 */
static gboolean
parse_uri_async (MeegoMediaPlayerControl *self, gchar *uri)
{
  GstElement *uridecodebin;
  GstStateChangeReturn res;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (uri, FALSE);
  g_return_val_if_fail ((!priv->uri_parse_pipe), TRUE);

  priv->is_live = IS_LIVE_URI(priv->uri);

  //use uridecodebin to automatically detect streams.
  priv->uri_parse_pipe = uridecodebin = gst_element_factory_make ("uridecodebin", NULL);
  if (!uridecodebin) {
    return FALSE;
  }

  g_object_set (G_OBJECT(uridecodebin), "uri", priv->uri, NULL);

  g_signal_connect (uridecodebin, "autoplug-continue",
                    G_CALLBACK (autoplug_continue_cb), self);

  g_signal_connect (uridecodebin, "no-more-pads",
                    G_CALLBACK (no_more_pads_cb), self);

  if (gst_element_set_state (uridecodebin, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    TEARDOWN_ELEMENT (priv->uri_parse_pipe);
    return FALSE;
  }
  return TRUE;
}
