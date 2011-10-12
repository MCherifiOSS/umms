#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include <gst/app/gstappsink.h>
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
//#define DVB_SRC

static void meego_media_player_control_init (MeegoMediaPlayerControl* iface);
static gpointer socket_listen_thread(DvbPlayer* dvd_player);
static void socket_thread_join(MeegoMediaPlayerControl* dvd_player);
static void send_socket_data(GstBuffer* buf, gpointer user_data);

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

#define SOCK_MAX_SERV_CONNECTS 5
#define SOCK_SOCKET_DEFAULT_PORT 0 // The port number will be get by socket create.
#define SOCK_SOCKET_DEFAULT_ADDR "127.0.0.1"


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

  //current program recording
  gboolean recording;
  gchar *file_location;
  GstPad *request_pad;

  //associated data channel
  gchar *ip;
  gint  port;
  /* Use for socket raw data transfer. */
  GMutex *socks_lock; // protect the sockets FD.
  int listen_fd;
  GThread *listen_thread;
  gint serv_fds[SOCK_MAX_SERV_CONNECTS];
  gint sock_exit_flag;
};

static gboolean _stop_pipe (MeegoMediaPlayerControl *control);
static gboolean dvb_player_set_video_size (MeegoMediaPlayerControl *self, guint x, guint y, guint w, guint h);
static gboolean create_xevent_handle_thread (MeegoMediaPlayerControl *self);
static gboolean destroy_xevent_handle_thread (MeegoMediaPlayerControl *self);
static void release_resource (MeegoMediaPlayerControl *self);
static void pad_added_cb (GstElement *element, GstPad *pad, gpointer data);
static void no_more_pads_cb (GstElement *element, gpointer data);
static gboolean autoplug_pad(DvbPlayer *player, GstPad *pad, gint chain_type);
static GstPad *link_multiqueue (DvbPlayer *player, GstPad *pad);
static void update_elements_list (DvbPlayer *player);
static gboolean autoplug_dec_element (DvbPlayer *player, GstElement * element);
static gboolean link_sink (DvbPlayer *player, GstPad *pad);
static GstPad *get_sink_pad (GstElement * element);
static gboolean start_recording (MeegoMediaPlayerControl *self, gchar *location);
static gboolean stop_recording (MeegoMediaPlayerControl *self);
gboolean set_ismd_audio_sink_property (GstElement *asink, const gchar *prop_name, gpointer val);
gboolean get_ismd_audio_sink_property (GstElement *asink, const gchar *prop_name, gpointer val);


enum {
  CHAIN_TYPE_VIDEO,
  CHAIN_TYPE_AUDIO,
  CHAIN_TYPE_SUB
};

#define DVBT_PARAMS_NUM 8
const gchar *dvbt_param_name[] = {
  "modulation",
  "trans-mode",
  "bandwidth",
  "frequency",
  "code-rate-lp",
  "code-rate-hp",
  "guard",
  "hierarchy"
};

guint dvbt_param_val[DVBT_PARAMS_NUM] = {0, };

/*
 * URI pattern:
 * dvb://?program-number=x&type=x&modulation=x&trans-mod=x&bandwidth=x&frequency=x&code-rate-lp=x&
 * code-rate-hp=x&guard=x&hierarchy=x
 */
enum dvb_type {
  DVB_T,
  DVB_C,
  DVB_S,
  DVB_TYPE_NUM
};

static gchar *dvb_type_name[] = {
  "DVB-T",
  "DVB-C",
  "DVB-S"
};

static gboolean
set_properties(DvbPlayer *player, const gchar *location)
{
  gint i;
  gboolean ret = FALSE;
  gchar **part = NULL;
  gchar **params = NULL;
  gchar *format = NULL;
  gint program_num;
  gint type;
  gboolean invalid_params = FALSE;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);

  g_return_val_if_fail (location, FALSE);
  UMMS_DEBUG ("location = '%s'", location);

  if (!priv->source || !priv->tsdemux)
    goto out;

  if (!(part = g_strsplit (location, "?", 0))) {
    UMMS_DEBUG ("splitting location failed");
    goto out;
  }

  if (!(params = g_strsplit (part[1], "&", 0))) {
    UMMS_DEBUG ("splitting params failed");
    goto out;
  }

  if ((0 == sscanf (params[1], "type=%d", &type))) {
    UMMS_DEBUG ("Invalid dvb type string: %s", params[1]);
    invalid_params = TRUE;
    goto out;
  }

  if (!(type >= 0 && type < DVB_TYPE_NUM)) {
    UMMS_DEBUG ("Invalid dvb type: %d", type);
    invalid_params = TRUE;
    goto out;
  }

  UMMS_DEBUG ("Setting %s properties", dvb_type_name[type]);

  //setting dvbsrc properties
  for (i = 0; i < DVBT_PARAMS_NUM; i++) {
    UMMS_DEBUG ("dvbt_param_name[%d]=%s", i, dvbt_param_name[i]);
    format = g_strconcat (dvbt_param_name[i], "=%u", NULL);
    sscanf (params[i+2], format, &dvbt_param_val[i]);
    g_free (format);
    g_object_set (priv->source, dvbt_param_name[i],  dvbt_param_val[i], NULL);
    UMMS_DEBUG ("Setting param: %s=%u", dvbt_param_name[i], dvbt_param_val[i]);
  }

  //setting program number
  sscanf (params[0], "program-number=%d", &program_num);
  g_object_set (priv->tsdemux, "program-number",  program_num, NULL);
  UMMS_DEBUG ("Setting program-number: %d", program_num);

  ret = TRUE;

out:
  if (part)
    g_strfreev (part);
  if (params)
    g_strfreev (params);

  if (invalid_params) {
    UMMS_DEBUG ("Incorrect params string");
  }

  if (!ret) {
    UMMS_DEBUG ("failed");
  }
  return ret;
}

static gboolean
dvb_player_parse_uri (DvbPlayer *player, const gchar *uri)
{
  gboolean ret = TRUE;
  gchar *protocol = NULL;
  gchar *location = NULL;

  g_return_val_if_fail (uri, FALSE);

  protocol = gst_uri_get_protocol (uri);

  if (strcmp (protocol, "dvb") != 0) {
    ret = FALSE;
  } else {
    location = gst_uri_get_location (uri);

    if (location != NULL) {
      ret = set_properties (player, location);
    } else
      ret = FALSE;
  }

  if (protocol)
    g_free (protocol);
  if (location)
    g_free (location);

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

#ifdef DVB_SRC
  return dvb_player_parse_uri (DVB_PLAYER (self), uri);
#else
  return TRUE;
#endif
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
  gint x, y, w, h, rx, ry;
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

  REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
  //FIXME: request the HW viddec resource according to added pad and its caps.
  REQUEST_RES(self, ResourceTypeHwViddec, INVALID_RES_HANDLE, "No HW viddec resource");
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
    if (gst_element_set_state(priv->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("Set pipeline to paused failed");
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean
_stop_pipe (MeegoMediaPlayerControl *control)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (control);

  if (priv->recording) {
    stop_recording (control);
  }

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
  GstElement *pipe = priv->pipeline;
  gboolean ret = FALSE;
  GstElement *vsink_bin = NULL;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");

  vsink_bin = priv->vsink;
  if (vsink_bin) {
    gchar *rectangle_des = NULL;

    UMMS_DEBUG ("video sink: %p, name: %s", vsink_bin, GST_ELEMENT_NAME(vsink_bin));
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "rectangle")) {
      rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
      UMMS_DEBUG ("set rectangle damension :'%s'", rectangle_des);
      g_object_set (G_OBJECT(vsink_bin), "rectangle", rectangle_des, NULL);
      g_free (rectangle_des);
      ret = TRUE;
    } else {
      UMMS_DEBUG ("video sink: %s has no 'rectangle' property",  GST_ELEMENT_NAME(vsink_bin));
    }
  } else {
    UMMS_DEBUG ("no video sink");
  }

  return ret;
}

static gboolean
dvb_player_get_video_size (MeegoMediaPlayerControl *self,
    guint *w, guint *h)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  guint x[1], y[1];
  GstElement *vsink_bin = NULL;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");

  vsink_bin = priv->vsink;
  if (vsink_bin) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "rectangle")) {
      gchar *rectangle_des = NULL;
      UMMS_DEBUG ("ismd_vidrend_bin: %p, name: %s", vsink_bin, GST_ELEMENT_NAME(vsink_bin));
      g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
      sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
      UMMS_DEBUG ("got rectangle damension :'%u,%u,%u,%u'", *x, *y, *w, *h);
      return TRUE;
    } else {
      UMMS_DEBUG ("video sink: %s has no 'rectangle' property",  GST_ELEMENT_NAME(vsink_bin));
      return FALSE;
    }
  } else {
    UMMS_DEBUG ("no video sink");
    return FALSE;
  }
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
  DvbPlayerPrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  UMMS_DEBUG ("invoked");

  volume = CLAMP ((((double)(vol*8)) / 100), 0.0, 8.0);
  UMMS_DEBUG ("umms volume = %d, ismd volume  = %f", vol, volume);
  return set_ismd_audio_sink_property (priv->asink, "volume", &volume);
}

static gboolean
dvb_player_get_volume (MeegoMediaPlayerControl *self, gint *vol)
{
  gdouble volume;
  DvbPlayerPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  UMMS_DEBUG ("invoked");
  if (!get_ismd_audio_sink_property (priv->asink, "volume", &volume)) {
    return FALSE;
  } else {
    *vol = volume*100/8;
    UMMS_DEBUG ("ismd volume=%f, umms volume=%d", volume, *vol);
    return TRUE;
  }
}

static gboolean
dvb_player_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
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
  return set_ismd_audio_sink_property (priv->asink, "mute", &mute);
}

static gboolean
dvb_player_is_mute (MeegoMediaPlayerControl *self, gint *mute)
{
  DvbPlayerPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  return get_ismd_audio_sink_property (priv->asink, "mute", mute);
}


static gboolean
dvb_player_set_scale_mode (MeegoMediaPlayerControl *self, gint scale_mode)
{
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

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  vsink_bin = priv->vsink;
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
  return ret;
}

static gboolean
dvb_player_get_scale_mode (MeegoMediaPlayerControl *self, gint *scale_mode)
{
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

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  vsink_bin = priv->vsink;
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
dvb_player_get_protocol_name(MeegoMediaPlayerControl *self, gchar ** prot_name)
{
  *prot_name = "dvb";
  return TRUE;
}

static gboolean
dvb_player_get_current_uri(MeegoMediaPlayerControl *self, gchar ** uri)
{
  DvbPlayerPrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *uri = priv->uri;
  return TRUE;
}

static gboolean start_recording (MeegoMediaPlayerControl *self, gchar *location)
{

  gboolean ret = FALSE;
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  GstElement *filesink = NULL;
  gchar *file_location = NULL;

  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  if ((priv->player_state != PlayerStatePlaying) || priv->tsfilesink || !priv->tsdemux) {
    goto out;
  }

  if (!(filesink = gst_element_factory_make ("filesink", NULL))) {
    UMMS_DEBUG ("Creating filesink failed");
    goto out;
  }
  gst_object_ref_sink (filesink);

#define DEFAULT_FILE_LOCATION "/tmp/record.ts"
  if (location && location[0] != '\0')
    file_location = location;
  else
    file_location = DEFAULT_FILE_LOCATION;
  UMMS_DEBUG ("location: '%s'", file_location);
  g_object_set (filesink, "location", file_location, NULL);

  gst_bin_add (GST_BIN(priv->pipeline), filesink);

  if (!(sinkpad = gst_element_get_static_pad (filesink, "sink"))) {
    UMMS_DEBUG ("Getting program pad failed");
    goto failed;
  }

  if (!(srcpad = gst_element_get_request_pad (priv->tsdemux, "program"))) {
    UMMS_DEBUG ("Getting program pad failed");
    goto failed;
  }


  if (GST_PAD_LINK_OK != gst_pad_link (srcpad, sinkpad)) {
    UMMS_DEBUG ("Linking filesink failed");
    goto failed;
  }

  if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (filesink, GST_STATE_PLAYING)) {
    UMMS_DEBUG ("Setting filesink to playing failed");
    goto failed;
  }


  ret = TRUE;
  priv->recording = TRUE;
  priv->tsfilesink = filesink;
  priv->request_pad = srcpad;

out:
  if (sinkpad)
    gst_object_unref (sinkpad);

  if (!ret) {
    UMMS_DEBUG ("failed!!!");
  }

  return ret;

failed:
  gst_bin_remove (GST_BIN(priv->pipeline), filesink);
  TEARDOWN_ELEMENT (filesink);
  goto out;
}

static gboolean stop_recording (MeegoMediaPlayerControl *self)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");

  if (!priv->tsfilesink) {
    UMMS_DEBUG ("We don't have filesink, can't remove it");
    return FALSE;
  }

  gst_bin_remove (GST_BIN(priv->pipeline), priv->tsfilesink);
  TEARDOWN_ELEMENT (priv->tsfilesink);

  gst_element_release_request_pad (priv->tsdemux, priv->request_pad);
  gst_object_unref (priv->request_pad);
  priv->request_pad = NULL;

  priv->recording = FALSE;
  UMMS_DEBUG ("End");
  return TRUE;
}

static gboolean
dvb_player_record (MeegoMediaPlayerControl *self, gboolean to_record, gchar *location)
{
  DvbPlayerPrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  if (to_record == priv->recording) {
    return TRUE;
  }

  if (to_record)
    return start_recording (self, location);
  else
    return stop_recording (self);
}

static gboolean
dvb_player_get_pat (MeegoMediaPlayerControl *self, GPtrArray **pat)
{
  GValueArray *pat_info = NULL;
  GPtrArray *pat_out = NULL;
  DvbPlayerPrivate *priv = NULL;
  gboolean ret = FALSE;
  gint i;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (pat, FALSE);

  priv = GET_PRIVATE (self);

  if (!priv->tsdemux)
    goto out;

  pat_out = g_ptr_array_new ();
  if (!pat_out) {
    goto out;
  }

  g_object_get (priv->tsdemux, "pat-info", &pat_info, NULL);

  if (!pat_info) {
    UMMS_DEBUG ("no pat-info");
    goto out;
  }


  for (i = 0; i < pat_info->n_values; i++) {
    GValue *val = NULL;
    GObject *entry = NULL;
    GHashTable *ht = NULL;
    GValue *val_out = NULL;
    guint program_num = 0;
    guint pid = 0;


    //get program-number and pid
    val = g_value_array_get_nth (pat_info, i);
    entry = g_value_get_object (val);
    g_object_get (entry, "program-number", &program_num, "pid", &pid, NULL);
    UMMS_DEBUG ("program-number : %u, pid : %u", program_num, pid);

    //fill the output
    ht = g_hash_table_new (NULL, NULL);
    val_out = g_new0(GValue, 1);
    g_value_init (val_out, G_TYPE_UINT);
    g_value_set_uint (val_out, program_num);
    g_hash_table_insert (ht, "program-number", val_out);

    val_out = g_new0(GValue, 1);
    g_value_init (val_out, G_TYPE_UINT);
    g_value_set_uint (val_out, pid);
    g_hash_table_insert (ht, "pid", val_out);

    g_ptr_array_add (pat_out, ht);
  }
  ret = TRUE;
  *pat = pat_out;

out:
  if (!ret && pat_out) {
    g_ptr_array_free (pat_out, FALSE);
  }

  if (pat_info)
    g_value_array_free (pat_info);

  return ret;
}

/*
 * pmt() {
 *    program-number
 *    pcr-pid
 *    for (i = 0; i<elementary_stream_number; i++) {
 *      stream-info() {
 *        pid
 *        stream_type
 *      }
 *    }
 * }
 *
 */
static gboolean
dvb_player_get_pmt (MeegoMediaPlayerControl *self, guint *program_num, guint *pcr_pid, GPtrArray **stream_info)
{
  DvbPlayerPrivate *priv;
  gboolean ret = FALSE;
  GHashTable *stream_info_out;
  gint i;

  GObject *pmt_info;
  GValueArray *stream_info_array = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (program_num, FALSE);
  g_return_val_if_fail (pcr_pid, FALSE);
  g_return_val_if_fail (stream_info, FALSE);

  priv = GET_PRIVATE (self);

  if (!priv->tsdemux)
    goto out;

  g_object_get (priv->tsdemux, "pmt-info", &pmt_info, NULL);
  if (!pmt_info)
    goto out;

  //get all the pmt specific info we need
  g_object_get (pmt_info, "program-number", program_num, "pcr-pid", pcr_pid, "stream-info", &stream_info_array,  NULL);
  if (!stream_info_array) {
    UMMS_DEBUG ("getting stream-info failed");
    goto out;
  }

  //elementary stream info
  *stream_info = g_ptr_array_new ();
  for (i = 0; i < stream_info_array->n_values; i++) {
    guint pid;
    guint stream_type;
    GValue *val_tmp;
    GObject *stream_info_tmp;

    val_tmp = g_value_array_get_nth (stream_info_array, i);
    stream_info_tmp = g_value_get_object (val_tmp);
    g_object_get (stream_info_tmp, "pid", &pid, "stream-type", &stream_type, NULL);

    stream_info_out = g_hash_table_new (NULL, NULL);
    val_tmp = g_new0 (GValue, 1);
    g_value_init (val_tmp, G_TYPE_UINT);
    g_value_set_uint (val_tmp, pid);
    g_hash_table_insert (stream_info_out, "pid", val_tmp);
    val_tmp = g_new0 (GValue, 1);
    g_value_init (val_tmp, G_TYPE_UINT);
    g_value_set_uint (val_tmp, stream_type);
    g_hash_table_insert (stream_info_out, "stream-type", val_tmp);

    g_ptr_array_add (*stream_info, stream_info_out);
  }

  ret =  TRUE;

out:
  if (pmt_info)
    g_object_unref (pmt_info);
  if (stream_info_array)
    g_value_array_free (stream_info_array);
  return ret;
}

static gboolean
dvb_player_get_associated_data_channel (MeegoMediaPlayerControl *self, gchar **ip, gint *port)
{
  DvbPlayerPrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (ip, FALSE);
  g_return_val_if_fail (port, FALSE);

  priv = GET_PRIVATE (self);

  if (priv->ip) {
    *ip = g_strdup(priv->ip);
  } else {
    *ip = NULL;
  }

  *port = priv->port;
  return TRUE;
}

GstFlowReturn new_buffer_cb (GstAppSink *sink, gpointer user_data)
{
  GstBuffer *buf = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  buf = gst_app_sink_pull_buffer (sink);
  if (!buf) {
    UMMS_DEBUG ("pulling buffer failed");
    ret = GST_FLOW_ERROR;
    goto out;
  }

  //Send the buf data from socket.
  send_socket_data(buf, user_data);

out:
  if (buf)
    gst_buffer_unref (buf);
  return ret;
}

static gboolean
connect_appsink (DvbPlayer *self)
{
  GstElement *appsink;
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  gboolean ret = FALSE;
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  if (!priv->tsdemux) {
    goto out;
  }

  if (!(appsink = gst_element_factory_make ("appsink", NULL))) {
    UMMS_DEBUG ("Creating appsink failed");
    goto out;
  }

  GstAppSinkCallbacks *callbacks = g_new0 (GstAppSinkCallbacks, 1);
  callbacks->new_buffer = new_buffer_cb;
  gst_app_sink_set_callbacks (GST_APP_SINK_CAST (appsink), callbacks, self, NULL);
  g_free (callbacks);

  gst_bin_add (GST_BIN(priv->pipeline), appsink);

  if (!(sinkpad = gst_element_get_static_pad (appsink, "sink"))) {
    UMMS_DEBUG ("Getting program pad failed");
    goto failed;
  }

  g_object_set (priv->tsdemux, "pids", INIT_PIDS, NULL);
  if (!(srcpad = gst_element_get_request_pad (priv->tsdemux, "rawts"))) {
    UMMS_DEBUG ("Getting rawts pad failed");
    goto failed;
  }


  if (GST_PAD_LINK_OK != gst_pad_link (srcpad, sinkpad)) {
    UMMS_DEBUG ("Linking appsink failed");
    goto failed;
  }

  ret = TRUE;

out:
  if (sinkpad)
    gst_object_unref (sinkpad);
  if (srcpad)
    gst_object_unref (srcpad);

  if (!ret) {
    UMMS_DEBUG ("failed!!!");
  }

  return ret;

failed:
  gst_bin_remove (GST_BIN(priv->pipeline), appsink);
  goto out;
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
  meego_media_player_control_implement_stop (klass,
      dvb_player_stop);
  meego_media_player_control_implement_set_video_size (klass,
      dvb_player_set_video_size);
  meego_media_player_control_implement_get_video_size (klass,
      dvb_player_get_video_size);
  meego_media_player_control_implement_is_seekable (klass,
      dvb_player_is_seekable);
  meego_media_player_control_implement_set_volume (klass,
      dvb_player_set_volume);
  meego_media_player_control_implement_get_volume (klass,
      dvb_player_get_volume);
  meego_media_player_control_implement_support_fullscreen (klass,
      dvb_player_support_fullscreen);
  meego_media_player_control_implement_get_player_state (klass,
      dvb_player_get_player_state);
  meego_media_player_control_implement_set_mute (klass,
      dvb_player_set_mute);
  meego_media_player_control_implement_is_mute (klass,
      dvb_player_is_mute);
  meego_media_player_control_implement_set_scale_mode (klass,
      dvb_player_set_scale_mode);
  meego_media_player_control_implement_get_scale_mode (klass,
      dvb_player_get_scale_mode);
  meego_media_player_control_implement_get_protocol_name (klass,
      dvb_player_get_protocol_name);
  meego_media_player_control_implement_get_current_uri (klass,
      dvb_player_get_current_uri);
  meego_media_player_control_implement_record (klass,
      dvb_player_record);
  meego_media_player_control_implement_get_pat (klass,
      dvb_player_get_pat);
  meego_media_player_control_implement_get_pmt (klass,
      dvb_player_get_pmt);
  meego_media_player_control_implement_get_associated_data_channel (klass,
      dvb_player_get_associated_data_channel);
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
  int i;

  UMMS_DEBUG ("Begin");
  dvb_player_stop ((MeegoMediaPlayerControl *)object);

  TEARDOWN_ELEMENT(priv->source);
  TEARDOWN_ELEMENT(priv->tsdemux);
  TEARDOWN_ELEMENT(priv->vsink);
  TEARDOWN_ELEMENT(priv->asink);
  TEARDOWN_ELEMENT(priv->pipeline);

  if (priv->listen_thread) {
    socket_thread_join((MeegoMediaPlayerControl *)object);
    priv->listen_thread = NULL;
  }

  for (i = 0; i < SOCK_MAX_SERV_CONNECTS; i++) {
    if (priv->serv_fds[i] != -1) {
      close(priv->serv_fds[i]);
    }
    priv->serv_fds[i] = -1;
  }

  if (priv->socks_lock) {
    g_mutex_free (priv->socks_lock);
    priv->socks_lock = NULL;
  }

  if (priv->ip && priv->ip != SOCK_SOCKET_DEFAULT_ADDR) {
    g_free(priv->ip);
    priv->ip = NULL;
  }

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
  DvbPlayerPrivate *priv = GET_PRIVATE (self);

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);

  old_player_state = priv->player_state;
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
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
dvb_player_init (DvbPlayer *self)
{
  DvbPlayerPrivate *priv;
  GstBus *bus;
  GstElement *pipeline = NULL;
  GstElement *source = NULL;
  GstElement *front_queue = NULL;
  GstElement *clock_provider = NULL;
  GstElement *tsdemux = NULL;
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

  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
      self);

  gst_object_unref (GST_OBJECT (bus));

  /*frontend pipeline: dvbsrc --> queue --> ismd_clock_recovery_provider --> flutsdemux*/
#ifdef DVB_SRC
  priv->source = source = gst_element_factory_make ("dvbsrc", "dvb-src");
  if (!source) {
    UMMS_DEBUG ("Creating dvbsrc failed");
    goto failed;
  }

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
  gst_bin_add_many (GST_BIN (pipeline), source, front_queue, tsdemux, NULL);
#ifdef DVB_SRC
  gst_bin_add (GST_BIN (pipeline), clock_provider);
  gst_element_link_many (source, front_queue, clock_provider, tsdemux, NULL);
#else
  gst_element_link_many (source, front_queue, tsdemux, NULL);
#endif

  connect_appsink (self);

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


  //Create socket thread
  priv->socks_lock = g_mutex_new ();
  priv->sock_exit_flag = 0;
  priv->port = SOCK_SOCKET_DEFAULT_PORT;
  priv->ip = SOCK_SOCKET_DEFAULT_ADDR;
  priv->listen_thread = g_thread_create ((GThreadFunc) socket_listen_thread, self, TRUE, NULL);
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
    &hw_vc1_static_caps
                                                      };

static void
pad_added_cb (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad = NULL;
  GstPad *srcpad  = NULL;
  DvbPlayer *player = (DvbPlayer *) data;
  DvbPlayerPrivate *priv = GET_PRIVATE (player);
  gchar *name = gst_pad_get_name (pad);

  UMMS_DEBUG ("flutsdemux pad: %s added", name);
  if (!g_strcmp0 (name, "rawts")) {
    g_print ("pad(%s) added\n", GST_PAD_NAME(pad));
  } else if (!g_strcmp0 (name, "program")) {
    g_print ("pad(%s) added\n", GST_PAD_NAME(pad));
  } else {
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

  if (!caps || gst_caps_is_empty (caps)) {
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
  for (walk = compatible_elements; walk; walk = walk->next) {
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
      case GST_PAD_ALWAYS: {
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
      case GST_PAD_REQUEST: {
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

  g_return_val_if_fail (DVB_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (GST_IS_PAD (pad), FALSE);

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

    } else if (g_str_has_prefix (name, "audio")) {
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


static void
socket_thread_join(MeegoMediaPlayerControl* dvd_player)
{
  DvbPlayerPrivate *priv = GET_PRIVATE (dvd_player);
  struct sockaddr_in server_addr;

  g_mutex_lock (priv->socks_lock);
  priv->sock_exit_flag = 1;

  if (priv->listen_fd != -1) { /* need to wakeup the listen thread. */
    struct hostent *host = gethostbyname("localhost");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd != -1) {
      bzero(&server_addr, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(priv->port);
      server_addr.sin_addr = *((struct in_addr*)host->h_addr);

      UMMS_DEBUG("try to wakeup the thread by connect");
      g_mutex_unlock (priv->socks_lock);
      connect (fd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr));
      close(fd);
    } else {
      g_mutex_unlock (priv->socks_lock);
      UMMS_DEBUG("socket create failed, can not wakeup the listen thread");
    } 
  } else {
    g_mutex_unlock (priv->socks_lock);//Just Unlock it.
  }

  g_thread_join(priv->listen_thread);
  UMMS_DEBUG("listen_thread has joined");
}


static void
send_socket_data(GstBuffer* buf, gpointer user_data)
{
  int i = 0;
  int write_num = -1;
  DvbPlayerPrivate *priv = GET_PRIVATE (user_data);

  g_mutex_lock (priv->socks_lock);
  if (priv->sock_exit_flag) {
    g_mutex_unlock (priv->socks_lock);
    UMMS_DEBUG("Skip the work because exit.");
    return;
  }

  /* Now write the data. */
  for (i = 0; i < SOCK_MAX_SERV_CONNECTS; i++) {
    if (priv->serv_fds[i] != -1) {
      UMMS_DEBUG("Send the data for i:%d, fd:%d, data size is %d",
                 i, priv->serv_fds[i], GST_BUFFER_SIZE(buf));

      /* Do not send the signal because the socket closed.
       * Use MSG_NOSIGNAL flag */
      write_num = send(priv->serv_fds[i], GST_BUFFER_DATA(buf), GST_BUFFER_SIZE(buf), MSG_NOSIGNAL);

      if (write_num == -1) {
        UMMS_DEBUG("write data failed, because %s", strerror(errno));
        close(priv->serv_fds[i]);
        priv->serv_fds[i] = -1; /* mark invalid and not use again. */
      } else if (write_num > 0 && write_num !=
                 GST_BUFFER_SIZE(buf)) { // write some bytes but not the whole because we set NO_BLOCK.

        /* TODO: We need to handle the data size problem here. Because
         * the funciton is called in signal context and can not block,
         * we has set the fd to NO_BLOCK and the write may failed
         * because the data is to big. We may need to ref the buffer and
         * send it from the current offset next time. */
        UMMS_DEBUG("The socket just write part of the data, notice!!!");
      } else {
        UMMS_DEBUG("write data %d bytes to socket fd:%d.", GST_BUFFER_SIZE(buf), priv->serv_fds[i]);
      }
    }
  }

  g_mutex_unlock (priv->socks_lock);
}


static gpointer
socket_listen_thread(DvbPlayer* dvd_player)
{
  struct sockaddr_in cli_addr;
  struct sockaddr_in serv_addr;
  static int new_fd = -1;
  socklen_t cli_len;
  socklen_t serv_len;
  int i = 0;
  DvbPlayerPrivate *priv = GET_PRIVATE (dvd_player);

  g_mutex_lock (priv->socks_lock);
  for (i = 0; i < SOCK_MAX_SERV_CONNECTS; i++) {
    if (priv->serv_fds[i] != -1) {
      close(priv->serv_fds[i]);
    }
    priv->serv_fds[i] = -1;
  }
  
  priv->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (priv->listen_fd < 0) {
    UMMS_DEBUG("The listen socket create failed!");    
    g_mutex_unlock (priv->socks_lock);
    return NULL;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(priv->port);
  if (bind(priv->listen_fd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)) == -1) {
    UMMS_DEBUG("try to binding to %s:%d Failed, error is %s",
               priv->ip, priv->port, strerror(errno));
    close(priv->listen_fd);
    priv->listen_fd = -1;    
    g_mutex_unlock (priv->socks_lock);
    return NULL;
  }

  serv_len = sizeof(struct sockaddr);
  if (getsockname(priv->listen_fd, (struct sockaddr *)&serv_addr, &serv_len) != 0) {
    UMMS_DEBUG("We can not get the port of listen_fd");
    close(priv->listen_fd);
    priv->listen_fd = -1;    
    g_mutex_unlock (priv->socks_lock);
    return NULL;
  }
  
  UMMS_DEBUG("we now binding to %s:%d", inet_ntoa(serv_addr.sin_addr), serv_addr.sin_port);
  priv->port = serv_addr.sin_port;
  g_mutex_unlock (priv->socks_lock);

  if (listen(priv->listen_fd, 5) == -1) {
    UMMS_DEBUG("Listen Failed, error us %s", strerror(errno));
    close(priv->listen_fd);
    priv->listen_fd = -1;    
    return NULL;
  }

  while (1) {
    cli_len = sizeof(cli_addr);
    new_fd = accept(priv->listen_fd, (struct sockaddr*)(&cli_addr), &cli_len);
    if (new_fd < 0) {
      UMMS_DEBUG("A invalid accept call, errno is %s", strerror(errno));
      break;
    } else {
      UMMS_DEBUG("We get a connect request from %s:%d",
                 inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
    }

    g_mutex_lock (priv->socks_lock);
    for (i = 0; i < SOCK_MAX_SERV_CONNECTS; i++) {
      if (priv->serv_fds[i] == -1)
        break;
    }

    if (i < SOCK_MAX_SERV_CONNECTS) {
      priv->serv_fds[i] = new_fd;
      fcntl(priv->serv_fds[i], F_SETFL, O_NONBLOCK);
    } else {
      UMMS_DEBUG("The connection is too much, can not serve you, sorry");
      close(new_fd);
    }

    /* To check whether the main thread want us to exit. */
    if (priv->sock_exit_flag) {
      UMMS_DEBUG("Main thread told listen thread exit, BYE!");
      g_mutex_unlock (priv->socks_lock);
      break;
    }
    g_mutex_unlock (priv->socks_lock);
  }

  if (priv->listen_fd != -1) {
    close(priv->listen_fd);
    priv->listen_fd = -1;
  }
  return NULL;
}


gboolean 
set_ismd_audio_sink_property (GstElement *asink, const gchar *prop_name, gpointer val)
{
  g_return_val_if_fail (asink, FALSE);

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS(asink), prop_name)) {
    UMMS_DEBUG ("No property \"%s\"", prop_name);
    return FALSE;
  }

  if (!g_strcmp0(prop_name, "volume")) {
    g_object_set (asink, prop_name, *(gdouble *)val, NULL);
  } else if (!g_strcmp0(prop_name, "mute")) {
    g_object_set (asink, prop_name, *(gboolean *)val, NULL);
  } else {
    UMMS_DEBUG ("no care about prop \"%s\"", prop_name);
    return FALSE;
  }
  return TRUE;
}

gboolean 
get_ismd_audio_sink_property (GstElement *asink, const gchar *prop_name, gpointer val)
{
  g_return_val_if_fail (asink, FALSE);

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS(asink), prop_name)) {
    UMMS_DEBUG ("No property \"%s\"", prop_name);
    return FALSE;
  }

  if (!g_strcmp0(prop_name, "volume")) {
    g_object_get (asink, prop_name, (gdouble *)val, NULL);
  } else if (!g_strcmp0(prop_name, "mute")) {
    g_object_get (asink, prop_name, (gboolean *)val, NULL);
  } else {
    UMMS_DEBUG ("no care about prop \"%s\"", prop_name);
    return FALSE;
  }
  return TRUE;
}
