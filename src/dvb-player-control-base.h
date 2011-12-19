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

#ifndef _DVB_PLAYER_CONTROL_BASE_H
#define _DVB_PLAYER_CONTROL_BASE_H


#include <glib-object.h>
#include <gst/gst.h>
#include <X11/Xlib.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"

G_BEGIN_DECLS

#define DVB_PLAYER_CONTROL_TYPE_BASE dvb_player_control_base_get_type()

#define DVB_PLAYER_CONTROL_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBase))

#define DVB_PLAYER_CONTROL_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBaseClass))

#define DVB_PLAYER_CONTROL_IS_COMMON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DVB_PLAYER_CONTROL_TYPE_BASE))

#define DVB_PLAYER_CONTROL_IS_COMMON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DVB_PLAYER_CONTROL_TYPE_BASE))

#define DVB_PLAYER_CONTROL_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBaseClass))

#define INVALID_PLANE_ID -1

#define SOCK_MAX_SERV_CONNECTS 5
#define SOCK_SOCKET_DEFAULT_PORT 0 // The port number will be get by socket create.
#define SOCK_SOCKET_DEFAULT_ADDR "127.0.0.1"
#define INIT_PIDS "0:1:16:17:18"
#define DVB_SRC


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

#define DVB_PLAYER_CONTROL_BASE_PARAMS_NUM (8)

enum dvb_player_control_base_type {
  DVB_T,
  DVB_C,
  DVB_S,
  DVB_TYPE_NUM
};

typedef struct _DvbPlayerControlBase DvbPlayerControlBase;
typedef struct _DvbPlayerControlBaseClass DvbPlayerControlBaseClass;
typedef struct _DvbPlayerControlBasePrivate DvbPlayerControlBasePrivate;

struct _DvbPlayerControlBase
{
  GObject parent;

  DvbPlayerControlBasePrivate *priv;
};


struct _DvbPlayerControlBaseClass
{
  GObjectClass parent_class;

  /*virtual APIs*/
  gboolean (*set_target) (DvbPlayerControlBase *self, gint type, GHashTable *params);

  gboolean (*set_volume) (DvbPlayerControlBase *self, gint vol);

  gboolean (*get_volume) (DvbPlayerControlBase *self, gint *vol);

  gboolean (*set_mute) (DvbPlayerControlBase *self, gint mute);

  gboolean (*is_mute) (DvbPlayerControlBase *self, gint *mute);

  gboolean (*set_scale_mode) (DvbPlayerControlBase *self, gint scale_mode);

  gboolean (*get_scale_mode) (DvbPlayerControlBase *self, gint *scale_mode);

  gboolean (*request_resource) (DvbPlayerControlBase *self);
  void (*release_resource) (DvbPlayerControlBase *self);
  gboolean (*unset_xwindow_target) (DvbPlayerControlBase *self);
  gboolean (*setup_xwindow_target) (DvbPlayerControlBase *self, GHashTable *params);

  gboolean (*set_video_size) (DvbPlayerControlBase *self, guint x, guint y, guint w, guint h);

  gboolean (*get_video_size) (DvbPlayerControlBase *self, guint *w, guint *h);

  void (*no_more_pads_cb) (GstElement *element, gpointer data);
  void (*pad_added_cb) (GstElement *element,GstPad *pad,gpointer data);
  void (*autoplug_multiqueue) (DvbPlayerControlBase *player,GstPad *srcpad,gchar *name);
  gboolean (*autoplug_pad) (DvbPlayerControlBase *player, GstPad *pad, gint chain_type);
  gboolean (*link_sink) (DvbPlayerControlBase *player, GstPad *pad);
  void (*bus_group) (GstElement *pipeline, DvbPlayerControlBase *self);
  GstElement *(*pipeline_init) (DvbPlayerControlBase *self);

};

GType dvb_player_control_base_get_type (void) G_GNUC_CONST;

DvbPlayerControlBase *dvb_player_control_base_new (void);

enum {
  CHAIN_TYPE_VIDEO,
  CHAIN_TYPE_AUDIO,
  CHAIN_TYPE_SUB
};

struct _DvbPlayerControlBasePrivate {

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

G_END_DECLS

#endif /* _DVB_PLAYER_CONTROL_BASE_H */
