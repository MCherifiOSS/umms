#include <string.h>
#include <stdio.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib/gprintf.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"
#include "dvb-player.h"
#include "meego-media-player-control.h"
#include "param-table.h"

/* add PAT, CAT, NIT, SDT, EIT to pids filter for dvbsrc */
#define INIT_PIDS "0:1:16:17:18"


static void meego_media_player_control_init (MeegoMediaPlayerControl* iface);

G_DEFINE_TYPE_WITH_CODE (DvbPlayer, dvb_player, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEEGO_TYPE_MEDIA_PLAYER_CONTROL, meego_media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DVB_TYPE_PLAYER, DvbPlayerPrivate))

#define GET_PRIVATE(o) ((DvbPlayer *)o)->priv

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

struct _DvbPlayerPrivate {

  GstElement *pipeline;
  GstElement *source;
  GstElement *tsdemux;
  GstElement *vsink;
  GstElement *asink;
  GstElement *mq;
  GstElement *tsfilesink;

  gchar *uri;
  gint  seekable;

  //UMMS defined player state
  PlayerState player_state;//current state
  PlayerState pending_state;//target state, for async state change(*==>PlayerStatePaused, PlayerStateNull/Stopped==>PlayerStatePlaying)

  gboolean is_live;
  gboolean target_initialized;

  //XWindow target stuff
  gboolean xwin_initialized;
  gint     target_type;
  Window   app_win_id;//top-level window
  Window   video_win_id;
  Display  *disp;
  GThread  *event_thread;
  gboolean event_thread_running;
  
  //resource management
  UmmsResourceManager *res_mngr;//no need to unref, since it is global singleton.
  GList    *res_list;
  gboolean resource_prepared;
  //flags to indicate resource needed by uri, should be reset before setting a new uri.
  gboolean has_video;
  gint     hw_viddec;//number of HW viddec needed
  gboolean has_audio;
  gint     hw_auddec;//number of HW auddec needed
  gboolean has_sub;//FIXME: what resource needed by subtitle?

  /* Use it to buffer the tag of the stream. */
  GstTagList *tag_list;
  gchar *title;
  gchar *artist;

  GList *elements;//decodarble and audio/video sink element factory
  guint32  elements_cookie;
};

static gboolean _stop_pipe (MeegoMediaPlayerControl *control);
static gboolean dvb_player_set_video_size (MeegoMediaPlayerControl *self, guint x, guint y, guint w, guint h);
static gboolean create_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean destroy_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean parse_uri_async (MeegoMediaPlayerControl *self, gchar *uri);
static void release_resource (MeegoMediaPlayerControl *self);
static gboolean dvb_player_set_subtitle_uri(MeegoMediaPlayerControl *player, gchar *suburi);
static void pad_added_cb (GstElement *element, GstPad *pad, gpointer data);
static void no_more_pads_cb (GstElement *element, gpointer data);
static gboolean autoplug_pad(DvbPlayer *player, GstPad *pad, gint chain_type);
static GstPad *link_multiqueue (DvbPlayer *player, GstPad *pad);
static void update_elements_list (DvbPlayer *player);
static gboolean autoplug_dec_element (DvbPlayer *player, GstElement * element);
static gboolean link_sink (DvbPlayer *player, GstPad *pad);
static GstPad *get_sink_pad (GstElement * element);

enum {
  CHAIN_TYPE_VIDEO,
  CHAIN_TYPE_AUDIO,
  CHAIN_TYPE_SUB
};

//TODO: figure out the tuner parameters and program number 
static gboolean
parse_uri (DvbPlayer *player, const gchar *uri)
{
  gboolean ret = TRUE;
  return ret;
}

static gboolean
dvb_player_set_uri (MeegoMediaPlayerControl *self,
                    const gchar           *uri)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("SetUri called: old = %s new = %s",
              priv->uri,
              uri);

  if (priv->player_state != PlayerStateNull) {
    UMMS_DEBUG ("Failed, can only set uri on Null state");
    return FALSE;
  }

  if (priv->uri) {
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);

  return parse_uri (DVB_PLAYER (self), uri); 
}

static gboolean
get_video_rectangle (MeegoMediaPlayerControl *self, gint *ax, gint *ay, gint *w, gint *h, gint *rx, gint *ry)
{
  XWindowAttributes video_win_attr, app_win_attr;
  gint app_x, app_y;
  Window junkwin;
  Status status;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  property = XInternAtom (priv->disp,"_MUTTER_HINTS",0);
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

  DvbPlayerPrivate *priv = GET_PRIVATE (self);
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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  if (!priv->xwin_initialized)
    return TRUE;

  destroy_xevent_handle_thread (self);
  priv->xwin_initialized = FALSE;
  return TRUE;
}

static void
dvb_player_handle_xevents (MeegoMediaPlayerControl *control)
{
  XEvent e;
  gint x,y,w,h,rx,ry;
  DvbPlayerPrivate *priv = GET_PRIVATE (control);

  g_return_if_fail (control);

  /* Handle Expose */
  while (XCheckWindowEvent (priv->disp,
          priv->app_win_id, StructureNotifyMask, &e)) {
    switch (e.type) {
      case ConfigureNotify:
        get_video_rectangle (control, &x, &y, &w, &h, &rx, &ry);
        cutout (control, rx, ry, w, h);
        dvb_player_set_video_size (control, x, y, w, h);
        UMMS_DEBUG ("Got ConfigureNotify, video window abs_x=%d,abs_y=%d,w=%d,y=%d", x, y, w, h);
        break;
      default:
        break;
    }
  }
}

static gpointer
dvb_player_event_thread (MeegoMediaPlayerControl* control)
{

  DvbPlayerPrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ("Begin");
  while (priv->event_thread_running) {

    if (priv->app_win_id) {
      dvb_player_handle_xevents (control);
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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (!priv->event_thread) {
      /* Setup our event listening thread */
      UMMS_DEBUG ("run xevent thread");
      priv->event_thread_running = TRUE;
      priv->event_thread = g_thread_create (
          (GThreadFunc) dvb_player_event_thread, self, TRUE, NULL);
  }
  return TRUE;
}

static gboolean
destroy_xevent_handle_thread (MeegoMediaPlayerControl *self)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

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
  GstElement *new_vsink = NULL;
  gboolean   ret = TRUE;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstElement **cur_vsink_p = &priv->vsink;

  new_vsink = gst_element_factory_make ("ismd_vidrend_bin", NULL);
  if (!new_vsink) {
    UMMS_DEBUG ("Failed to make ismd_vidrend_bin");
    ret = FALSE;
    goto OUT;
  }
  //sinking it 
  gst_object_ref_sink (new_vsink);
  UMMS_DEBUG ("new ismd_vidrend_bin: %p, name: %s", new_vsink, GST_ELEMENT_NAME(new_vsink));
  gst_object_replace ((GstObject **)cur_vsink_p, (GstObject *)new_vsink);

  if (rect)
    g_object_set (new_vsink, "rectangle", rect, NULL);

  if (plane != INVALID_PLANE_ID)
    g_object_set (new_vsink, "gdl-plane", plane, NULL);

OUT:
  if (new_vsink)
    gst_object_unref (new_vsink);
  return ret;
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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
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
    if (!XQueryTree(disp, cur_win, &root_win, &parent_win, &child_list,&num_children)) {
      UMMS_DEBUG ("Can't query window tree.");
      return 0;
    }

    UMMS_DEBUG ("cur_win(%lx), parent_win(%lx)", cur_win, parent_win);
    if (child_list) XFree((char *)child_list);

    if (!FROM_THE_SAME_PROC(cur_win, parent_win)){
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
  gchar *rect = NULL;
  GValue *val = NULL;
  GstElement *shmvbin = NULL;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstElement **cur_vsink_p = &priv->vsink;
  
  UMMS_DEBUG ("setting up datacopy target");
  shmvbin = gst_element_factory_make ("shmvidrendbin", NULL);
  if (!shmvbin) {
    UMMS_DEBUG ("Making \"shmvidrendbin\" failed");
    return FALSE;
  }
  gst_object_ref_sink (shmvbin);

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
  if (val) {
    rect = (gchar *)g_value_get_string (val);
    UMMS_DEBUG ("setting rectangle = '%s'", rect);
    g_object_set (shmvbin, "rectangle", rect, NULL);
  }

  gst_object_replace ((GstObject **)cur_vsink_p, (GstObject *)shmvbin);
  UMMS_DEBUG ("Set \"shmvidrendbin\" to dvbplayer");

  if (shmvbin)
    gst_object_unref (shmvbin);
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
static int xerror_handler (
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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  if (!priv->target_initialized)
    return TRUE;

  /*
   *For DataCopy and ReservedType0, nothing need to do here.
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
dvb_player_set_target (MeegoMediaPlayerControl *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  vsink_bin = priv->vsink;
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
  }

  return ret;
}

//both Xwindow and ReservedType0 target use ismd_vidrend_sink, so that need clock.
#define NEED_CLOCK(target_type) \
  ((target_type == XWindow || target_type == ReservedType0)?TRUE:FALSE)

#define REQUEST_RES(self, t, p, e_msg)                                        \
  do{                                                                         \
    DvbPlayerPrivate *priv = GET_PRIVATE (self);                              \
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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  if (priv->resource_prepared)
    return TRUE;
  
  //FIXME:For now, just request HW clock and Tuner resources. How to know we need HW decoder?
  REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
  REQUEST_RES(self, ResourceTypeTuner, INVALID_RES_HANDLE, "No tuner resource");

  priv->resource_prepared = TRUE;
  return TRUE;
}

static void
release_resource (MeegoMediaPlayerControl *self)
{
  GList *g;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

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
dvb_player_play (MeegoMediaPlayerControl *self)
{
    DvbPlayerPrivate *priv = GET_PRIVATE(self);

    if (request_resource(self)) {

      if (gst_element_set_state(priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        UMMS_DEBUG ("Set pipeline to paused failed");
        return FALSE;
      }

      //make dvbsrc to produce data
      if (gst_element_set_state(priv->source, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        UMMS_DEBUG ("Setting dvbsrc to playing failed");
        return FALSE;
      }
    }
    return TRUE;
}

static gboolean
_stop_pipe (MeegoMediaPlayerControl *control)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (control);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Unable to set NULL state");
    return FALSE;
  }

  release_resource (control);

  UMMS_DEBUG ("gstreamer engine stopped");
  return TRUE;
}

static gboolean
dvb_player_stop (MeegoMediaPlayerControl *self)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  PlayerState old_state;

  if (!_stop_pipe (self))
    return FALSE;

  old_state = priv->player_state;
  priv->player_state = PlayerStateStopped;

  if (old_state != priv->player_state) {
    meego_media_player_control_emit_player_state_changed (self, old_state, priv->player_state);
  }
  meego_media_player_control_emit_stopped (self);

  return TRUE;
}

static gboolean
dvb_player_set_video_size (MeegoMediaPlayerControl *self,
    guint x, guint y, guint w, guint h)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstElement *pipeline = priv->pipeline;
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipeline, FALSE);
  UMMS_DEBUG ("invoked");

  //We use ismd_vidrend_bin as video-sink, so we can set rectangle property.
  g_object_get (G_OBJECT(pipeline), "video-sink", &vsink_bin, NULL);
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
dvb_player_get_video_size (MeegoMediaPlayerControl *self,
    guint *w, guint *h)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstElement *pipeline = priv->pipeline;
  guint x[1], y[1];
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipeline, FALSE);
  UMMS_DEBUG ("invoked");
  g_object_get (G_OBJECT(pipeline), "video-sink", &vsink_bin, NULL);
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
dvb_player_is_seekable (MeegoMediaPlayerControl *self, gboolean *seekable)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gboolean res = FALSE;
  gint old_seekable;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (seekable != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);


  if (priv->uri == NULL)
    return FALSE;

  old_seekable = priv->seekable;

  if (priv->seekable == -1) {
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_TIME);
    if (gst_element_query (pipeline, query)) {
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
dvb_player_set_volume (MeegoMediaPlayerControl *self, gint vol)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  volume = CLAMP ((((gdouble)vol) / 100), 0.0, 1.0);
  UMMS_DEBUG ("set volume to = %f", volume);
  gst_stream_volume_set_volume (GST_STREAM_VOLUME (pipeline),
      GST_STREAM_VOLUME_FORMAT_CUBIC,
      volume);

  return TRUE;
}

static gboolean
dvb_player_get_volume (MeegoMediaPlayerControl *self, gint *volume)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  gdouble vol;

  *volume = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipeline),
        GST_STREAM_VOLUME_FORMAT_CUBIC);

  *volume = vol * 100;
  UMMS_DEBUG ("cur volume=%f(double), %d(int)", vol, *volume);

  return TRUE;
}

static gboolean
dvb_player_has_video (MeegoMediaPlayerControl *self, gboolean *has_video)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  gint n_video;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipeline), "n-video", &n_video, NULL);
  UMMS_DEBUG ("'%d' videos in stream", n_video);
  *has_video = (n_video > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
dvb_player_has_audio (MeegoMediaPlayerControl *self, gboolean *has_audio)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  gint n_audio;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipeline), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("'%d' audio tracks in stream", n_audio);
  *has_audio = (n_audio > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
dvb_player_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
  return TRUE;
}

static gboolean
dvb_player_is_streaming (MeegoMediaPlayerControl *self, gboolean *is_streaming)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  g_return_val_if_fail (priv->uri, FALSE);
  /*For now, we consider live source to be streaming source , hence unseekable.*/
  *is_streaming = priv->is_live;
  UMMS_DEBUG ("uri:'%s' is %s streaming source", priv->uri, (*is_streaming) ? "" : "not");
  return TRUE;
}

static gboolean
dvb_player_get_player_state (MeegoMediaPlayerControl *self,
    gint *state)
{
  DvbPlayerPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  *state = priv->player_state;
  return TRUE;
}

static gboolean 
dvb_player_get_current_audio (MeegoMediaPlayerControl *self, gint *cur_audio)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint c_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  g_object_get (G_OBJECT (pipeline), "current-audio", &c_audio, NULL);
  UMMS_DEBUG ("the current audio stream is %d", c_audio);

  *cur_audio = c_audio;

  return TRUE;
}

static gboolean 
dvb_player_set_current_video (MeegoMediaPlayerControl *self, gint cur_video)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *ele = NULL;
  gint n_video = -1;
  gchar *program_num = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  ele = priv->tsdemux;
  g_return_val_if_fail (GST_IS_ELEMENT (ele), FALSE);

  program_num = g_strdup_printf("%d", cur_video);
  UMMS_DEBUG ("set program-numbers begin, program-numbers = %s", program_num);
  //g_object_set (G_OBJECT (tsparse), "program-numbers", program_num, NULL);
  g_object_set (G_OBJECT (ele), "program-number", cur_video, NULL);
  g_free (program_num);
  UMMS_DEBUG ("set program-numbers end");

  return TRUE;
}

static gboolean
dvb_player_set_current_audio (MeegoMediaPlayerControl *self, gint cur_audio)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_audio ourselves.*/
  g_object_get (G_OBJECT (pipeline), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("The total audio numeber is %d, we want to set to %d", n_audio, cur_audio);
  if((cur_audio< 0) || (cur_audio >= n_audio)) {
    UMMS_DEBUG ("The audio we want to set is %d, invalid one.", cur_audio);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipeline), "current-audio", cur_audio, NULL);

  return TRUE;
}

static gboolean
dvb_player_get_video_num (MeegoMediaPlayerControl *self, gint *video_num)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint n_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  g_object_get (G_OBJECT (pipeline), "n-video", &n_video, NULL);
  UMMS_DEBUG ("the video number of the stream is %d", n_video);

  *video_num = n_video;

  return TRUE;
}

static gboolean
dvb_player_get_audio_num (MeegoMediaPlayerControl *self, gint *audio_num)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  g_object_get (G_OBJECT (pipeline), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d", n_audio);

  *audio_num = n_audio;

  return TRUE;
}

static gboolean
dvb_player_get_subtitle_num (MeegoMediaPlayerControl *self, gint *sub_num)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  g_object_get (G_OBJECT (pipeline), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("the subtitle number of the stream is %d", n_sub);

  *sub_num = n_sub;

  return TRUE;
}

static gboolean
dvb_player_get_current_subtitle (MeegoMediaPlayerControl *self, gint *cur_sub)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint c_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  g_object_get (G_OBJECT (pipeline), "current-text", &c_sub, NULL);
  UMMS_DEBUG ("the current subtitle stream is %d", c_sub);

  *cur_sub = c_sub;

  return TRUE;
}

static gboolean
dvb_player_set_current_subtitle (MeegoMediaPlayerControl *self, gint cur_sub)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_sub ourselves.*/
  g_object_get (G_OBJECT (pipeline), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("The total subtitle numeber is %d, we want to set to %d", n_sub, cur_sub);
  if((cur_sub < 0) || (cur_sub >= n_sub)) {
    UMMS_DEBUG ("The subtitle we want to set is %d, invalid one.", cur_sub);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipeline), "current-text", cur_sub, NULL);

  return TRUE;
}

static gboolean
dvb_player_set_mute (MeegoMediaPlayerControl *self, gint mute) 
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("set mute to = %d", mute);
  gst_stream_volume_set_mute (GST_STREAM_VOLUME (pipeline), mute);

  return TRUE;
}


static gboolean
dvb_player_is_mute (MeegoMediaPlayerControl *self, gint *mute) 
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  gboolean is_mute;

  *mute = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  is_mute = gst_stream_volume_get_mute (GST_STREAM_VOLUME (pipeline));
  UMMS_DEBUG("Get the mute %d", is_mute);
  *mute = is_mute;

  return TRUE;
}


static gboolean
dvb_player_set_scale_mode (MeegoMediaPlayerControl *self, gint scale_mode) 
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
  GstElement *vsink_bin;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */ 
  g_object_get (G_OBJECT(pipeline), "video-sink", &vsink_bin, NULL);
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
  if(vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}


static gboolean
dvb_player_get_scale_mode (MeegoMediaPlayerControl *self, gint *scale_mode)
{
  GstElement *pipeline;
  DvbPlayerPrivate *priv;
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
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */ 
  g_object_get (G_OBJECT(pipeline), "video-sink", &vsink_bin, NULL);
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
  if(vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}

static gboolean
dvb_player_get_video_codec (MeegoMediaPlayerControl *self, gint channel, gchar ** video_codec)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *video_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;

  g_object_get (G_OBJECT (pipeline), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-video-tags", channel, &tag_list);
  if(tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if(size = gst_tag_list_get_tag_size(tag_list, GST_TAG_VIDEO_CODEC) > 0) {
    gchar *st = NULL;        

    for (i = 0; i < size; ++i) {
      if(gst_tag_list_get_string_index (tag_list, GST_TAG_VIDEO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the video codec named: %s", channel, st);
        if(codec_name) {
          codec_name = g_strconcat(codec_name, st);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);  
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if(codec_name)
    *video_codec = codec_name;

  if(tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
dvb_player_get_audio_codec (MeegoMediaPlayerControl *self, gint channel, gchar ** audio_codec)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *audio_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;

  g_object_get (G_OBJECT (pipeline), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d", 
          tol_channel, channel);
  
  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-audio-tags", channel, &tag_list);
  if(tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if(size = gst_tag_list_get_tag_size(tag_list, GST_TAG_AUDIO_CODEC) > 0) {
    gchar *st = NULL;        

    for (i = 0; i < size; ++i) {
      if(gst_tag_list_get_string_index (tag_list, GST_TAG_AUDIO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the audio codec named: %s", channel, st);
        if(codec_name) {
          codec_name = g_strconcat(codec_name, st);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);  
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if(codec_name)
    *audio_codec = codec_name;

  if(tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}

static gboolean
dvb_player_get_video_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *video_rate)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  guint32 bit_rate = 0;

  *video_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;

  g_object_get (G_OBJECT (pipeline), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d", 
          tol_channel, channel);
  
  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-video-tags", channel, &tag_list);
  if(tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if(gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate/1000;
  } else if(gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate/1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if(tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
dvb_player_get_audio_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *audio_rate)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  guint32 bit_rate = 0;
  
  *audio_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;

  g_object_get (G_OBJECT (pipeline), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-audio-tags", channel, &tag_list);
  if(tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if(gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate/1000;
  } else if(gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate/1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if(tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
dvb_player_get_encapsulation(MeegoMediaPlayerControl *self, gchar ** encapsulation)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gchar *enca_name = NULL;

  *encapsulation = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;

  if(priv->tag_list) {
    gst_tag_list_get_string (priv->tag_list, GST_TAG_CONTAINER_FORMAT, &enca_name);
    if(enca_name) {
      UMMS_DEBUG("get the container name: %s", enca_name);
      *encapsulation = enca_name;
    } else {
      UMMS_DEBUG("no infomation about the container.");
    }
  }

  return TRUE;
}


static gboolean
dvb_player_get_audio_samplerate(MeegoMediaPlayerControl *self, gint channel, gint * sample_rate)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *sample_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipeline), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-audio-pad", channel, &pad);
  if(pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_int (s, "rate", sample_rate);
    gst_caps_unref (caps);
  }

  if(pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
dvb_player_get_video_framerate(MeegoMediaPlayerControl *self, gint channel, 
                               gint * frame_rate_num, gint * frame_rate_denom)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
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

  g_object_get (G_OBJECT (pipeline), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-video-pad", channel, &pad);
  if(pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "framerate", frame_rate_num, frame_rate_denom);
    gst_caps_unref (caps);
  }

  if(pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
dvb_player_get_video_resolution(MeegoMediaPlayerControl *self, gint channel, gint * width, gint * height)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;

  *width = 0;
  *height = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */

  g_object_get (G_OBJECT (pipeline), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-video-pad", channel, &pad);
  if(pad == NULL) {
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

  if(pad)
    gst_object_unref (pad);

  return TRUE;
}


static gboolean
dvb_player_get_video_aspect_ratio(MeegoMediaPlayerControl *self, gint channel, 
                                  gint * ratio_num, gint * ratio_denom)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
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

  g_object_get (G_OBJECT (pipeline), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d", 
          tol_channel, channel);

  if(channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipeline, "get-video-pad", channel, &pad);
  if(pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "pixel-aspect-ratio", ratio_num, ratio_denom);
    gst_caps_unref (caps);
  }

  if(pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
dvb_player_get_protocol_name(MeegoMediaPlayerControl *self, gchar ** prot_name)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gchar * uri = NULL;
  
  *prot_name = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  g_object_get (G_OBJECT (pipeline), "uri", &uri, NULL);
 
  if(!uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipeline);
    return FALSE;
  }
  
  if(!gst_uri_is_valid(uri)) {
    UMMS_DEBUG("uri: %s  is invalid", uri);
    g_free(uri);
    return FALSE;
  }

  UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri is %s", pipeline, uri);

  *prot_name = gst_uri_get_protocol(uri);
  UMMS_DEBUG("Get the protocol name is %s", *prot_name);
  
  g_free(uri);
  return TRUE;
}


static gboolean
dvb_player_get_current_uri(MeegoMediaPlayerControl *self, gchar ** uri)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gchar * s_uri = NULL;
  
  *uri = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_object_get (G_OBJECT (pipeline), "uri", &s_uri, NULL);
 
  if(!s_uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipeline);
    return FALSE;
  }

  if(!gst_uri_is_valid(s_uri)) {
    UMMS_DEBUG("uri: %s  is invalid", s_uri);
    g_free(s_uri);
    return FALSE;
  }

  *uri = s_uri;
  return TRUE;
}

static gboolean
dvb_player_get_title(MeegoMediaPlayerControl *self, gchar ** title)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *title = g_strdup (priv->title);
  UMMS_DEBUG ("title = %s", *title);

  return TRUE;
}

static gboolean
dvb_player_get_artist(MeegoMediaPlayerControl *self, gchar ** artist)
{
  DvbPlayerPrivate *priv = NULL;
  GstElement *pipeline = NULL;
  
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
      dvb_player_set_uri);
  meego_media_player_control_implement_set_target (klass,
      dvb_player_set_target);
  meego_media_player_control_implement_play (klass,
      dvb_player_play);
//  meego_media_player_control_implement_pause (klass,
//      dvb_player_pause);
  meego_media_player_control_implement_stop (klass,
      dvb_player_stop);
//  meego_media_player_control_implement_set_video_size (klass,
//      dvb_player_set_video_size);
//  meego_media_player_control_implement_get_video_size (klass,
//      dvb_player_get_video_size);
//  meego_media_player_control_implement_is_seekable (klass,
//      dvb_player_is_seekable);
//  meego_media_player_control_implement_set_position (klass,
//      dvb_player_set_position);
//  meego_media_player_control_implement_get_position (klass,
//      dvb_player_get_position);
//  meego_media_player_control_implement_set_playback_rate (klass,
//      dvb_player_set_playback_rate);
//  meego_media_player_control_implement_get_playback_rate (klass,
//      dvb_player_get_playback_rate);
//  meego_media_player_control_implement_set_volume (klass,
//      dvb_player_set_volume);
//  meego_media_player_control_implement_get_volume (klass,
//      dvb_player_get_volume);
//  meego_media_player_control_implement_get_media_size_time (klass,
//      dvb_player_get_media_size_time);
//  meego_media_player_control_implement_get_media_size_bytes (klass,
//      dvb_player_get_media_size_bytes);
//  meego_media_player_control_implement_has_video (klass,
//      dvb_player_has_video);
//  meego_media_player_control_implement_has_audio (klass,
//      dvb_player_has_audio);
//  meego_media_player_control_implement_support_fullscreen (klass,
//      dvb_player_support_fullscreen);
//  meego_media_player_control_implement_is_streaming (klass,
//      dvb_player_is_streaming);
  meego_media_player_control_implement_get_player_state (klass,
      dvb_player_get_player_state);
//  meego_media_player_control_implement_get_buffered_bytes (klass,
//      dvb_player_get_buffered_bytes);
//  meego_media_player_control_implement_get_buffered_time (klass,
//      dvb_player_get_buffered_time);
//  meego_media_player_control_implement_get_current_video (klass,
//      dvb_player_get_current_video);
//  meego_media_player_control_implement_get_current_audio (klass,
//      dvb_player_get_current_audio);
//  meego_media_player_control_implement_set_current_video (klass,
//      dvb_player_set_current_video);
//  meego_media_player_control_implement_set_current_audio (klass,
//      dvb_player_set_current_audio);
//  meego_media_player_control_implement_get_video_num (klass,
//      dvb_player_get_video_num);
//  meego_media_player_control_implement_get_audio_num (klass,
//      dvb_player_get_audio_num);
//  meego_media_player_control_implement_set_proxy (klass,
//      dvb_player_set_proxy);
//  meego_media_player_control_implement_set_subtitle_uri (klass,
//      dvb_player_set_subtitle_uri);
//  meego_media_player_control_implement_get_subtitle_num (klass,
//      dvb_player_get_subtitle_num);
//  meego_media_player_control_implement_set_current_subtitle (klass,
//      dvb_player_set_current_subtitle);
//  meego_media_player_control_implement_get_current_subtitle (klass,
//      dvb_player_get_current_subtitle);
//  meego_media_player_control_implement_set_buffer_depth (klass,
//      dvb_player_set_buffer_depth);
//  meego_media_player_control_implement_set_mute (klass,
//      dvb_player_set_mute);
//  meego_media_player_control_implement_is_mute (klass,
//      dvb_player_is_mute);
//  meego_media_player_control_implement_suspend (klass,
//      dvb_player_suspend);
//  meego_media_player_control_implement_restore (klass,
//      dvb_player_restore);
//  meego_media_player_control_implement_set_scale_mode (klass,
//      dvb_player_set_scale_mode);
//  meego_media_player_control_implement_get_scale_mode (klass,
//      dvb_player_get_scale_mode);      
//  meego_media_player_control_implement_get_video_codec (klass,
//      dvb_player_get_video_codec);
//  meego_media_player_control_implement_get_audio_codec (klass,
//      dvb_player_get_audio_codec);
//  meego_media_player_control_implement_get_video_bitrate (klass,
//      dvb_player_get_video_bitrate);
//  meego_media_player_control_implement_get_audio_bitrate (klass,
//      dvb_player_get_audio_bitrate);
//  meego_media_player_control_implement_get_encapsulation (klass,
//      dvb_player_get_encapsulation);
//  meego_media_player_control_implement_get_audio_samplerate (klass,
//      dvb_player_get_audio_samplerate);
//  meego_media_player_control_implement_get_video_framerate (klass,
//      dvb_player_get_video_framerate);
//  meego_media_player_control_implement_get_video_resolution (klass,
//      dvb_player_get_video_resolution);
//  meego_media_player_control_implement_get_video_aspect_ratio (klass,
//      dvb_player_get_video_aspect_ratio);
//  meego_media_player_control_implement_get_protocol_name (klass,
//      dvb_player_get_protocol_name);
//  meego_media_player_control_implement_get_current_uri (klass,
//      dvb_player_get_current_uri);
//  meego_media_player_control_implement_get_title (klass,
//      dvb_player_get_title);
//  meego_media_player_control_implement_get_artist (klass,
//      dvb_player_get_artist);
}

static void
dvb_player_get_property (GObject    *object,
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
dvb_player_set_property (GObject      *object,
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
dvb_player_dispose (GObject *object)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (object);

  UMMS_DEBUG ("Begin");
  dvb_player_stop ((MeegoMediaPlayerControl *)object);

  TEARDOWN_ELEMENT(priv->source);
  TEARDOWN_ELEMENT(priv->tsdemux);
  TEARDOWN_ELEMENT(priv->vsink);
  TEARDOWN_ELEMENT(priv->asink);
  TEARDOWN_ELEMENT(priv->pipeline);

  if (priv->target_type == XWindow) {
    unset_xwindow_target ((MeegoMediaPlayerControl *)object);
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL;
  }

  if(priv->tag_list) 
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;

  G_OBJECT_CLASS (dvb_player_parent_class)->dispose (object);
  UMMS_DEBUG ("End");
}

static void
dvb_player_finalize (GObject *object)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (object);

  RESET_STR(priv->uri);
  RESET_STR(priv->title);
  RESET_STR(priv->artist);

  G_OBJECT_CLASS (dvb_player_parent_class)->finalize (object);
}

static void
dvb_player_class_init (DvbPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DvbPlayerPrivate));

  object_class->get_property = dvb_player_get_property;
  object_class->set_property = dvb_player_set_property;
  object_class->dispose = dvb_player_dispose;
  object_class->finalize = dvb_player_finalize;
}

static void
bus_message_state_change_cb (GstBus     *bus,
    GstMessage *message,
    DvbPlayer  *self)
{

  GstState old_state, new_state;
  PlayerState old_player_state;
  gpointer src;
  gboolean seekable;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);

  old_player_state = priv->player_state;
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
  } else if(new_state == GST_STATE_PLAYING) {
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
bus_message_get_tag_cb (GstBus *bus, GstMessage *message, DvbPlayer  *self)
{
  gpointer src;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstPad * src_pad = NULL;
  GstTagList * tag_list = NULL;
  guint32 bit_rate;
  gchar * pad_name = NULL;
  gchar * element_name = NULL;
  gchar * title = NULL;
  gchar * artist = NULL;
  gboolean metadata_changed = FALSE;

  src = GST_MESSAGE_SRC (message);
 
  if(message->type != GST_MESSAGE_TAG) {
    UMMS_DEBUG("not a tag message in a registered tag signal, strange");
    return;
  }

  gst_message_parse_tag_full (message, &src_pad, &tag_list);
  if(src_pad) {
    pad_name = g_strdup (GST_PAD_NAME (src_pad));
    UMMS_DEBUG("The pad name is %s", pad_name);
  }

  if(message->src) {
    element_name = g_strdup (GST_ELEMENT_NAME (message->src));
    UMMS_DEBUG("The element name is %s", element_name);
  }

  priv->tag_list = 
      gst_tag_list_merge(priv->tag_list, tag_list, GST_TAG_MERGE_REPLACE);

  //cache the title
  if(gst_tag_list_get_string_index (tag_list, GST_TAG_TITLE, 0, &title)) {
    UMMS_DEBUG("Element: %s, provide the title: %s", element_name, title);
    RESET_STR(priv->title); 
    priv->title = title;
    metadata_changed = TRUE;
  }

  //cache the artist
  if(gst_tag_list_get_string_index (tag_list, GST_TAG_ARTIST, 0, &artist)) {
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
  if(size = gst_tag_list_get_tag_size(tag_list, GST_TAG_VIDEO_CODEC) > 0) {
    video_codec = g_strdup_printf("%s-->%s Video Codec: ",
            element_name? element_name: "NULL", pad_name? pad_name: "NULL");

    for (i = 0; i < size; ++i) {
      gchar *st = NULL;        

      if(gst_tag_list_get_string_index (tag_list, GST_TAG_VIDEO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Element: %s, Pad: %s provide the video codec named: %s", element_name, pad_name, st);
        video_codec = g_strconcat(video_codec, st);
        g_free (st);  
      }
    }

    /* store the name for later use. */
    g_strlcpy(priv->video_codec, video_codec, DVB_PLAYER_MAX_VIDEOCODEC_SIZE);
    UMMS_DEBUG("%s", video_codec);
  }

  if(size = gst_tag_list_get_tag_size(tag_list, GST_TAG_AUDIO_CODEC) > 0) {
    audio_codec = g_strdup_printf("%s-->%s Audio Codec: ",
            element_name? element_name: "NULL", pad_name? pad_name: "NULL");

    for (i = 0; i < size; ++i) {
      gchar *st = NULL;        

      if(gst_tag_list_get_string_index (tag_list, GST_TAG_AUDIO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Element: %s, Pad: %s provide the audio codec named: %s", element_name, pad_name, st);
        audio_codec = g_strconcat(audio_codec, st);
        g_free (st);  
      }
    }

    UMMS_DEBUG("%s", audio_codec);

    /* need to consider the multi-channel audio case. The demux and decoder
     * will both send this message. We prefer codec info from decoder now. Need to improve */
    if(element_name && (g_strstr_len(element_name, strlen(element_name), "demux") ||
                g_strstr_len(element_name, strlen(element_name), "Demux") ||
                g_strstr_len(element_name, strlen(element_name), "DEMUX"))) {
      if(priv->audio_codec_used < DVB_PLAYER_MAX_AUDIO_STREAM) {
        g_strlcpy(priv->audio_codec[priv->audio_codec_used], audio_codec, DVB_PLAYER_MAX_AUDIOCODEC_SIZE);
        priv->audio_codec_used++;
      } else {
        UMMS_DEBUG("audio_codec need to discard because too many steams");
        out_of_channel = 1;
      }
    } else {
      UMMS_DEBUG("audio_codec need to discard because it not come from demux");
    }
  }

  if(gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    /* Again, the bitrate info may come from demux and audio decoder, we use demux now. */
    UMMS_DEBUG("Element: %s, Pad: %s provide the bitrate: %d", element_name, pad_name, bit_rate);
    if(element_name && (g_strstr_len(element_name, strlen(element_name), "demux") ||
                g_strstr_len(element_name, strlen(element_name), "Demux") ||
                g_strstr_len(element_name, strlen(element_name), "DEMUX"))) {
      gchar * codec = NULL;
      int have_found = 0;

      /* first we check whether it is the bitrate of video. */
      if(video_codec) { /* the bitrate sent with the video codec, easy one. */
        have_found = 1;
        priv->video_bitrate = bit_rate;
        UMMS_DEBUG("we set the bitrate: %u for video", bit_rate);
      }

      if(!have_found) { /* try to compare the element and pad name. */
        codec = g_strdup_printf("%s-->%s Video Codec: ",
                element_name? element_name: "NULL", pad_name? pad_name: "NULL");
        if(g_strncasecmp(priv->video_codec, codec, strlen(codec))) {
          have_found = 1;
          priv->video_bitrate = bit_rate;
          UMMS_DEBUG("we set the bitrate: %u for video", bit_rate);
        }

        g_free(codec);
      }

      /* find it in the audio codec stream. */ 
      if(!have_found) {
        if(audio_codec) {
          /* the bitrate sent with the audio codec, easy one. */
          have_found = 1;
          if(!out_of_channel) {
            priv->audio_bitrate[priv->audio_codec_used -1] = bit_rate;
            UMMS_DEBUG("we set the bitrate: %u for audio stream: %d", bit_rate, priv->audio_codec_used -1);
          } else {
            UMMS_DEBUG("audio bitrate need to discard because too many steams");
          }
        }

        if(!have_found) {  /* last try, use audio element and pad to index. */
          codec = g_strdup_printf("%s-->%s Audio Codec: ",
                  element_name? element_name: "NULL", pad_name? pad_name: "NULL");

          for(i=0; i<priv->audio_codec_used; i++) {
            if(g_strncasecmp(priv->audio_codec[i], codec, strlen(codec)))
              break;
          }

          have_found = 1; /* if not find, we use audio channel as defaule, so always find. */
          if(i < priv->audio_codec_used) {
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

  if(video_codec)
    g_free(video_codec);
  if(audio_codec)
    g_free(audio_codec);
#endif

  if(src_pad)
    g_object_unref(src_pad);
  gst_tag_list_free (tag_list);
  if(pad_name)
    g_free(pad_name);
  if(element_name)
    g_free(element_name);

}

static void
bus_message_eos_cb (GstBus     *bus,
                    GstMessage *message,
                    DvbPlayer  *self)
{
  UMMS_DEBUG ("message::eos received on bus");
  meego_media_player_control_emit_eof (self);
}


static void
bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    DvbPlayer  *self)
{
  GError *error = NULL;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("message::error received on bus");

  gst_message_parse_error (message, &error, NULL);
  _stop_pipe (MEEGO_MEDIA_PLAYER_CONTROL(self));

  meego_media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("Error emitted with message = %s", error->message);

  g_clear_error (&error);
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus,
                  GstMessage *message,
                  DvbPlayer *engine)
{
  DvbPlayerPrivate *priv;
  GstElement *vsink;
  GError *err = NULL;
  GstMessage *msg = NULL;

  if ( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if ( ! gst_structure_has_name( message->structure, "prepare-gdl-plane" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (engine, GST_BUS_PASS);
  g_return_val_if_fail (DVB_IS_PLAYER (engine), GST_BUS_PASS);
  priv = GET_PRIVATE (engine);

  vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));
  UMMS_DEBUG ("sync-handler received on bus: prepare-gdl-plane, source: %s", GST_ELEMENT_NAME(vsink));

  if (!prepare_plane ((MeegoMediaPlayerControl *)engine)) {
    //Since we are in streame thread, let the vsink to post the error message. Handle it in bus_message_error_cb().
    err = g_error_new_literal (UMMS_RESOURCE_ERROR, UMMS_RESOURCE_ERROR_NO_RESOURCE, "Plane unavailable");
    msg = gst_message_new_error (GST_OBJECT_CAST(priv->pipeline), err, "No resource");
    gst_element_post_message (priv->pipeline, msg);
    g_error_free (err);
  }

//  meego_media_player_control_emit_request_window (engine);

  if (message)
    gst_message_unref (message);
  return( GST_BUS_DROP );
}

static void
video_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  DvbPlayer * priv = (DvbPlayer *) user_data;
  meego_media_player_control_emit_video_tag_changed(priv, stream_id);
}

static void
audio_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  DvbPlayer * priv = (DvbPlayer *) user_data;
  meego_media_player_control_emit_audio_tag_changed(priv, stream_id);
}

static void
text_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  DvbPlayer * priv = (DvbPlayer *) user_data;
  meego_media_player_control_emit_text_tag_changed(priv, stream_id);
}

static void
dvb_player_init (DvbPlayer *self)
{
  DvbPlayerPrivate *priv;
  GstBus *bus;
  GstCaps *caps;
  GstElement *pipeline = NULL;
  GstElement *source = NULL;
  GstElement *front_queue = NULL;
  GstElement *clock_provider = NULL;
  GstElement *tsdemux = NULL;
  GstElement *tsfilesink = NULL;
  GstElement *vsink = NULL;
  GstElement *asink = NULL;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->pipeline = pipeline = gst_pipeline_new ("dvb-player");
  if (!pipeline) {
    UMMS_DEBUG ("Creating pipeline elment failed");
    goto failed;
  }

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);

  g_signal_connect_object (bus, "message::error",
      G_CALLBACK (bus_message_error_cb),
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

//  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
//      self);

  gst_object_unref (GST_OBJECT (bus));

#define DVB_SRC
/*frontend pipeline: dvbsrc --> queue --> ismd_clock_recovery_provider --> flutsdemux*/
#ifdef DVB_SRC
  priv->source = source = gst_element_factory_make ("dvbsrc", "dvb-src");
  if (!source) {
    UMMS_DEBUG ("Creating dvbsrc failed");
    goto failed;
  }
  g_object_set (source, "modulation", 1,   "trans-mode", 0, \
      "bandwidth", 0,    "frequency", 578000000, \
      "code-rate-lp", 0, "code-rate-hp", 3, \
      "guard", 0, "hierarchy", 0, NULL);

  clock_provider = gst_element_factory_make ("ismd_clock_recovery_provider", "ismd-clock-recovery-provider");
  if (!clock_provider) {
    UMMS_DEBUG ("Creating ismd_clock_recovery_provider element failed");
    goto failed;
  }
#else
  priv->source = source = gst_element_factory_make ("filesrc", NULL);
  if (!source) {
    UMMS_DEBUG ("Creating filesrc failed");
    goto failed;
  }
  g_object_set (priv->source, "location", "/root/tvb.ts", NULL);

  GstClock *clock = get_hw_clock ();
  if (clock) {
    gst_pipeline_use_clock (GST_PIPELINE_CAST(pipeline), clock);
    g_object_unref (clock);
  } else { 
    UMMS_DEBUG ("Can't get HW clock");
    meego_media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, "Can't get HW clock for live source");
    goto failed;
  }
#endif
  front_queue   = gst_element_factory_make ("queue", "front-queue");
  priv->tsdemux = tsdemux = gst_element_factory_make ("flutsdemux", NULL);
  
  /*create default audio sink elements*/
  //FIXME: Consider the subtitle sink
  priv->asink = asink = gst_element_factory_make ("ismd_audio_sink", NULL);

  /*recode sink*/
  //priv->tsfilesink = gst_element_factory_make ("filesink", "tsfilesink");
  //g_object_set (priv->tsfilesink, "location", "/root/pvr.ts", NULL);

  if (!front_queue || !tsdemux || !asink) {
    UMMS_DEBUG ("front_queue(%p), tsdemux(%p), asink(%p)", \
                front_queue, tsdemux, asink);
    UMMS_DEBUG ("Creating element failed\n");
    goto failed;
  }

  //we need ref these element for later using.
  gst_object_ref_sink (source);
  gst_object_ref_sink (tsdemux);
  gst_object_ref_sink (asink);

  g_signal_connect (tsdemux, "pad-added", G_CALLBACK (pad_added_cb), self);
  g_signal_connect (tsdemux, "no-more-pads", G_CALLBACK (no_more_pads_cb), self);


  /* Add and link frontend elements*/
  gst_bin_add_many (GST_BIN (pipeline),source, front_queue, tsdemux, NULL);
#ifdef DVB_SRC
  gst_bin_add (GST_BIN (pipeline), clock_provider);
  gst_element_link_many (source, front_queue, clock_provider, tsdemux, NULL);
#else
  gst_element_link_many (source, front_queue, tsdemux, NULL);
#endif

  priv->player_state = PlayerStateNull;
  priv->res_mngr = umms_resource_manager_new ();
  priv->res_list = NULL;
  priv->tag_list = NULL;

  priv->seekable = -1;
  priv->resource_prepared = FALSE;
  priv->has_video = FALSE;
  priv->hw_viddec = 0;
  priv->has_audio = FALSE;
  priv->hw_auddec = FALSE;
  priv->title = NULL;
  priv->artist = NULL;

  //URI is one of metadatas whose change should be notified.
  meego_media_player_control_emit_metadata_changed (self);


  //Setup default target.
#define FULL_SCREEN_RECT "0,0,0,0"
  setup_ismd_vbin (MEEGO_MEDIA_PLAYER_CONTROL(self), FULL_SCREEN_RECT, UPP_A);
  priv->target_type = ReservedType0;
  priv->target_initialized = TRUE;
  return;

failed:
  TEARDOWN_ELEMENT(pipeline);
  TEARDOWN_ELEMENT(source);
  TEARDOWN_ELEMENT(front_queue);
  TEARDOWN_ELEMENT(clock_provider);
  TEARDOWN_ELEMENT(tsdemux);
  TEARDOWN_ELEMENT(vsink);
  TEARDOWN_ELEMENT(asink);
  return;
}

DvbPlayer *
dvb_player_new (void)
{
  return g_object_new (DVB_TYPE_PLAYER, NULL);
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
                                        &hw_vc1_static_caps};

static void
pad_added_cb (GstElement *element,
		GstPad     *pad,
		gpointer    data)
{
  GstPad *decoded_pad = NULL;
  GstPad *sinkpad = NULL;
  GstPad *srcpad  = NULL;
  DvbPlayer *player = (DvbPlayer *) data;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);
  gchar *name = gst_pad_get_name (pad);

  UMMS_DEBUG ("flutsdemux pad: %s added", name);
  if (!g_strcmp0 (name, "rawts")) {
    gst_bin_add (GST_BIN (priv->pipeline), priv->tsfilesink);
    sinkpad = gst_element_get_static_pad (priv->tsfilesink, "sink");
    gst_pad_link (pad, sinkpad);
    g_print ("pad(%s) linked\n", GST_PAD_NAME(pad));
    gst_element_set_state (priv->tsfilesink, GST_STATE_PLAYING);
  } else if (!g_strcmp0 (name, "program")) {
    gst_bin_add (GST_BIN (priv->pipeline), priv->tsfilesink);
    sinkpad = gst_element_get_static_pad (priv->tsfilesink, "sink");
    gst_pad_link (pad, sinkpad);
    g_print ("pad(%s) linked\n", GST_PAD_NAME(pad));
  }else {
    UMMS_DEBUG ("autoplug elementary stream pad: %s", name);
    if (!priv->mq) {
      priv->mq = gst_element_factory_make ("multiqueue", NULL);
      if (!priv->mq) {
        UMMS_DEBUG ("Making multiqueue failed");
        goto out;
      }
      gst_bin_add (GST_BIN (priv->pipeline), priv->mq);
      gst_element_set_state (priv->mq, GST_STATE_PAUSED);
    }

    if (!(srcpad = link_multiqueue (player, pad))) {
      UMMS_DEBUG ("Linking multiqueue failed");
      goto out;
    }

    if (g_str_has_prefix (name, "video")) {
      autoplug_pad (player, srcpad, CHAIN_TYPE_VIDEO);
    } else if (g_str_has_prefix (name, "audio")) {
      //autoplug_pad (player, srcpad, CHAIN_TYPE_AUDIO);
      link_sink (player, srcpad);
    } else {
      //FIXME: do we need to handle flustsdemux's subpicture/private pad? 
    }
  }

out:
  if (srcpad)
    gst_object_unref (srcpad);
  if (sinkpad)
    gst_object_unref (sinkpad);
  if (name)
    g_free (name);
}

static void
no_more_pads_cb (GstElement *element, gpointer data)
{
  DvbPlayer *player = (DvbPlayer *)data;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);
  GstStateChangeReturn ret;

  /*set pipeline to playing, so that sinks start to render data*/
  UMMS_DEBUG ("Set pipe to playing");
  if ((ret = gst_element_set_state (priv->pipeline, GST_STATE_PLAYING)) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Settint pipeline to playing failed");
  }
  return;
}

/* 
 * autoplug the pad with a decoder and proceed to link a sink
 */
static gboolean autoplug_pad(DvbPlayer *player, GstPad *pad, gint chain_type)
{
  GstCaps *caps = NULL;
  gboolean ret = TRUE;
  GList *compatible_elements = NULL;
  GList *walk = NULL;
  GList *chain_elements = NULL;//store the elements autoplugged from this pad.
  gboolean done = FALSE;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);

  UMMS_DEBUG ("autoplugging pad: %s, caps: ", GST_PAD_NAME (pad));
  caps = gst_pad_get_caps (pad);

  if (!caps || gst_caps_is_empty (caps)){
    UMMS_DEBUG ("Unknow caps");
    ret = FALSE;
    goto out;
  }

  if (!gst_caps_is_fixed (caps)) {
    UMMS_DEBUG ("caps is not fixed, strange");
    goto out;
  }

  update_elements_list (player);

  compatible_elements =
      gst_element_factory_list_filter (priv->elements, caps, GST_PAD_SINK,
      FALSE);
  if (!compatible_elements) {
    UMMS_DEBUG ("no compatible element available for this pad:%s, caps : %"GST_PTR_FORMAT, GST_PAD_NAME(pad), caps);
    ret = FALSE;
    goto out;
  }
  
  gst_plugin_feature_list_debug (compatible_elements);

  //make and connect element
  for (walk = compatible_elements; walk; walk= walk->next) {
    GstElementFactory *factory;
    GstElement *element;
    GstPad *sinkpad;
    const gchar *klass;
    gboolean link_ret;

    factory = GST_ELEMENT_FACTORY_CAST (walk->data);

    /* 
     * if it's a sink, we treat this pad as decoded pad,
     * and try to connnect this pad with our custom sink other than the auto-detected one
     */
    if (gst_element_factory_list_is_type (factory, GST_ELEMENT_FACTORY_TYPE_SINK)) {
      UMMS_DEBUG ("we found a \"decoded\" pad");

      klass = gst_element_factory_get_klass (factory);

      if (strstr (klass, "Audio") || strstr (klass, "Video")) {
        UMMS_DEBUG ("linking custom sink");
        link_ret = link_sink (player, pad);
      } else {
        /* unknown klass, skip this element */
        UMMS_DEBUG ("unknown sink klass %s found", klass);
        continue;
      }

      done = link_ret;
      break;
    }

    /*it is not sink, autoplug the dec element*/
    if ((element = gst_element_factory_create (factory, NULL)) == NULL) {
      UMMS_DEBUG ("Could not create an element from %s",
          gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)));
      continue;
    }

    if ((gst_element_set_state (element,
                GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("Couldn't set %s to READY",
          GST_ELEMENT_NAME (element));
      gst_object_unref (element);
      continue;
    }

    if (!(sinkpad = get_sink_pad (element))) {
      UMMS_DEBUG ("Element %s doesn't have a sink pad",
          GST_ELEMENT_NAME (element));
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (element);
      continue;
    }

    if (!(gst_bin_add (GST_BIN (priv->pipeline), element))) {
      UMMS_DEBUG ("Couldn't add %s to the bin",
          GST_ELEMENT_NAME (element));
      gst_object_unref (sinkpad);
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (element);
      continue;
    }

    if ((gst_pad_link (pad, sinkpad)) != GST_PAD_LINK_OK) {
      UMMS_DEBUG ("Link failed on pad %s:%s",
          GST_DEBUG_PAD_NAME (sinkpad));
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (sinkpad);
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }
    gst_object_unref (sinkpad);
    UMMS_DEBUG ("linked on pad %s:%s", GST_DEBUG_PAD_NAME (pad));

    if (!autoplug_dec_element (player, element)) {
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }

    /* Bring the element to the state of the parent */
    if ((gst_element_set_state (element,
                GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE) {

      UMMS_DEBUG ("Couldn't set %s to PAUSED",
          GST_ELEMENT_NAME (element));

      /* Remove this element and the downstream one we just added.*/
      gst_element_set_state (element, GST_STATE_NULL);
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }

    /*successful*/
    done = TRUE;
    break;
  }

  ret = done;

out:
  if (caps)
    gst_caps_unref (caps);

  if (compatible_elements)
    gst_plugin_feature_list_free (compatible_elements);
  return ret;
}

/*
 * link the pad to multiqueue and return corresponding src pad.
 * unref the returned pad after usage. 
 */
static GstPad *link_multiqueue (DvbPlayer *player, GstPad *pad)
{
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  GstIterator *it = NULL;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);

  if (!priv->mq) {
    UMMS_DEBUG ("No multiqueue");
    return NULL;
  }

  if (!(sinkpad = gst_element_get_request_pad (priv->mq, "sink%d"))) {
    UMMS_DEBUG ("Couldn't get sinkpad from multiqueue");
    return NULL;
  }

  if ((gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)) {
    UMMS_DEBUG ("Couldn't link demuxer and multiqueue");
    goto error;
  }

  it = gst_pad_iterate_internal_links (sinkpad);

  if (!it || (gst_iterator_next (it, (gpointer) & srcpad)) != GST_ITERATOR_OK
      || srcpad == NULL) {
    UMMS_DEBUG ("Couldn't get corresponding srcpad from multiqueue for sinkpad %" GST_PTR_FORMAT,
        sinkpad);
    goto error;
  }

out:
  if (sinkpad)
    gst_object_unref (sinkpad);

  if (it)
    gst_iterator_free (it);

  return srcpad;

error:
  gst_element_release_request_pad (priv->mq, sinkpad);
  goto out;
}

//decodable and sink elements
static void
update_elements_list (DvbPlayer *player)
{
  GList *res, *sinks;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);

  if (!priv->elements ||
      priv->elements_cookie !=
      gst_default_registry_get_feature_list_cookie ()) {
    if (priv->elements)
      gst_plugin_feature_list_free (priv->elements);
    res =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
    sinks =
        gst_element_factory_list_get_elements
        (GST_ELEMENT_FACTORY_TYPE_AUDIOVIDEO_SINKS, GST_RANK_MARGINAL);
    priv->elements = g_list_concat (res, sinks);
    priv->elements =
        g_list_sort (priv->elements, gst_plugin_feature_rank_compare_func);
    priv->elements_cookie = gst_default_registry_get_feature_list_cookie ();
  }
}

static GstPad *
get_sink_pad (GstElement * element)
{
  GstIterator *it;
  GstPad *pad = NULL;
  gpointer point;

  it = gst_element_iterate_sink_pads (element);

  if ((gst_iterator_next (it, &point)) == GST_ITERATOR_OK)
    pad = (GstPad *) point;

  gst_iterator_free (it);

  return pad;
}

static gboolean
autoplug_dec_element (DvbPlayer *player, GstElement * element)
{
  GList *pads;
  gboolean ret;

  UMMS_DEBUG ("Attempting to connect element %s further",
      GST_ELEMENT_NAME (element));

  for (pads = GST_ELEMENT_GET_CLASS (element)->padtemplates; pads;
      pads = g_list_next (pads)) {
    GstPadTemplate *templ = GST_PAD_TEMPLATE (pads->data);
    const gchar *templ_name;

    /* we are only interested in source pads */
    if (GST_PAD_TEMPLATE_DIRECTION (templ) != GST_PAD_SRC)
      continue;

    templ_name = GST_PAD_TEMPLATE_NAME_TEMPLATE (templ);
    UMMS_DEBUG ("got a source pad template %s", templ_name);

    /* We only care about always pad */
    switch (GST_PAD_TEMPLATE_PRESENCE (templ)) {
      case GST_PAD_ALWAYS:
      {
        /* get the pad that we need to autoplug */
        GstPad *pad = gst_element_get_static_pad (element, templ_name);
        if (!pad) {
          /* strange, pad is marked as always but it's not
           * there. Fix the element */
          UMMS_DEBUG ("could not get the pad for always template %s", templ_name);
          break;
        }

        UMMS_DEBUG ("got the pad for always template %s",
            templ_name);
        /* 
         * here is the pad, we need link it to sink. 
         * note that we just autoplug the first found always pad unless autoplugging failed.
         */
        ret = link_sink (player, pad);
        gst_object_unref (pad);

        if (ret) {
          return TRUE;
        }
        break;
      }
      case GST_PAD_SOMETIMES:
      case GST_PAD_REQUEST:
      {
        UMMS_DEBUG ("ignoring sometimes and request padtemplate %s", templ_name);
        break;
      }
    }
  }

  return FALSE;
}

static gboolean link_sink (DvbPlayer *player, GstPad *pad)
{
  DvbPlayerPrivate *priv; 
  GstStructure *s;
  const gchar *name;
  GstPad *sinkpad = NULL;
  GstCaps *caps   = NULL;
  gboolean ret = TRUE;

  g_return_if_fail (DVB_IS_PLAYER (player));
  g_return_if_fail (GST_IS_PAD (pad));

  priv = GET_PRIVATE (player);

  caps = gst_pad_get_caps_reffed (pad);
  UMMS_DEBUG ("pad:%s, caps : %"GST_PTR_FORMAT, GST_PAD_NAME(pad), caps);

  s = gst_caps_get_structure (caps, 0);

  if (s) {
    GstElement *sink = NULL;
    GstElement *queue = NULL;
    name = gst_structure_get_name (s);
    g_printf ("caps name : %s\n", name);

    if (g_str_has_prefix (name, "video")) {
      sink = priv->vsink;
      g_print ("Found video decoded pad: %p, linking it\n", pad);

    } else if(g_str_has_prefix (name, "audio")) {
      g_print ("Found audio decoded pad: %p, linking it\n", pad);
      sink = priv->asink;

    } else {
      g_print ("Ignore this pad\n");
    }

    if (sink) {
      if (!(queue = gst_element_factory_make ("queue", NULL))) {
        UMMS_DEBUG ("Creating queue failed");
        goto out;
      }

      gst_bin_add (GST_BIN (priv->pipeline), queue);
      gst_bin_add (GST_BIN (priv->pipeline), sink);
      if (!gst_element_link (queue, sink)) {
        UMMS_DEBUG ("linking queue and sink failed");
        gst_element_set_state (queue, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), queue);
        gst_element_set_state (sink, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), sink);
        ret = FALSE;
        goto out;
      }

      sinkpad = gst_element_get_static_pad (queue, "sink");
      if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {
        g_print ("link pad(%s): sinkpad(%s) failed\n", GST_PAD_NAME(pad), GST_PAD_NAME (sinkpad));
        gst_element_set_state (queue, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), queue);
        gst_element_set_state (sink, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), sink);
        ret = FALSE;
        goto out;
      }

      gst_element_set_state (queue, GST_STATE_PAUSED);
      gst_element_set_state (sink, GST_STATE_PAUSED);
      g_print ("pad: %p linked\n", pad);
    } 
  } else {
    g_print ("Getting caps structure failed\n");
    ret = FALSE;
    goto out;
  }
 
out:

  if (caps)
    gst_caps_unref (caps);

  if (sinkpad)
    gst_object_unref (sinkpad);

  return ret;
}
