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
#include <fcntl.h>
#include <unistd.h>
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
#include "player-control-generic.h"
#include "media-player-control.h"
#include "param-table.h"

G_DEFINE_TYPE (PlayerControlGeneric, player_control_generic, PLAYER_CONTROL_TYPE_BASE);

#define GET_PRIVATE(o) ((PLAYER_CONTROL_BASE(o))->priv)

static gboolean 
unset_xwindow_plane (PlayerControlBase *base)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (base);
  UMMS_DEBUG ("Invoked");

  if (priv->disp && priv->video_win_id) {
    XDestroyWindow (priv->disp, priv->video_win_id);
    priv->video_win_id = 0;
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL; 
  }

  return TRUE;
}

static gboolean
unset_target (MediaPlayerControl *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

  if (!priv->target_initialized)
    return TRUE;

  switch (priv->target_type) {
    case XWindow:
      break;
    case DataCopy:
      break;
    case Socket:
      g_assert_not_reached ();
      break;
    case ReservedType0:
      unset_xwindow_plane (PLAYER_CONTROL_BASE(self));
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  priv->target_type = TargetTypeInvalid;
  priv->target_initialized = FALSE;
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

static int xerror_handler (
  Display *dpy,
  XErrorEvent *event)
{
  return x_print_error (dpy, event, stderr);
}

static gboolean 
_setup_xwindow_plane (PlayerControlBase *self, gchar *rect)
{
  gint x, y, w, h, rx, ry;
  Window root;
  Window win;
  XSetWindowAttributes xattributes;
  guint xattributes_mask = 0;
  gboolean fullscreen = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  MediaPlayerControl *control = MEDIA_PLAYER_CONTROL(self);

  if (priv->xwin_initialized) {
    unset_xwindow_plane (self);
  }

#define FULL_SCREEN_RECT "0,0,0,0"
  if (g_strcmp0 (rect, FULL_SCREEN_RECT)) {
    UMMS_DEBUG ("rectangle = '%s'", rect);
    sscanf (rect , "%u,%u,%u,%u", &x, &y, &w, &h);
    fullscreen = FALSE;
  }

  if (!priv->disp)
    priv->disp = XOpenDisplay (NULL);

  if (!priv->disp) {
    UMMS_DEBUG ("Could not open display");
    return FALSE;
  }

  XSetErrorHandler (xerror_handler);

  if (fullscreen) {
    gint screen_num = DefaultScreen (priv->disp);
    w = DisplayWidth (priv->disp, screen_num);
    h = DisplayHeight (priv->disp, screen_num);
    x = 0;
    y = 0;
  }

  root = DefaultRootWindow (priv->disp);

  xattributes.override_redirect = 1;
  xattributes_mask |= CWOverrideRedirect;

  priv->video_win_id = win = XCreateWindow (priv->disp, root, x, y, w, h, 
      0, CopyFromParent, CopyFromParent, CopyFromParent,
      xattributes_mask, &xattributes);
  XLowerWindow (priv->disp, priv->video_win_id);
  XSync (priv->disp, 0);

  return TRUE;
}

static gboolean 
setup_xwindow_plane (PlayerControlBase *self, GHashTable *params)
{
  GValue *val;
  gchar *rect = NULL;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  MediaPlayerControl *control = MEDIA_PLAYER_CONTROL(self);

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
  if (val) {
    rect = (gchar *)g_value_get_string (val);
  }

  return _setup_xwindow_plane (self, rect);
}

/*virtual Apis: */
static gboolean
set_target (PlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  MediaPlayerControl *self_iface = (MediaPlayerControl *)self;
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

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
    UMMS_DEBUG ("PlayerControl not loaded, reason may be SetUri not invoked");
    return FALSE;
  }

  if (priv->target_initialized) {
    unset_target (self_iface);
  }

  switch (type) {
    case XWindow:
      break;
    case DataCopy:
      break;
    case Socket:
      UMMS_DEBUG ("unsupported target type: Socket");
      ret = FALSE;
      break;
    case ReservedType0:
      ret = setup_xwindow_plane (self_iface, params);
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
activate_player_control (PlayerControlBase *self, GstState target_state)
{
  gboolean ret;
  PlayerState old_pending;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG("derived one");

  g_return_val_if_fail (priv->uri, FALSE);
  g_return_val_if_fail (target_state == GST_STATE_PAUSED || target_state == GST_STATE_PLAYING, FALSE);

  old_pending = priv->pending_state;
  priv->pending_state = ((target_state == GST_STATE_PAUSED) ? PlayerStatePaused : PlayerStatePlaying);

  if (gst_element_set_state(priv->pipeline, target_state) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("set pipeline to %d failed", target_state);
    ret = FALSE;
    goto OUT;
  }

OUT:
  if (!ret) {
    priv->pending_state = old_pending;
  }
  return ret;
}

static void
player_control_generic_get_property (GObject    *object,
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
player_control_generic_set_property (GObject      *object,
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
player_control_generic_dispose (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (player_control_generic_parent_class)->dispose (object);
}

static void
player_control_generic_finalize (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (player_control_generic_parent_class)->finalize (object);
}

static void
player_control_generic_class_init (PlayerControlGenericClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  UMMS_DEBUG("Called");

  g_type_class_add_private (klass, sizeof (PlayerControlBasePrivate));

  object_class->get_property = player_control_generic_get_property;
  object_class->set_property = player_control_generic_set_property;
  object_class->dispose = player_control_generic_dispose;
  object_class->finalize = player_control_generic_finalize;

  PlayerControlBaseClass *parent_class = PLAYER_CONTROL_BASE_CLASS(klass);
  /*derived one*/
  parent_class->activate_player_control = activate_player_control;
  parent_class->set_target = set_target;
  parent_class->unset_target = unset_target;

}

static void
player_control_generic_init (PlayerControlGeneric *self)
{
  PlayerControlBasePrivate *priv;
  GstBus *bus;

  priv = GET_PRIVATE (self);
  /*Setup default target*/
  if (_setup_xwindow_plane (PLAYER_CONTROL_BASE(self), FULL_SCREEN_RECT)) {
    priv->target_type = ReservedType0;
    priv->target_initialized = TRUE;
  }
  UMMS_DEBUG("Called");
}

PlayerControlGeneric *
player_control_generic_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (PLAYER_CONTROL_TYPE_GENERIC, NULL);
}

