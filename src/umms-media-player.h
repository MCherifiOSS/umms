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

#ifndef _UMMS_MEDIA_PLAYER_H
#define _UMMS_MEDIA_PLAYER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_MEDIA_PLAYER umms_media_player_get_type()

#define UMMS_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_MEDIA_PLAYER, UmmsMediaPlayer))

#define UMMS_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_MEDIA_PLAYER, UmmsMediaPlayerClass))

#define UMMS_IS_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_MEDIA_PLAYER))

#define UMMS_IS_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_MEDIA_PLAYER))

#define UMMS_MEDIA_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_MEDIA_PLAYER, UmmsMediaPlayerClass))

typedef struct _UmmsMediaPlayer UmmsMediaPlayer;
typedef struct _UmmsMediaPlayerClass UmmsMediaPlayerClass;
typedef struct _UmmsMediaPlayerPrivate UmmsMediaPlayerPrivate;

struct _UmmsMediaPlayer {
  GObject parent;
  UmmsMediaPlayerPrivate *priv;
};


struct _UmmsMediaPlayerClass {
  GObjectClass parent_class;
};

GType umms_media_player_get_type (void) G_GNUC_CONST;

UmmsMediaPlayer *umms_media_player_new (void);

/*
 * For methods that calling internal API(UmmsPlayerBackend) directly
 * (e.g. umms_media_player_set_position()), just forwards the results (i.e.
 * the return value and error).
 * For other methods(e.g. umms_media_player_set_uri()), they determinate what
 * should return according to their inner logic.
 */
gboolean umms_media_player_set_uri (UmmsMediaPlayer *self, const gchar *uri, GError **error);
gboolean umms_media_player_set_target (UmmsMediaPlayer *self, gint type, GHashTable *params, GError **error);
gboolean umms_media_player_play(UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_pause(UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_stop(UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_set_position(UmmsMediaPlayer *self, gint64 pos, GError **error);
gboolean umms_media_player_get_position(UmmsMediaPlayer *self, gint64 *pos, GError **error);
gboolean umms_media_player_set_playback_rate(UmmsMediaPlayer *self, gdouble rate, GError **error);
gboolean umms_media_player_get_playback_rate(UmmsMediaPlayer *self, gdouble *rate, GError **error);
gboolean umms_media_player_set_volume(UmmsMediaPlayer *self, gint vol, GError **error);
gboolean umms_media_player_get_volume(UmmsMediaPlayer *self, gint *vol, GError **error);
gboolean umms_media_player_set_video_size(UmmsMediaPlayer *self, guint x, guint y, guint w, guint h, GError **error);
gboolean umms_media_player_get_video_size(UmmsMediaPlayer *self, guint *w, guint *h, GError **error);
gboolean umms_media_player_get_buffered_time(UmmsMediaPlayer *self, gint64 *buffered_time, GError **error);
gboolean umms_media_player_get_buffered_bytes(UmmsMediaPlayer *self, gint64 *buffered_bytes, GError **error);
gboolean umms_media_player_get_media_size_time(UmmsMediaPlayer *self, gint64 *size_time, GError **error);
gboolean umms_media_player_get_media_size_bytes(UmmsMediaPlayer *self, gint64 *size_bytes, GError **error);
gboolean umms_media_player_has_video(UmmsMediaPlayer *self, gboolean *has_video, GError **error);
gboolean umms_media_player_has_audio(UmmsMediaPlayer *self, gboolean *has_audio, GError **error);
gboolean umms_media_player_is_streaming(UmmsMediaPlayer *self, gboolean *is_streaming, GError **error);
gboolean umms_media_player_is_seekable(UmmsMediaPlayer *self, gboolean *is_seekable, GError **error);
gboolean umms_media_player_support_fullscreen(UmmsMediaPlayer *self, gboolean *support_fullscreen, GError **error);
gboolean umms_media_player_get_player_state(UmmsMediaPlayer *self, gint *state, GError **error);
gboolean umms_media_player_reply(UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_set_proxy (UmmsMediaPlayer *self, GHashTable *params, GError **error);
gboolean umms_media_player_suspend (UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_restore (UmmsMediaPlayer *self, GError **error);
gboolean umms_media_player_get_current_video (UmmsMediaPlayer *player, gint *cur_video, GError **err);
gboolean umms_media_player_get_current_audio (UmmsMediaPlayer *player, gint *cur_audio, GError **err);
gboolean umms_media_player_set_current_video (UmmsMediaPlayer *player, gint cur_video, GError **err);
gboolean umms_media_player_set_current_audio (UmmsMediaPlayer *player, gint cur_audio, GError **err);
gboolean umms_media_player_get_video_num (UmmsMediaPlayer *player, gint *video_num, GError **err);
gboolean umms_media_player_get_audio_num (UmmsMediaPlayer *player, gint *audio_num, GError **err);
gboolean umms_media_player_set_subtitle_uri (UmmsMediaPlayer *player, gchar *sub_uri, GError **err);
gboolean umms_media_player_get_subtitle_num (UmmsMediaPlayer *player, gint *sub_num, GError **err);
gboolean umms_media_player_get_current_subtitle (UmmsMediaPlayer *player, gint *cur_sub, GError **err);
gboolean umms_media_player_set_current_subtitle (UmmsMediaPlayer *player, gint cur_sub, GError **err);
gboolean umms_media_player_set_buffer_depth (UmmsMediaPlayer *player, gint format, gint64 buf_val, GError **err);
gboolean umms_media_player_get_buffer_depth (UmmsMediaPlayer *player, gint format, gint64 *buf_val, GError **err);
gboolean umms_media_player_set_mute (UmmsMediaPlayer *player, gint mute, GError **err);
gboolean umms_media_player_is_mute (UmmsMediaPlayer *player, gint *mute, GError **err);
gboolean umms_media_player_set_scale_mode (UmmsMediaPlayer *player, gint scale_mode, GError **err);
gboolean umms_media_player_get_scale_mode (UmmsMediaPlayer *player, gint *scale_mode, GError **err);
gboolean umms_media_player_get_video_codec (UmmsMediaPlayer *player, gint channel, gchar **video_codec, GError **err);
gboolean umms_media_player_get_audio_codec (UmmsMediaPlayer *player, gint channel, gchar **audio_codec, GError **err);
gboolean umms_media_player_get_video_bitrate (UmmsMediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean umms_media_player_get_audio_bitrate (UmmsMediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean umms_media_player_get_encapsulation (UmmsMediaPlayer *player, gchar **encapsulation, GError **err);
gboolean umms_media_player_get_audio_samplerate (UmmsMediaPlayer *player, gint channel, gint *sample_rate, GError **err);
gboolean umms_media_player_get_video_framerate (UmmsMediaPlayer *player, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom, GError **err);
gboolean umms_media_player_get_video_resolution (UmmsMediaPlayer *player, gint channel,
    gint * width, gint * height, GError **err);
gboolean umms_media_player_get_video_aspect_ratio (UmmsMediaPlayer *player, gint channel,
    gint * ratio_num, gint * ratio_denom, GError **err);
gboolean umms_media_player_get_protocol_name (UmmsMediaPlayer *player, gchar **protocol_name, GError **err);
gboolean umms_media_player_get_current_uri (UmmsMediaPlayer *player, gchar **uri, GError **err);
gboolean umms_media_player_get_title (UmmsMediaPlayer *player, gchar **title, GError **err);
gboolean umms_media_player_get_artist (UmmsMediaPlayer *player, gchar **artist, GError **err);
gboolean umms_media_player_record (UmmsMediaPlayer *player, gboolean to_record, gchar *location, GError **err);
gboolean umms_media_player_get_pat (UmmsMediaPlayer *player, GPtrArray **pat, GError **err);
gboolean umms_media_player_get_pmt (UmmsMediaPlayer *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info,
                               GError **err);
gboolean umms_media_player_get_associated_data_channel (UmmsMediaPlayer *player, gchar **ip, gint *port, GError **err);

gboolean umms_media_player_activate (UmmsMediaPlayer *player, PlayerState state, GError **err);
G_END_DECLS

#endif /* _UMMS_MEDIA_PLAYER_H */
