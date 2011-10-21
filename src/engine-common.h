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

#ifndef _ENGINE_COMMON_H
#define _ENGINE_COMMON_H

#include <glib-object.h>
#include <gst/gst.h>
#include <X11/Xlib.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"

G_BEGIN_DECLS

#define ENGINE_TYPE_COMMON engine_common_get_type()

#define ENGINE_COMMON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  ENGINE_TYPE_COMMON, EngineCommon))

#define ENGINE_COMMON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  ENGINE_TYPE_COMMON, EngineCommonClass))

#define ENGINE_IS_COMMON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  ENGINE_TYPE_COMMON))

#define ENGINE_IS_COMMON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  ENGINE_TYPE_COMMON))

#define ENGINE_COMMON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  ENGINE_TYPE_COMMON, EngineCommonClass))

typedef struct _EngineCommon EngineCommon;
typedef struct _EngineCommonClass EngineCommonClass;
typedef struct _EngineCommonPrivate EngineCommonPrivate;

struct _EngineCommon
{
  GObject parent;

  EngineCommonPrivate *priv;
};


struct _EngineCommonClass
{
  GObjectClass parent_class;

  /*virtual APIs*/
  gboolean (*activate_engine) (EngineCommon *self, GstState target_state);
  gboolean (*set_target) (EngineCommon *self, gint type, GHashTable *params);
};

GType engine_common_get_type (void) G_GNUC_CONST;

EngineCommon *engine_common_new (void);


struct _EngineCommonPrivate {
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

#endif /* _ENGINE_COMMON_H */
