/*
 * UMMS (Unified Multi Media Service) provides a set of DBus APIs to support
 * playing Audio and Video as well as DVB playback.
 *
 * Authored by Zhiwen Wu <zhiwen.wu@intel.com>
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

#ifndef _UMMS_PLAYER_BACKEND_H
#define _UMMS_PLAYER_BACKEND_H

#include <glib-object.h>
#include <umms-resource-manager.h>
#include <umms-plugin.h>
#include "umms-types.h"

G_BEGIN_DECLS

#define UMMS_TYPE_PLAYER_BACKEND umms_player_backend_get_type()

#define UMMS_PLAYER_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_PLAYER_BACKEND, UmmsPlayerBackend))

#define UMMS_PLAYER_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_PLAYER_BACKEND, UmmsPlayerBackendClass))

#define UMMS_IS_PLAYER_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_PLAYER_BACKEND))

#define UMMS_IS_PLAYER_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_PLAYER_BACKEND))

#define UMMS_PLAYER_BACKEND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_PLAYER_BACKEND, UmmsPlayerBackendClass))

#define REQUEST_RES(self, t, p, e_msg)                                        \
  do{                                                                         \
    ResourceRequest req = {0,};                                               \
    Resource *res = NULL;                                                     \
    req.type = t;                                                             \
    req.preference = p;                                                       \
    res = umms_resource_manager_request_resource (self->res_mngr, &req);      \
    if (!res) {                                                               \
      umms_player_backend_release_resource(self);                                                 \
      umms_player_backend_emit_error (self,              \
                                      UMMS_RESOURCE_ERROR_NO_RESOURCE,        \
                                      e_msg);                          \
      return FALSE;                                                           \
    }                                                                         \
    self->res_list = g_list_append (self->res_list, res);                     \
    }while(0)



typedef struct _UmmsPlayerBackend UmmsPlayerBackend;
typedef struct _UmmsPlayerBackendClass UmmsPlayerBackendClass;
typedef struct _UmmsPlayerBackendPrivate UmmsPlayerBackendPrivate;

struct _UmmsPlayerBackend {
  GObject parent;
  UmmsPlayerBackendPrivate *priv;

  UmmsPlugin *plugin;
  UmmsResourceManager *res_mngr;//no need to unref, since it is global singleton.
  /* client setting */
  gchar *uri;
  gchar *proxy_uri;
  gchar *proxy_id;
  gchar *proxy_pw;
  /* player status */
  PlayerState player_state;//current state
  PlayerState pending_state;//target state, for async state change(*==>PlayerStatePaused, PlayerStateNull/Stopped==>PlayerStatePlaying)

  /* URI specific stuff
   * These fields and associated things should be reset before we play a new URI
   */
  GList    *res_list;
  gboolean buffering;
  gint buffer_percent;
  gint  seekable;
  gboolean is_live;
  gint64 duration;//ms
  gint64 total_bytes;
  gboolean suspended;//child state of PlayerStateStopped
  gint64   pos;//snapshot of suspended execution, for now, we just define the position
  gchar *title;
  gchar *artist;
};

struct _UmmsPlayerBackendClass {
  GObjectClass parent_class;
  gboolean (*set_uri) (UmmsPlayerBackend *self, const gchar *in_uri, GError **err);
  gboolean (*set_target) (UmmsPlayerBackend *self, gint type, GHashTable *params, GError **err);
  gboolean (*play) (UmmsPlayerBackend *self, GError **err);
  gboolean (*pause) (UmmsPlayerBackend *self, GError **err);
  gboolean (*stop) (UmmsPlayerBackend *self, GError **err);
  gboolean (*set_position) (UmmsPlayerBackend *self, gint64 in_pos, GError **err);
  gboolean (*get_position) (UmmsPlayerBackend *self, gint64 *cur_time, GError **err);
  gboolean (*set_playback_rate) (UmmsPlayerBackend *self, gdouble rate, GError **err);
  gboolean (*get_playback_rate) (UmmsPlayerBackend *self, gdouble *out_rate, GError **err);
  gboolean (*set_volume) (UmmsPlayerBackend *self, gint in_volume, GError **err);
  gboolean (*get_volume) (UmmsPlayerBackend *self, gint *vol, GError **err);
  gboolean (*set_video_size) (UmmsPlayerBackend *self, guint in_x, guint in_y, guint in_w, guint in_h, GError **err);
  gboolean (*get_video_size) (UmmsPlayerBackend *self, guint *w, guint *h, GError **err);
  gboolean (*get_buffered_bytes) (UmmsPlayerBackend *self, gint64 *depth, GError **err);
  gboolean (*get_buffered_time) (UmmsPlayerBackend *self, gint64 *depth, GError **err);
  gboolean (*get_media_size_time) (UmmsPlayerBackend *self, gint64 *media_size_time, GError **err);
  gboolean (*get_media_size_bytes) (UmmsPlayerBackend *self, gint64 *media_size_bytes, GError **err);
  gboolean (*has_audio) (UmmsPlayerBackend *self, gboolean *has_audio, GError **err);
  gboolean (*has_video) (UmmsPlayerBackend *self, gboolean *has_video, GError **err);
  gboolean (*is_streaming) (UmmsPlayerBackend *self, gboolean *is_streaming, GError **err);
  gboolean (*is_seekable) (UmmsPlayerBackend *self, gboolean *seekable, GError **err);
  gboolean (*support_fullscreen) (UmmsPlayerBackend *self, gboolean *support_fullscreen, GError **err);
  gboolean (*get_player_state) (UmmsPlayerBackend *self, gint *state, GError **err);
  gboolean (*get_current_video) (UmmsPlayerBackend *self, gint *cur_video, GError **err);
  gboolean (*get_current_audio) (UmmsPlayerBackend *self, gint *cur_audio, GError **err);
  gboolean (*set_current_video) (UmmsPlayerBackend *self, gint cur_video, GError **err);
  gboolean (*set_current_audio) (UmmsPlayerBackend *self, gint cur_audio, GError **err);
  gboolean (*get_video_num) (UmmsPlayerBackend *self, gint *video_num, GError **err);
  gboolean (*get_audio_num) (UmmsPlayerBackend *self, gint *audio_num, GError **err);
  gboolean (*set_proxy) (UmmsPlayerBackend *self, GHashTable *params, GError **err);
  gboolean (*set_subtitle_uri) (UmmsPlayerBackend *self, gchar *sub_uri, GError **err);
  gboolean (*get_subtitle_num) (UmmsPlayerBackend *self, gint *subtitle_num, GError **err);
  gboolean (*get_current_subtitle) (UmmsPlayerBackend *self, gint *cur_sub, GError **err);
  gboolean (*set_current_subtitle) (UmmsPlayerBackend *self, gint cur_sub, GError **err);
  gboolean (*set_buffer_depth) (UmmsPlayerBackend *self, gint format, gint64 buf_val, GError **err);
  gboolean (*get_buffer_depth) (UmmsPlayerBackend *self, gint format, gint64 *buf_val, GError **err);
  gboolean (*set_mute) (UmmsPlayerBackend *self, gint mute, GError **err);
  gboolean (*is_mute) (UmmsPlayerBackend *self, gint *mute, GError **err);
  gboolean (*set_scale_mode) (UmmsPlayerBackend *self, gint scale_mode, GError **err);
  gboolean (*get_scale_mode) (UmmsPlayerBackend *self, gint *scale_mode, GError **err);
  gboolean (*suspend) (UmmsPlayerBackend *self, GError **err);
  gboolean (*restore) (UmmsPlayerBackend *self, GError **err);
  gboolean (*get_video_codec) (UmmsPlayerBackend *self, gint channel, gchar ** video_codec, GError **err);
  gboolean (*get_audio_codec) (UmmsPlayerBackend *self, gint channel, gchar ** audio_codec, GError **err);
  gboolean (*get_video_bitrate) (UmmsPlayerBackend *self, gint channel, gint *bit_rate, GError **err);
  gboolean (*get_audio_bitrate) (UmmsPlayerBackend *self, gint channel, gint *bit_rate, GError **err);
  gboolean (*get_encapsulation) (UmmsPlayerBackend *self, gchar ** encapsulation, GError **err);
  gboolean (*get_audio_samplerate) (UmmsPlayerBackend *self, gint channel, gint * sample_rate, GError **err);
  gboolean (*get_video_framerate) (UmmsPlayerBackend *self, gint channel, gint * frame_rate_num, gint * frame_rate_denom, GError **err);
  gboolean (*get_video_resolution) (UmmsPlayerBackend *self, gint channel, gint * width, gint * height, GError **err);
  gboolean (*get_video_aspect_ratio) (UmmsPlayerBackend *self, gint channel, gint * ratio_num, gint * ratio_denom, GError **err);
  gboolean (*get_protocol_name) (UmmsPlayerBackend *self, gchar ** prot_name, GError **err);
  gboolean (*get_current_uri) (UmmsPlayerBackend *self, gchar ** uri, GError **err);
  gboolean (*get_title) (UmmsPlayerBackend *self, gchar ** uri, GError **err);
  gboolean (*get_artist) (UmmsPlayerBackend *self, gchar ** uri, GError **err);
  gboolean (*record) (UmmsPlayerBackend *self, gboolean to_record, gchar *location, GError **err);
  gboolean (*get_pat) (UmmsPlayerBackend *self, GPtrArray **pat, GError **err);
  gboolean (*get_pmt) (UmmsPlayerBackend *self, guint *program_num, guint *pcr_pid, GPtrArray **stream_info, GError **err);
  gboolean (*get_associated_data_channel) (UmmsPlayerBackend *self, gchar **ip, gint *port, GError **err);
};

GType umms_player_backend_get_type (void) G_GNUC_CONST;

UmmsPlayerBackend *umms_player_backend_new (void);

/*
 * For method that carrys return values (e.g umms_player_backend_get_*()),
 * if the calling failed,must return FALSE and set the GError **err.
 */
gboolean umms_player_backend_set_uri (UmmsPlayerBackend *self, const gchar *uri, GError **err);
gboolean umms_player_backend_set_target (UmmsPlayerBackend *self, gint type, GHashTable *params, GError **err);
gboolean umms_player_backend_play(UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_pause(UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_stop(UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_set_position(UmmsPlayerBackend *self, gint64 pos, GError **err);
gboolean umms_player_backend_get_position(UmmsPlayerBackend *self, gint64 *pos, GError **err);
gboolean umms_player_backend_set_playback_rate(UmmsPlayerBackend *self, gdouble rate, GError **err);
gboolean umms_player_backend_get_playback_rate(UmmsPlayerBackend *self, gdouble *rate, GError **err);
gboolean umms_player_backend_set_volume(UmmsPlayerBackend *self, gint vol, GError **err);
gboolean umms_player_backend_get_volume(UmmsPlayerBackend *self, gint *vol, GError **err);
gboolean umms_player_backend_set_video_size(UmmsPlayerBackend *self, guint x, guint y, guint w, guint h, GError **err);
gboolean umms_player_backend_get_video_size(UmmsPlayerBackend *self, guint *w, guint *h, GError **err);
gboolean umms_player_backend_get_buffered_time(UmmsPlayerBackend *self, gint64 *buffered_time, GError **err);
gboolean umms_player_backend_get_buffered_bytes(UmmsPlayerBackend *self, gint64 *buffered_bytes, GError **err);
gboolean umms_player_backend_get_media_size_time(UmmsPlayerBackend *self, gint64 *size_time, GError **err);
gboolean umms_player_backend_get_media_size_bytes(UmmsPlayerBackend *self, gint64 *size_bytes, GError **err);
gboolean umms_player_backend_has_video(UmmsPlayerBackend *self, gboolean *has_video, GError **err);
gboolean umms_player_backend_has_audio(UmmsPlayerBackend *self, gboolean *has_audio, GError **err);
gboolean umms_player_backend_is_streaming(UmmsPlayerBackend *self, gboolean *is_streaming, GError **err);
gboolean umms_player_backend_is_seekable(UmmsPlayerBackend *self, gboolean *is_seekable, GError **err);
gboolean umms_player_backend_support_fullscreen(UmmsPlayerBackend *self, gboolean *support_fullscreen, GError **err);
gboolean umms_player_backend_get_player_state(UmmsPlayerBackend *self, gint *state, GError **err);
gboolean umms_player_backend_reply(UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_set_proxy (UmmsPlayerBackend *self, GHashTable *params, GError **err);
gboolean umms_player_backend_suspend (UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_restore (UmmsPlayerBackend *self, GError **err);
gboolean umms_player_backend_get_current_video (UmmsPlayerBackend *player, gint *cur_video, GError **err);
gboolean umms_player_backend_get_current_audio (UmmsPlayerBackend *player, gint *cur_audio, GError **err);
gboolean umms_player_backend_set_current_video (UmmsPlayerBackend *player, gint cur_video, GError **err);
gboolean umms_player_backend_set_current_audio (UmmsPlayerBackend *player, gint cur_audio, GError **err);
gboolean umms_player_backend_get_video_num (UmmsPlayerBackend *player, gint *video_num, GError **err);
gboolean umms_player_backend_get_audio_num (UmmsPlayerBackend *player, gint *audio_num, GError **err);
gboolean umms_player_backend_set_subtitle_uri (UmmsPlayerBackend *player, gchar *sub_uri, GError **err);
gboolean umms_player_backend_get_subtitle_num (UmmsPlayerBackend *player, gint *sub_num, GError **err);
gboolean umms_player_backend_get_current_subtitle (UmmsPlayerBackend *player, gint *cur_sub, GError **err);
gboolean umms_player_backend_set_current_subtitle (UmmsPlayerBackend *player, gint cur_sub, GError **err);
gboolean umms_player_backend_set_buffer_depth (UmmsPlayerBackend *player, gint format, gint64 buf_val, GError **err);
gboolean umms_player_backend_get_buffer_depth (UmmsPlayerBackend *player, gint format, gint64 *buf_val, GError **err);
gboolean umms_player_backend_set_mute (UmmsPlayerBackend *player, gint mute, GError **err);
gboolean umms_player_backend_is_mute (UmmsPlayerBackend *player, gint *mute, GError **err);
gboolean umms_player_backend_set_scale_mode (UmmsPlayerBackend *player, gint scale_mode, GError **err);
gboolean umms_player_backend_get_scale_mode (UmmsPlayerBackend *player, gint *scale_mode, GError **err);
gboolean umms_player_backend_get_video_codec (UmmsPlayerBackend *player, gint channel, gchar **video_codec, GError **err);
gboolean umms_player_backend_get_audio_codec (UmmsPlayerBackend *player, gint channel, gchar **audio_codec, GError **err);
gboolean umms_player_backend_get_video_bitrate (UmmsPlayerBackend *player, gint channel, gint *bit_rate, GError **err);
gboolean umms_player_backend_get_audio_bitrate (UmmsPlayerBackend *player, gint channel, gint *bit_rate, GError **err);
gboolean umms_player_backend_get_encapsulation (UmmsPlayerBackend *player, gchar **encapsulation, GError **err);
gboolean umms_player_backend_get_audio_samplerate (UmmsPlayerBackend *player, gint channel, gint *sample_rate, GError **err);
gboolean umms_player_backend_get_video_framerate (UmmsPlayerBackend *player, gint channel, gint *frame_rate_num, gint *frame_rate_denom, GError **err);
gboolean umms_player_backend_get_video_resolution (UmmsPlayerBackend *player, gint channel, gint *width, gint *height, GError **err);
gboolean umms_player_backend_get_video_aspect_ratio (UmmsPlayerBackend *player, gint channel, gint *ratio_num, gint *ratio_denom, GError **err);
gboolean umms_player_backend_get_protocol_name (UmmsPlayerBackend *player, gchar **protocol_name, GError **err);
gboolean umms_player_backend_get_current_uri (UmmsPlayerBackend *player, gchar **uri, GError **err);
gboolean umms_player_backend_get_title (UmmsPlayerBackend *player, gchar **title, GError **err);
gboolean umms_player_backend_get_artist (UmmsPlayerBackend *player, gchar **artist, GError **err);
gboolean umms_player_backend_record (UmmsPlayerBackend *player, gboolean to_record, gchar *location, GError **err);
gboolean umms_player_backend_get_pat (UmmsPlayerBackend *player, GPtrArray **pat, GError **err);
gboolean umms_player_backend_get_pmt (UmmsPlayerBackend *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info, GError **err);
gboolean umms_player_backend_get_associated_data_channel (UmmsPlayerBackend *player, gchar **ip, gint *port, GError **err);

/* non-dbus-exported methods */
void umms_player_backend_set_plugin (UmmsPlayerBackend *self, UmmsPlugin *plugin);
gboolean umms_player_backend_support_prot (UmmsPlayerBackend *player, const gchar *prot);
void umms_player_backend_release_resource (UmmsPlayerBackend *self);
gboolean umms_player_backend_is_live_uri (const gchar *uri);
const gchar * umms_player_backend_state_get_name (PlayerState state);

void umms_player_backend_emit_initialized (UmmsPlayerBackend *self);
void umms_player_backend_emit_eof (UmmsPlayerBackend *self);
void umms_player_backend_emit_error (UmmsPlayerBackend *self, guint error_num, gchar *error_des);
void umms_player_backend_emit_buffering (UmmsPlayerBackend *self, gint percent);
void umms_player_backend_emit_player_state_changed (UmmsPlayerBackend *self, gint old_state, gint new_state);
void umms_player_backend_emit_seeked (UmmsPlayerBackend *self);
void umms_player_backend_emit_stopped (UmmsPlayerBackend *self);
void umms_player_backend_emit_suspended (UmmsPlayerBackend *self);
void umms_player_backend_emit_restored (UmmsPlayerBackend *self);
void umms_player_backend_emit_video_tag_changed (UmmsPlayerBackend *self, gint channel);
void umms_player_backend_emit_audio_tag_changed (UmmsPlayerBackend *self, gint channel);
void umms_player_backend_emit_text_tag_changed (UmmsPlayerBackend *self, gint channel);
void umms_player_backend_emit_metadata_changed (UmmsPlayerBackend *self);
void umms_player_backend_emit_record_start (UmmsPlayerBackend *self);
void umms_player_backend_emit_record_stop (UmmsPlayerBackend *self);

G_END_DECLS

#endif /* _UMMS_PLAYER_BACKEND_H */
