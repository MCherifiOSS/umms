/*
 * UMMS (Unified Multi Media Service) provides a set of DBus APIs to support
 * playing Audio and Video as well as DVB playback.
 *
 * Authored by Zhiwen Wu <zhiwen.wu@intel.com>
 *             Junyan He <junyan.he@intel.com>
 * Copyright (c) 2011 Intel Corp.
 *
 * UMMS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * UMMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with UMMS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
#include "dvb-player-control-tv.h"
#include "media-player-control.h"
#include "param-table.h"


G_DEFINE_TYPE (DvbPlayerControlTv, dvb_player_control_tv, DVB_PLAYER_CONTROL_TYPE_BASE);

#define GET_PRIVATE(o) ((DvbPlayerControlBase *)o)->priv

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBasePrivate))

static void release_resource (DvbPlayerControlBase *self);
static GstClock *get_hw_clock(void);
static gboolean setup_ismd_vbin(MediaPlayerControl *self, gchar *rect, gint plane);
static gboolean cutout (DvbPlayerControlBase *self, gint x, gint y, gint w, gint h);



static void autoplug_multiqueue (DvbPlayerControlBase *player,GstPad *srcpad,gchar *name)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(player);

  if (g_str_has_prefix (name, "video")) {
    kclass->autoplug_pad (player, srcpad, CHAIN_TYPE_VIDEO);
  } else if (g_str_has_prefix (name, "audio")) {
    kclass->link_sink (player, srcpad);
  } else {
    //FIXME: do we need to handle flustsdemux's subpicture/private pad?
  }
  return;
}

static gboolean
destroy_xevent_handle_thread (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (priv->event_thread) {
    priv->event_thread_running = FALSE;
    g_thread_join (priv->event_thread);
    priv->event_thread = NULL;
  }
  UMMS_DEBUG ("End");
  return TRUE;
}

static gboolean
unset_xwindow_target (DvbPlayerControlBase *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if (!priv->xwin_initialized)
    return TRUE;

  destroy_xevent_handle_thread (self);
  priv->xwin_initialized = FALSE;
  return TRUE;
}

static gboolean
unset_target (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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

static gboolean setup_gdl_plane_target (MediaPlayerControl *self, GHashTable *params)
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

static gboolean setup_datacopy_target (MediaPlayerControl *self, GHashTable *params)
{
  gchar *rect = NULL;
  GValue *val = NULL;
  GstElement *shmvbin = NULL;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
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
int xerror_handler (
Display *dpy,
XErrorEvent *event)
{
  return x_print_error (dpy, event, stderr);
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
static Window get_top_level_win (DvbPlayerControlBase *self, Window sub_win)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
  Display *disp = priv->disp;
  Window root_win, parent_win, cur_win;
  unsigned int num_children;
  Window *child_list = NULL;
  Window top_win = 0;
  gboolean done = FALSE;

  if (!disp)
    return 0;

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
  return top_win;
}


static gboolean
get_video_rectangle (MediaPlayerControl *self, gint *ax, gint *ay, gint *w, gint *h, gint *rx, gint *ry)
{
  XWindowAttributes video_win_attr, app_win_attr;
  gint app_x, app_y;
  Window junkwin;
  Status status;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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
set_video_size (DvbPlayerControlBase *self,
    guint x, guint y, guint w, guint h)

{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
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
get_video_size (DvbPlayerControlBase *self, guint *w, guint *h)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
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

static void
player_control_tv_handle_xevents (MediaPlayerControl *control)
{
  XEvent e;
  gint x, y, w, h, rx, ry;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (control);

  g_return_if_fail (control);

  /* Handle Expose */
  while (XCheckWindowEvent (priv->disp,
         priv->app_win_id, StructureNotifyMask, &e)) {
    switch (e.type) {
      case ConfigureNotify:
        get_video_rectangle (control, &x, &y, &w, &h, &rx, &ry);
        cutout (control, rx, ry, w, h);

        set_video_size (control, x, y, w, h);
        UMMS_DEBUG ("Got ConfigureNotify, video window abs_x=%d,abs_y=%d,w=%d,y=%d", x, y, w, h);
        break;
      default:
        break;
    }
  }
}


static gpointer
player_control_tv_event_thread (MediaPlayerControl* control)
{

  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ("Begin");
  while (priv->event_thread_running) {

    if (priv->app_win_id) {
      player_control_tv_handle_xevents (control);
    }
    /* FIXME: do we want to align this with the framerate or anything else? */
    g_usleep (G_USEC_PER_SEC / 20);

  }

  UMMS_DEBUG ("End");
  return NULL;
}


static gboolean
create_xevent_handle_thread (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (!priv->event_thread) {
    /* Setup our event listening thread */
    UMMS_DEBUG ("run xevent thread");
    priv->event_thread_running = TRUE;
    priv->event_thread = g_thread_create (
          (GThreadFunc) player_control_tv_event_thread, self, TRUE, NULL);
  }
  return TRUE;
}

static gboolean
cutout (DvbPlayerControlBase *self, gint x, gint y, gint w, gint h)
{
  Atom property;
  gchar data[256];
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  property = XInternAtom (priv->disp, "_MUTTER_HINTS", 0);
  if (!property) {
    UMMS_DEBUG ("XInternAtom failed");
    return FALSE;
  }

  g_sprintf (data, "tv-cutout-x=%d:tv-cutout-y=%d:tv-cutout-width=%d:"
             "tv-cutout-height=%d:tv-half-trans=0:tv-full-window=0", x, y, w, h);

  UMMS_DEBUG ("Hints to mtv-mutter = \"%s\"", data);

  XChangeProperty(priv->disp, priv->app_win_id, property, XA_STRING, 8, PropModeReplace,
                  (unsigned char *)data, strlen(data));
  return TRUE;
}

/*
 * 1. Calculate top-level window according to video window.
 * 2. Cutout video window geometry according to its relative position to top-level window.
 * 3. Setup underlying ismd_vidrend_bin element.
 * 4. Create xevent handle thread.
 */

static gboolean setup_xwindow_target (DvbPlayerControlBase *self, GHashTable *params)
{
  GValue *val;
  gint x, y, w, h, rx, ry;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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
set_target (DvbPlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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


#define REQUEST_RES(self, t, p, e_msg)                                        \
  do{                                                                         \
    DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);                              \
    ResourceRequest req = {0,};                                               \
    Resource *res = NULL;                                                     \
    req.type = t;                                                             \
    req.preference = p;                                                       \
    res = umms_resource_manager_request_resource (priv->res_mngr, &req);      \
    if (!res) {                                                               \
      release_resource(self);                                                 \
      media_player_control_emit_error (self,                            \
                                             UMMS_RESOURCE_ERROR_NO_RESOURCE, \
                                             e_msg);                          \
      return FALSE;                                                           \
    }                                                                         \
    priv->res_list = g_list_append (priv->res_list, res);                     \
    }while(0)


static gboolean
request_resource (DvbPlayerControlBase *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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
release_resource (DvbPlayerControlBase *self)
{
  GList *g;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  for (g = priv->res_list; g; g = g->next) {
    Resource *res = (Resource *) (g->data);
    umms_resource_manager_release_resource (priv->res_mngr, res);
    UMMS_DEBUG("TV forloop");
  }
  g_list_free (priv->res_list);
  priv->res_list = NULL;
  priv->resource_prepared = FALSE;

  return;
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
set_volume (DvbPlayerControlBase *self, gint vol)
{
  DvbPlayerControlBasePrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  UMMS_DEBUG ("invoked");

  volume = CLAMP ((((double)(vol*8)) / 100), 0.0, 8.0);
  UMMS_DEBUG ("umms volume = %d, ismd volume  = %f", vol, volume);
  return set_ismd_audio_sink_property (priv->asink, "volume", &volume);
}

static gboolean
get_volume (DvbPlayerControlBase *self, gint *vol)
{
  gdouble volume;
  DvbPlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

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
set_mute (DvbPlayerControlBase *self, gint mute)
{
  GstElement *pipeline;
  DvbPlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("set mute to = %d", mute);
  return set_ismd_audio_sink_property (priv->asink, "mute", &mute);
}

static gboolean
is_mute (DvbPlayerControlBase *self, gint *mute)
{
  DvbPlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  return get_ismd_audio_sink_property (priv->asink, "mute", mute);
}

static gboolean
set_scale_mode (DvbPlayerControlBase *self, gint scale_mode)
{
  DvbPlayerControlBasePrivate *priv;
  GstElement *vsink_bin;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

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
get_scale_mode (DvbPlayerControlBase *self, gint *scale_mode)
{
  DvbPlayerControlBasePrivate *priv;
  GstElement *vsink_bin;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;

  *scale_mode = ScaleModeInvalid;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

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

static void
dvb_player_control_tv_get_property (GObject    *object,
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
dvb_player_control_tv_set_property (GObject      *object,
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
dvb_player_control_tv_dispose (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (dvb_player_control_tv_parent_class)->dispose (object);

}

static void
dvb_player_control_tv_finalize (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (dvb_player_control_tv_parent_class)->finalize (object);

}


static gboolean setup_ismd_vbin(MediaPlayerControl *self, gchar *rect, gint plane)
{
  GstElement *new_vsink = NULL;
  gboolean   ret = TRUE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
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


static gpointer
socket_listen_thread(DvbPlayerControlBase* dvd_player)
{
  struct sockaddr_in cli_addr;
  struct sockaddr_in serv_addr;
  static int new_fd = -1;
  socklen_t cli_len;
  socklen_t serv_len;
  int i = 0;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (dvd_player);

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
  serv_addr.sin_addr.s_addr = inet_addr(priv->ip);
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
  
  UMMS_DEBUG("we now binding to %s:%d", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
  priv->port = ntohs(serv_addr.sin_port);
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


static void
send_socket_data(GstBuffer* buf, gpointer user_data)
{
  int i = 0;
  int write_num = -1;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (user_data);

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
connect_appsink (DvbPlayerControlBase *self)
{
  GstElement *appsink;
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  gboolean ret = FALSE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

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

  /*
   *If using async pause, when no buffer to appsink or the first buffer appears long after elementary stream buffers,
   *appsink never transit to PASUED or PLAYING which make pipeline unable to PLAYING. So we disable "async"
   */
  g_object_set (G_OBJECT (appsink), "async", FALSE, NULL);

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
factory_init (GstElement *pipeline, DvbPlayerControlBase *self)
{
  DvbPlayerControlBasePrivate *priv;
  GstElement *source = NULL;
  GstElement *front_queue = NULL;
  GstElement *clock_provider = NULL;
  GstElement *tsdemux = NULL;
  GstElement *vsink = NULL;
  GstElement *asink = NULL;
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

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
    media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, "Can't get HW clock for live source");
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

  g_signal_connect (tsdemux, "pad-added", G_CALLBACK (kclass->pad_added_cb), self);
  g_signal_connect (tsdemux, "no-more-pads", G_CALLBACK (kclass->no_more_pads_cb), self);

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
  media_player_control_emit_metadata_changed (self);


  //Setup default target.
#define FULL_SCREEN_RECT "0,0,0,0"
  setup_ismd_vbin (MEDIA_PLAYER_CONTROL(self), FULL_SCREEN_RECT, UPP_A);
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

static void
dvb_player_control_tv_init (DvbPlayerControlTv *self)
{
  GstElement *pipeline = NULL;
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS((DvbPlayerControlBase *)self);


  pipeline = kclass->pipeline_init((DvbPlayerControlBase *)self);

  if(pipeline==NULL){
    UMMS_DEBUG("dvb_player_control_tv_init pipeline error.");
    return;
  }

  factory_init(pipeline,(DvbPlayerControlBase *)self);

  return;
}

static void 
dvb_player_control_tv_class_init (DvbPlayerControlTvClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DvbPlayerControlBasePrivate));

  object_class->get_property = dvb_player_control_tv_get_property;
  object_class->set_property = dvb_player_control_tv_set_property;
  object_class->dispose = dvb_player_control_tv_dispose;
  object_class->finalize = dvb_player_control_tv_finalize;

  DvbPlayerControlBaseClass *parent_class = DVB_PLAYER_CONTROL_BASE_CLASS(klass);
  /*derived one*/
  parent_class->set_target = set_target;
  parent_class->release_resource = release_resource;
  parent_class->request_resource = request_resource;
  parent_class->unset_xwindow_target = unset_xwindow_target;
  parent_class->setup_xwindow_target = setup_xwindow_target;

  parent_class->set_volume = set_volume;
  parent_class->get_volume = get_volume;
  parent_class->set_mute = set_mute;
  parent_class->is_mute = is_mute;
  parent_class->set_scale_mode = set_scale_mode;
  parent_class->get_scale_mode = get_scale_mode;
  parent_class->get_video_size = get_video_size;
  parent_class->set_video_size = set_video_size;
  parent_class->autoplug_multiqueue = autoplug_multiqueue;  // Differ Implement

}

DvbPlayerControlTv *
dvb_player_control_tv_new (void)
{
  return g_object_new (DVB_PLAYER_CONTROL_TYPE_TV, NULL);
}


