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

#ifndef _MEEGO_MEDIA_PLAYER_H
#define _MEEGO_MEDIA_PLAYER_H

#include <glib-object.h>
#include "meego-media-player-control.h"

G_BEGIN_DECLS

#define MEEGO_TYPE_MEDIA_PLAYER meego_media_player_get_type()

#define MEEGO_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayer))

#define MEEGO_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerClass))

#define MEEGO_IS_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER))

#define MEEGO_IS_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER))

#define MEEGO_MEDIA_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerClass))

typedef struct _MeegoMediaPlayer MeegoMediaPlayer;
typedef struct _MeegoMediaPlayerClass MeegoMediaPlayerClass;
typedef struct _MeegoMediaPlayerPrivate MeegoMediaPlayerPrivate;

struct _MeegoMediaPlayer
{
  GObject parent;
  MeegoMediaPlayerControl *player_control;

  MeegoMediaPlayerPrivate *priv;
};


struct _MeegoMediaPlayerClass
{
    GObjectClass parent_class;

    /*
     *
     * self:            A MeegoMediaPlayer
     * uri:             Uri to play
     * new_engine[out]: TRUE if loaded a new engine and destroyed old engine
     *                  FALSE if not
     *
     * Returns:         TRUE if successful
     *                  FALSE if not
     *
     * Load engine according to uri prefix which indicates the source type, and store it in MeegoMediaPlayer::player_control.
     * Subclass should implement this vmethod to customize the procedure of backend engine loading.
     *          
     */
    gboolean (*load_engine) (MeegoMediaPlayer *self, const char *uri, gboolean *new_engine);

};

GType meego_media_player_get_type (void) G_GNUC_CONST;

MeegoMediaPlayer *meego_media_player_new (void);

gboolean meego_media_player_set_uri (MeegoMediaPlayer *self, const gchar *uri, GError **error);
gboolean meego_media_player_set_target (MeegoMediaPlayer *self, gint type, GHashTable *params, GError **error);
gboolean meego_media_player_play(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_pause(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_stop(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_set_position(MeegoMediaPlayer *self, gint64 pos, GError **error);
gboolean meego_media_player_get_position(MeegoMediaPlayer *self, gint64 *pos, GError **error);
gboolean meego_media_player_set_playback_rate(MeegoMediaPlayer *self, gdouble rate, GError **error);
gboolean meego_media_player_get_playback_rate(MeegoMediaPlayer *self, gdouble *rate, GError **error);
gboolean meego_media_player_set_volume(MeegoMediaPlayer *self, gint vol, GError **error);
gboolean meego_media_player_get_volume(MeegoMediaPlayer *self, gint *vol, GError **error);
gboolean meego_media_player_set_window_id(MeegoMediaPlayer *self, gdouble id, GError **error);
gboolean meego_media_player_set_video_size(MeegoMediaPlayer *self, guint x, guint y, guint w, guint h, GError **error);
gboolean meego_media_player_get_video_size(MeegoMediaPlayer *self, guint *w, guint *h, GError **error);
gboolean meego_media_player_get_buffered_time(MeegoMediaPlayer *self, gint64 *buffered_time, GError **error);
gboolean meego_media_player_get_buffered_bytes(MeegoMediaPlayer *self, gint64 *buffered_bytes, GError **error);
gboolean meego_media_player_get_media_size_time(MeegoMediaPlayer *self, gint64 *size_time, GError **error);
gboolean meego_media_player_get_media_size_bytes(MeegoMediaPlayer *self, gint64 *size_bytes, GError **error);
gboolean meego_media_player_has_video(MeegoMediaPlayer *self, gboolean *has_video, GError **error);
gboolean meego_media_player_has_audio(MeegoMediaPlayer *self, gboolean *has_audio, GError **error);
gboolean meego_media_player_is_streaming(MeegoMediaPlayer *self, gboolean *is_streaming, GError **error);
gboolean meego_media_player_is_seekable(MeegoMediaPlayer *self, gboolean *is_seekable, GError **error);
gboolean meego_media_player_support_fullscreen(MeegoMediaPlayer *self, gboolean *support_fullscreen, GError **error);
gboolean meego_media_player_get_player_state(MeegoMediaPlayer *self, gint *state, GError **error);
gboolean meego_media_player_reply(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_set_proxy (MeegoMediaPlayer *self, GHashTable *params, GError **error);
gboolean meego_media_player_suspend (MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_restore (MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_get_current_video (MeegoMediaPlayer *player, gint *cur_video, GError **err);
gboolean meego_media_player_get_current_audio (MeegoMediaPlayer *player, gint *cur_audio, GError **err);
gboolean meego_media_player_set_current_video (MeegoMediaPlayer *player, gint cur_video, GError **err);
gboolean meego_media_player_set_current_audio (MeegoMediaPlayer *player, gint cur_audio, GError **err);
gboolean meego_media_player_get_video_num (MeegoMediaPlayer *player, gint *video_num, GError **err);
gboolean meego_media_player_get_audio_num (MeegoMediaPlayer *player, gint *audio_num, GError **err);
gboolean meego_media_player_set_subtitle_uri (MeegoMediaPlayer *player, gchar *sub_uri, GError **err);
gboolean meego_media_player_get_subtitle_num (MeegoMediaPlayer *player, gint *sub_num, GError **err);
gboolean meego_media_player_get_current_subtitle (MeegoMediaPlayer *player, gint *cur_sub, GError **err);
gboolean meego_media_player_set_current_subtitle (MeegoMediaPlayer *player, gint cur_sub, GError **err);
gboolean meego_media_player_set_buffer_depth (MeegoMediaPlayer *player, gint format, gint64 buf_val, GError **err);
gboolean meego_media_player_get_buffer_depth (MeegoMediaPlayer *player, gint format, gint64 *buf_val, GError **err);
gboolean meego_media_player_set_mute (MeegoMediaPlayer *player, gint mute, GError **err);
gboolean meego_media_player_is_mute (MeegoMediaPlayer *player, gint *mute, GError **err);
gboolean meego_media_player_set_scale_mode (MeegoMediaPlayer *player, gint scale_mode, GError **err);
gboolean meego_media_player_get_scale_mode (MeegoMediaPlayer *player, gint *scale_mode, GError **err);
gboolean meego_media_player_get_video_codec (MeegoMediaPlayer *player, gint channel, gchar **video_codec, GError **err);
gboolean meego_media_player_get_audio_codec (MeegoMediaPlayer *player, gint channel, gchar **audio_codec, GError **err);
gboolean meego_media_player_get_video_bitrate (MeegoMediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean meego_media_player_get_audio_bitrate (MeegoMediaPlayer *player, gint channel, gint *bit_rate, GError **err);
gboolean meego_media_player_get_encapsulation (MeegoMediaPlayer *player, gchar **encapsulation, GError **err);
gboolean meego_media_player_get_audio_samplerate (MeegoMediaPlayer *player, gint channel, gint *sample_rate, GError **err);
gboolean meego_media_player_get_video_framerate (MeegoMediaPlayer *player, gint channel, 
                                        gint * frame_rate_num, gint * frame_rate_denom, GError **err);
gboolean meego_media_player_get_video_resolution (MeegoMediaPlayer *player, gint channel, 
                                         gint * width, gint * height, GError **err);
gboolean meego_media_player_get_video_aspect_ratio (MeegoMediaPlayer *player, gint channel, 
                                         gint * ratio_num, gint * ratio_denom, GError **err);
gboolean meego_media_player_get_protocol_name (MeegoMediaPlayer *player, gchar **protocol_name, GError **err);
gboolean meego_media_player_get_current_uri (MeegoMediaPlayer *player, gchar **uri, GError **err);
gboolean meego_media_player_get_title (MeegoMediaPlayer *player, gchar **title, GError **err);
gboolean meego_media_player_get_artist (MeegoMediaPlayer *player, gchar **artist, GError **err);
gboolean meego_media_player_record (MeegoMediaPlayer *player, gboolean to_record, gchar *location, GError **err);
gboolean meego_media_player_get_pat (MeegoMediaPlayer *player, GPtrArray **pat, GError **err);
gboolean meego_media_player_get_pmt (MeegoMediaPlayer *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info, 
    GError **err);
gboolean meego_media_player_get_associated_data_channel (MeegoMediaPlayer *player, gchar **ip, gint *port, GError **err);

G_END_DECLS

#endif /* _MEEGO_MEDIA_PLAYER_H */
