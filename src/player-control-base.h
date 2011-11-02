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

#ifndef _PLAYER_CONTROL_BASE_H
#define _PLAYER_CONTROL_BASE_H

#include <glib-object.h>
#include <gst/gst.h>
#include <X11/Xlib.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"

G_BEGIN_DECLS

#define PLAYER_CONTROL_TYPE_BASE player_control_base_get_type()

#define PLAYER_CONTROL_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  PLAYER_CONTROL_TYPE_BASE, PlayerControlBase))

#define PLAYER_CONTROL_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  PLAYER_CONTROL_TYPE_BASE, PlayerControlBaseClass))

#define PLAYER_CONTROL_IS_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  PLAYER_CONTROL_TYPE_BASE))

#define PLAYER_CONTROL_IS_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  PLAYER_CONTROL_TYPE_BASE))

#define PLAYER_CONTROL_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  PLAYER_CONTROL_TYPE_BASE, PlayerControlBaseClass))

typedef struct _PlayerControlBase PlayerControlBase;
typedef struct _PlayerControlBaseClass PlayerControlBaseClass;
typedef struct _PlayerControlBasePrivate PlayerControlBasePrivate;

struct _PlayerControlBase
{
  GObject parent;

  PlayerControlBasePrivate *priv;
};


struct _PlayerControlBaseClass
{
  GObjectClass parent_class;

  /*virtual APIs*/
  /*platform specified*/
  gboolean (*activate_player_control) (PlayerControlBase *self, GstState target_state);
  gboolean (*set_target) (PlayerControlBase *self, gint type, GHashTable *params);

  void     (*release_resource) (PlayerControlBase *self);
  gboolean (*request_resource) (PlayerControlBase *self);
  gboolean (*setup_xwindow_target) (PlayerControlBase *self, GHashTable *params);
  gboolean (*unset_xwindow_target) (PlayerControlBase *self);
  gboolean (*set_subtitle_uri) (PlayerControlBase *self, gchar *sub_uri);

  /*used by both base class and platform specified class*/
  void     (*set_proxy) (PlayerControlBase *self);
};

GType player_control_base_get_type (void) G_GNUC_CONST;

PlayerControlBase *player_control_base_new (void);


struct _PlayerControlBasePrivate {
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
  GstElement *uridecodebin;
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


G_END_DECLS

#endif /* _PLAYER_CONTROL_BASE_H */
