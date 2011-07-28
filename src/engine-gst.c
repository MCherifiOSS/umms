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

static const gchar *gst_state[] = {
  "GST_STATE_VOID_PENDING",
  "GST_STATE_NULL",
  "GST_STATE_READY",
  "GST_STATE_PAUSED",
  "GST_STATE_PLAYING"
};


/* list of URIs that we consider to be live source. */
static gchar *live_src_uri[] = { "http://", "mms://", "mmsh://", "rtsp://",
    "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://",
    NULL};

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
  GstElement *vsink;

  //buffering stuff
  gboolean buffering;
  gint buffer_percent;

  //UMMS defined player state
  PlayerState player_state;//current state
  PlayerState pending_state;//target state, for async state change(*==>PlayerStatePaused, PlayerStateNull/Stopped==>PlayerStatePlaying)

  gboolean is_live;
  gint64 duration;//ms
  gint64 total_bytes;

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
  UmmsResourceManager *res_mngr;
};

static void _reset_engine (MeegoMediaPlayerControl *self);
static gboolean _query_buffering_percent (GstElement *pipe, gdouble *percent);
static void _source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data);
static gboolean _stop_pipe (MeegoMediaPlayerControl *control);
static gboolean engine_gst_set_video_size (MeegoMediaPlayerControl *self, guint x, guint y, guint w, guint h);
static gboolean create_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean destroy_xevent_handle_thread (MeegoMediaPlayerControl *self);


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

  //_reset_engine (self);

  priv->uri = g_strdup (uri);

  g_object_set (priv->pipeline, "uri", uri, NULL);

  priv->seekable = -1;
  priv->is_live = IS_LIVE_URI(priv->uri);

  return TRUE;
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
  gint x,y,w,h,rx,ry;
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
  if (plane != -1)
    g_object_set (vsink, "gdl-plane", plane, NULL);

  g_object_set (priv->pipeline, "video-sink", vsink, NULL);
  UMMS_DEBUG ("Set ismd_vidrend_bin to playbin2");
  return TRUE;
}


static gboolean setup_gdl_plane_target (MeegoMediaPlayerControl *self, GHashTable *params)
{
  gchar *rect = NULL;
  gint  plane = -1;
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

/* 
 * 1. Retrive "video-window-id" and "top-window-id".
 * 2. Make videosink element and set its rectangle property according to video window geometry.
 * 3  Cutout video window geometry according to its relative position to application win.
 * 4. Create xevent handle thread.
 */

static gboolean setup_xwindow_target (MeegoMediaPlayerControl *self, GHashTable *params)
{
  GstElement *vsink;
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
  vsink = gst_element_factory_make ("ismd_vidrend_bin", NULL);
  if (!vsink) {
    UMMS_DEBUG ("Failed to make ismd_vidrend_bin");
    return FALSE;
  }

  gchar *rectangle_des = NULL;
  rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
  UMMS_DEBUG ("set rectangle damension :'%s'\n", rectangle_des);
  g_object_set (G_OBJECT(vsink), "rectangle", rectangle_des, NULL);
  g_free (rectangle_des);

  UMMS_DEBUG ("set ismd_vidrend_bin to playbin2");
  g_object_set (priv->pipeline, "video-sink", vsink, NULL);

  //Monitor top-level window's structure change event.
  XSelectInput (priv->disp, priv->app_win_id,
      StructureNotifyMask);
  create_xevent_handle_thread (self);

  priv->target_type = XWindow;
  priv->xwin_initialized = TRUE;

  return TRUE;
}

static gboolean
engine_gst_set_target (MeegoMediaPlayerControl *self, gint type, GHashTable *params)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->pipeline) {
    UMMS_DEBUG ("Engine not loaded, reason may be SetUri not invoked");
    return FALSE;
  }

  switch (type) {
    case XWindow:
      setup_xwindow_target (self, params);
      break;
    case DataCopy:
      UMMS_DEBUG ("DataCopy target");
      break;
    case Socket:
      UMMS_DEBUG ("unsupported target type: Socket");
      return FALSE;
      break;
    case ReservedType0:
      /*
       * ReservedType0 represents the gdl-plane video output in our UMMS implementation.
       */
      setup_gdl_plane_target (self, params);
      break;
    default:
      break;
  }
  priv->target_type = type;

  return TRUE;
}


static gboolean
engine_gst_play (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri) {
    UMMS_DEBUG(" Unable to set playing state - no URI set");
    return FALSE;
  }

  if(IS_LIVE_URI(priv->uri)) {     
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
  }

  priv->pending_state = PlayerStatePlaying;
  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

  return TRUE;
}

static gboolean
engine_gst_pause (MeegoMediaPlayerControl *self)
{
  GstStateChangeReturn ret;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (gst_element_set_state(priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("set pipeline to paused failed");
    return FALSE;
  }

  priv->pending_state = PlayerStatePaused;
  UMMS_DEBUG ("%s: called", __FUNCTION__);
  return TRUE;
}

static void
_reset_engine (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (priv->uri) {
    _stop_pipe(self);
    g_free (priv->uri);
    priv->uri = NULL;
  }

  if (priv->source) {
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  priv->total_bytes = -1;
  priv->duration = -1;
  priv->seekable = -1;
  priv->buffering = FALSE;
  priv->buffer_percent = -1;
  priv->player_state = PlayerStateStopped;
  priv->pending_state = PlayerStateNull;

  return;
}


static gboolean
_stop_pipe (MeegoMediaPlayerControl *control)
{
  EngineGstPrivate *priv = GET_PRIVATE (control);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG (" Unable to set NULL state");
    return FALSE;
  }


  UMMS_DEBUG ("gstreamer engine stopped");
  return TRUE;
}

static gboolean
engine_gst_stop (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!_stop_pipe (self))
    return FALSE;

  priv->player_state = PlayerStateStopped;
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
  UMMS_DEBUG ("%s:element name='%s'\n", __FUNCTION__, ele_name);

  //ugly solution, check by element metadata will be better
  ele_name = gst_element_get_name (element);
  if(g_strrstr(ele_name, "ismdgstvidrendbin") != NULL || g_strrstr(ele_name, "ismd_vidrend_bin") != NULL)
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
    GST_INFO ("found ismd_vidrend_bin.\n" );
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
  UMMS_DEBUG ("%s: invoked", __FUNCTION__);

  //We use ismd_vidrend_bin as video-sink, so we can set rectangle property.
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;

    rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("set rectangle damension :'%s'\n", rectangle_des);
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
  UMMS_DEBUG ("%s: invoked", __FUNCTION__);
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;
    g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
    sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("got rectangle damension :'%u,%u,%u,%u'\n", *x, *y, *w, *h);
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

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("Seeking to %" GST_TIME_FORMAT, GST_TIME_ARGS (in_pos * GST_MSECOND));

  gst_element_seek (pipe, 1.0,
                    GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, in_pos * GST_MSECOND,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  meego_media_player_control_emit_seeked (self);
  return TRUE;
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

  fmt = GST_FORMAT_TIME;
  if (gst_element_query_position (pipe, &fmt, &cur)) {
    UMMS_DEBUG ("current position = %lld (ms)", cur / GST_MSECOND);
  } else {
    UMMS_DEBUG ("Failed to query position");
  }

  gst_element_seek (pipe, in_rate,
                    GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, cur,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  return TRUE;
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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);
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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  volume = CLAMP ((((gdouble)vol) / 100), 0.0, 1.0);
  UMMS_DEBUG ("%s:set volume to = %f", __FUNCTION__, volume);
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

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipe),
        GST_STREAM_VOLUME_FORMAT_CUBIC);

  *volume = vol * 100;
  UMMS_DEBUG ("%s:cur volume=%f(double), %d(int)", __FUNCTION__, vol, *volume);

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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  if (gst_element_query_duration (pipe, &fmt, &duration)) {
    *media_size_time = duration / GST_MSECOND;
    UMMS_DEBUG ("%s:media size = %lld (ms)", __FUNCTION__, *media_size_time);
  } else {
    UMMS_DEBUG ("%s: query media_size_time failed", __FUNCTION__);
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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("%s: '%d' videos in stream", __FUNCTION__, n_video);
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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("%s: '%d' audio tracks in stream", __FUNCTION__, n_audio);
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

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_return_val_if_fail (priv->uri, FALSE);
  /*For now, we consider live source to be streaming source , hence unseekable.*/
  *is_streaming = priv->is_live;
  UMMS_DEBUG ("%s:uri:'%s' is %s streaming source", __FUNCTION__, priv->uri, (*is_streaming) ? "" : "not");
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
    UMMS_DEBUG ("%s: failed", __FUNCTION__);
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
engine_gst_set_window_id (MeegoMediaPlayerControl *self, gdouble window_id)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri || !priv->vsink || !GST_IS_X_OVERLAY(priv->vsink)) {
    UMMS_DEBUG(" Unable to set window id");
    return FALSE;
  }

  UMMS_DEBUG ("SetWindowId called, id = %d\n",
              (gint)window_id);

  gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (priv->vsink), (gint) window_id );
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
  UMMS_DEBUG ("%s: the current video stream is %d\n", __FUNCTION__, c_video);

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
  UMMS_DEBUG ("%s: the current audio stream is %d\n", __FUNCTION__, c_audio);

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
  UMMS_DEBUG ("%s: The total video numeber is %d, we want to set to %d\n",
          __FUNCTION__, n_video, cur_video);
  if((cur_video < 0) || (cur_video >= n_video)) {
    UMMS_DEBUG ("%s: The video we want to set is %d, invalid one.\n",
            __FUNCTION__, cur_video);
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
  UMMS_DEBUG ("%s: The total audio numeber is %d, we want to set to %d\n",
          __FUNCTION__, n_audio, cur_audio);
  if((cur_audio< 0) || (cur_audio >= n_audio)) {
    UMMS_DEBUG ("%s: The audio we want to set is %d, invalid one.\n",
            __FUNCTION__, cur_audio);
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
  UMMS_DEBUG ("%s: the video number of the stream is %d\n", __FUNCTION__, n_video);

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
  UMMS_DEBUG ("%s: the audio number of the stream is %d\n", __FUNCTION__, n_audio);

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
    UMMS_DEBUG ("%s: succeed to make the ismd_subrend_bin, set it to playbin2\n", __FUNCTION__);
    g_object_set (priv->pipeline, "text-sink", sub_sink, NULL);
  } else {
    UMMS_DEBUG ("%s: Unable to make the ismd_subrend_bin\n", __FUNCTION__);
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
  UMMS_DEBUG ("%s: the subtitle number of the stream is %d\n", __FUNCTION__, n_sub);

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
  UMMS_DEBUG ("%s: the current subtitle stream is %d\n", __FUNCTION__, c_sub);

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
  UMMS_DEBUG ("%s: The total subtitle numeber is %d, we want to set to %d\n",
          __FUNCTION__, n_sub, cur_sub);
  if((cur_sub < 0) || (cur_sub >= n_sub)) {
    UMMS_DEBUG ("%s: The subtitle we want to set is %d, invalid one.\n",
            __FUNCTION__, cur_sub);
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
  meego_media_player_control_implement_set_window_id (klass,
      engine_gst_set_window_id);
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
  if (priv->source) {
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  if (priv->pipeline) {
    g_object_unref (priv->pipeline);
    priv->pipeline = NULL;
  }

  if (priv->target_type == XWindow) {
    unset_xwindow_target ((MeegoMediaPlayerControl *)object);
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL;
  }

  G_OBJECT_CLASS (engine_gst_parent_class)->dispose (object);
}

static void
engine_gst_finalize (GObject *object)
{
  EngineGstPrivate *priv = GET_PRIVATE (object);

  RESET_STR(priv->uri);
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
  EngineGstPrivate *priv = GET_PRIVATE (self);

  GstState old_state, new_state;
  PlayerState old_player_state;
  gpointer src;

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

  if (priv->player_state != old_player_state) {
    UMMS_DEBUG ("emit state changed, old=%d, new=%d", old_player_state, priv->player_state);
    meego_media_player_control_emit_player_state_changed (self, priv->player_state);
  }
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

  if( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if( ! gst_structure_has_name( message->structure, "prepare-xwindow-id" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (engine, GST_BUS_DROP);
  g_return_val_if_fail (ENGINE_IS_GST (engine), GST_BUS_DROP);
  priv = GET_PRIVATE (engine);

  UMMS_DEBUG ("sync-handler received on bus: prepare-xwindow-id");
  priv->vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));

  meego_media_player_control_emit_request_window (engine);

  return( GST_BUS_DROP );
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


  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
      self);

  gst_object_unref (GST_OBJECT (bus));

  /*
   *Use GST_PLAY_FLAG_DOWNLOAD flag to enable Gstreamer Download buffer mode,
   *so that we can query the buffered time/bytes, and further the time rangs.
   */

  g_object_get (priv->pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_DOWNLOAD;
  g_object_set (priv->pipeline, "flags", flags, NULL);

  priv->player_state = PlayerStateNull;
  priv->target_type = ReservedType0;
  priv->res_mngr = umms_resource_manager_new ();

#define FULL_SCREEN_RECT "0,0,0,0"
  setup_ismd_vbin (MEEGO_MEDIA_PLAYER_CONTROL(self), FULL_SCREEN_RECT, UPP_A);
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
