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

#ifndef _MEDIA_PLAYER_H
#define _MEDIA_PLAYER_H

#include <glib-object.h>
#include "media-player-control.h"
#include "media-player-factory.h"

G_BEGIN_DECLS

#define TYPE_MEDIA_PLAYER media_player_get_type()

#define MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_MEDIA_PLAYER, MediaPlayer))

#define MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_MEDIA_PLAYER, MediaPlayerClass))

#define IS_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_MEDIA_PLAYER))

#define IS_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_MEDIA_PLAYER))

#define MEDIA_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_MEDIA_PLAYER, MediaPlayerClass))

typedef struct _MediaPlayer MediaPlayer;
typedef struct _MediaPlayerClass MediaPlayerClass;
typedef struct _MediaPlayerPrivate MediaPlayerPrivate;

struct _MediaPlayer
{
  GObject parent;
  MediaPlayerControl *player_control;

  MediaPlayerFactory *factory;

  MediaPlayerPrivate *priv;
};


struct _MediaPlayerClass
{
    GObjectClass parent_class;

#if 0
    /*
     *
     * self:            A MediaPlayer
     * uri:             Uri to play
     * new_engine[out]: TRUE if loaded a new engine and destroyed old engine
     *                  FALSE if not
     *
     * Returns:         TRUE if successful
     *                  FALSE if not
     *
     * Load engine according to uri prefix which indicates the source type, and store it in MediaPlayer::player_control.
     * Subclass should implement this vmethod to customize the procedure of backend engine loading.
     *          
     */
    gboolean (*load_engine) (MediaPlayer *self, const char *uri, gboolean *new_engine);
#endif

};

GType media_player_get_type (void) G_GNUC_CONST;

MediaPlayer *media_player_new (void);

gboolean media_player_set_uri (MediaPlayer *self, const gchar *uri, GError **error);
gboolean media_player_set_target (MediaPlayer *self, gint type, GHashTable *params, GError **error);
gboolean media_player_play(MediaPlayer *self, GError **error);
gboolean media_player_pause(MediaPlayer *self, GError **error);
gboolean media_player_stop(MediaPlayer *self, GError **error);
gboolean media_player_set_position(MediaPlayer *self, gint64 pos, GError **error);
gboolean media_player_get_position(MediaPlayer *self, gint64 *pos, GError **error);
gboolean media_player_set_playback_rate(MediaPlayer *self, gdouble rate, GError **error);
gboolean media_player_get_playback_rate(MediaPlayer *self, gdouble *rate, GError **error);
gboolean media_player_set_volume(MediaPlayer *self, gint vol, GError **error);
gboolean media_player_get_volume(MediaPlayer *self, gint *vol, GError **error);
gboolean media_player_set_window_id(MediaPlayer *self, gdouble id, GError **error);
gboolean media_player_set_video_size(MediaPlayer *self, guint x, guint y, guint w, guint h, GError **error);
gboolean media_player_get_video_size(MediaPlayer *self, guint *w, guint *h, GError **error);
gboolean media_player_get_buffered_time(MediaPlayer *self, gint64 *buffered_time, GError **error);
gboolean media_player_get_buffered_bytes(MediaPlayer *self, gint64 *buffered_bytes, GError **error);
gboolean media_player_get_media_size_time(MediaPlayer *self, gint64 *size_time, GError **error);
gboolean media_player_get_media_size_bytes(MediaPlayer *self, gint64 *size_bytes, GError **error);
gboolean media_player_has_video(MediaPlayer *self, gboolean *has_video, GError **error);
gboolean media_player_has_audio(MediaPlayer *self, gboolean *has_audio, GError **error);
gboolean media_player_is_streaming(MediaPlayer *self, gboolean *is_streaming, GError **error);
gboolean media_player_is_seekable(MediaPlayer *self, gboolean *is_seekable, GError **error);
gboolean media_player_support_fullscreen(MediaPlayer *self, gboolean *support_fullscreen, GError **error);
gboolean media_player_get_player_state(MediaPlayer *self, gint *state, GError **error);
gboolean media_player_reply(MediaPlayer *self, GError **error);
gboolean media_player_set_proxy (MediaPlayer *self, GHashTable *params, GError **error);
gboolean media_player_suspend (MediaPlayer *self, GError **error);
gboolean media_player_restore (MediaPlayer *self, GError **error);
gboolean media_player_get_current_video (MediaPlayer *player, gint *cur_video, GError **err);
gboolean media_player_get_current_audio (MediaPlayer *player, gint *cur_audio, GError **err);
gboolean media_player_set_current_video (MediaPlayer *player, gint cur_video, GError **err);
gboolean media_player_set_current_audio (MediaPlayer *player, gint cur_audio, GError **err);
gboolean media_player_get_video_num (MediaPlayer *player, gint *video_num, GError **err);
gboolean media_player_get_audio_num (MediaPlayer *player, gint *audio_num, GError **err);
gboolean media_player_set_subtitle_uri (MediaPlayer *player, gchar *sub_uri, GError **err);
gboolean media_player_get_subtitle_num (MediaPlayer *player, gint *sub_num, GError **err);
gboolean media_player_get_current_subtitle (MediaPlayer *player, gint *cur_sub, GError **err);
gboolean media_player_set_current_subtitle (MediaPlayer *player, gint cur_sub, GError **err);
gboolean media_player_set_buffer_depth (MediaPlayer *player, gint format, gint64 buf_val, GError **err);
gboolean media_player_get_buffer_depth (MediaPlayer *player, gint format, gint64 *buf_val, GError **err);
gboolean media_player_set_mute (MediaPlayer *player, gint mute, GError **err);
gboolean media_player_is_mute (MediaPlayer *player, gint *mute, GError **err);
gboolean media_player_set_scale_mode (MediaPlayer *player, gint scale_mode, GError **err);
gboolean media_player_get_scale_mode (MediaPlayer *player, gint *scale_mode, GError **err);
gboolean media_player_get_video_codec (MediaPlayer *player, gint channel, gchar **video_codec, GError **err);
gboolean media_player_get_audio_codec (MediaPlayer *player, gint channel, gchar **audio_codec, GError **err);
gboolean media_player_get_video_bitrate (MediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean media_player_get_audio_bitrate (MediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean media_player_get_encapsulation (MediaPlayer *player, gchar **encapsulation, GError **err);
gboolean media_player_get_audio_samplerate (MediaPlayer *player, gint channel, gint *sample_rate, GError **err);
gboolean media_player_get_video_framerate (MediaPlayer *player, gint channel, 
                                        gint * frame_rate_num, gint * frame_rate_denom, GError **err);
gboolean media_player_get_video_resolution (MediaPlayer *player, gint channel, 
                                         gint * width, gint * height, GError **err);
gboolean media_player_get_video_aspect_ratio (MediaPlayer *player, gint channel, 
                                         gint * ratio_num, gint * ratio_denom, GError **err);
gboolean media_player_get_protocol_name (MediaPlayer *player, gchar **protocol_name, GError **err);
gboolean media_player_get_current_uri (MediaPlayer *player, gchar **uri, GError **err);
gboolean media_player_get_title (MediaPlayer *player, gchar **title, GError **err);
gboolean media_player_get_artist (MediaPlayer *player, gchar **artist, GError **err);
gboolean media_player_record (MediaPlayer *player, gboolean to_record, gchar *location, GError **err);
gboolean media_player_get_pat (MediaPlayer *player, GPtrArray **pat, GError **err);
gboolean media_player_get_pmt (MediaPlayer *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info, 
    GError **err);
gboolean media_player_get_associated_data_channel (MediaPlayer *player, gchar **ip, gint *port, GError **err);

G_END_DECLS

#endif /* _MEDIA_PLAYER_H */
