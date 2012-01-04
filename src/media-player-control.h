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

#ifndef __MEDIA_PLAYER_CONTROL_H__
#define __MEDIA_PLAYER_CONTROL_H__
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MediaPlayerControl:
 *
 * Dummy typedef representing any implementation of this interface.
 */
typedef struct _MediaPlayerControl MediaPlayerControl;

/**
 * MediaPlayerControlClass:
 *
 * The class of MediaPlayerControl.
 */
typedef struct _MediaPlayerControlClass MediaPlayerControlClass;

GType media_player_control_get_type (void);
#define TYPE_MEDIA_PLAYER_CONTROL \
  (media_player_control_get_type ())
#define MEDIA_PLAYER_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_MEDIA_PLAYER_CONTROL, MediaPlayerControl))
#define IS_MEDIA_PLAYER_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_MEDIA_PLAYER_CONTROL))
#define MEDIA_PLAYER_CONTROL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE((obj), TYPE_MEDIA_PLAYER_CONTROL, MediaPlayerControlClass))


typedef gboolean (*media_player_control_set_uri_impl) (MediaPlayerControl *self, const gchar *in_uri);
void media_player_control_implement_set_uri (MediaPlayerControlClass *klass, 
                                                   media_player_control_set_uri_impl impl);

typedef gboolean (*media_player_control_set_target_impl) (MediaPlayerControl *self,
                                                                gint type, GHashTable *params);
void media_player_control_implement_set_target (MediaPlayerControlClass *klass,
                                                      media_player_control_set_target_impl impl);

typedef gboolean (*media_player_control_play_impl) (MediaPlayerControl *self);
void media_player_control_implement_play (MediaPlayerControlClass *klass, 
                                                media_player_control_play_impl impl);

typedef gboolean (*media_player_control_pause_impl) (MediaPlayerControl *self);
void media_player_control_implement_pause (MediaPlayerControlClass *klass,
                                                 media_player_control_pause_impl impl);

typedef gboolean (*media_player_control_stop_impl) (MediaPlayerControl *self);
void media_player_control_implement_stop (MediaPlayerControlClass *klass, 
                                                media_player_control_stop_impl impl);

typedef gboolean (*media_player_control_set_position_impl) (MediaPlayerControl *self, gint64 in_pos);
void media_player_control_implement_set_position (MediaPlayerControlClass *klass, 
                                                        media_player_control_set_position_impl impl);

typedef gboolean (*media_player_control_get_position_impl) (MediaPlayerControl *self, gint64 *cur_time);
void media_player_control_implement_get_position (MediaPlayerControlClass *klass,
                                                        media_player_control_get_position_impl impl);

typedef gboolean (*media_player_control_set_playback_rate_impl) (MediaPlayerControl *self, gdouble in_rate);
void media_player_control_implement_set_playback_rate (MediaPlayerControlClass *klass, 
                                                            media_player_control_set_playback_rate_impl impl);

typedef gboolean (*media_player_control_get_playback_rate_impl) (MediaPlayerControl *self, gdouble *out_rate);
void media_player_control_implement_get_playback_rate (MediaPlayerControlClass *klass,
                                                        media_player_control_get_playback_rate_impl impl);

typedef gboolean (*media_player_control_set_volume_impl) (MediaPlayerControl *self, gint in_volume);
void media_player_control_implement_set_volume (MediaPlayerControlClass *klass, 
                                                    media_player_control_set_volume_impl impl);

typedef gboolean (*media_player_control_get_volume_impl) (MediaPlayerControl *self, gint *vol);
void media_player_control_implement_get_volume (MediaPlayerControlClass *klass,
                                                      media_player_control_get_volume_impl impl);

typedef gboolean (*media_player_control_set_window_id_impl) (MediaPlayerControl *self, gdouble in_win_id);
void media_player_control_implement_set_window_id (MediaPlayerControlClass *klass,
                                                        media_player_control_set_window_id_impl impl);

typedef gboolean (*media_player_control_set_video_size_impl) (MediaPlayerControl *self,
                    guint in_x,
                    guint in_y,
                    guint in_w,
                    guint in_h);
void media_player_control_implement_set_video_size (MediaPlayerControlClass *klass,
                                                        media_player_control_set_video_size_impl impl);

typedef gboolean (*media_player_control_get_video_size_impl) (MediaPlayerControl *self, guint *w, guint *h);
void media_player_control_implement_get_video_size (MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_size_impl impl);

typedef gboolean (*media_player_control_get_buffered_bytes_impl) (MediaPlayerControl *self, gint64 *depth);
void media_player_control_implement_get_buffered_bytes (MediaPlayerControlClass *klass,
                                                        media_player_control_get_buffered_bytes_impl impl);

typedef gboolean (*media_player_control_get_buffered_time_impl) (MediaPlayerControl *self, gint64 *depth);
void media_player_control_implement_get_buffered_time (MediaPlayerControlClass *klass,
                                                        media_player_control_get_buffered_time_impl impl);

typedef gboolean (*media_player_control_get_media_size_time_impl) (MediaPlayerControl *self, gint64 *media_size_time);
void media_player_control_implement_get_media_size_time (MediaPlayerControlClass *klass,
                                                        media_player_control_get_media_size_time_impl impl);

typedef gboolean (*media_player_control_get_media_size_bytes_impl) (MediaPlayerControl *self, gint64 *media_size_bytes);
void media_player_control_implement_get_media_size_bytes (MediaPlayerControlClass *klass,
                                                            media_player_control_get_media_size_bytes_impl impl);

typedef gboolean (*media_player_control_has_audio_impl) (MediaPlayerControl *self, gboolean *has_audio);
void media_player_control_implement_has_audio (MediaPlayerControlClass *klass,
                                                     media_player_control_has_audio_impl impl);

typedef gboolean (*media_player_control_has_video_impl) (MediaPlayerControl *self, gboolean *has_video);
void media_player_control_implement_has_video (MediaPlayerControlClass *klass,
                                                     media_player_control_has_video_impl impl);

typedef gboolean (*media_player_control_is_streaming_impl) (MediaPlayerControl *self, gboolean *is_streaming);
void media_player_control_implement_is_streaming (MediaPlayerControlClass *klass,
                                                        media_player_control_is_streaming_impl impl);

typedef gboolean (*media_player_control_is_seekable_impl) (MediaPlayerControl *self, gboolean *seekable);
void media_player_control_implement_is_seekable (MediaPlayerControlClass *klass,
                                                       media_player_control_is_seekable_impl impl);

typedef gboolean (*media_player_control_support_fullscreen_impl) (MediaPlayerControl *self, gboolean *support_fullscreen);
void media_player_control_implement_support_fullscreen (MediaPlayerControlClass *klass,
                                                            media_player_control_support_fullscreen_impl impl);

typedef gboolean (*media_player_control_get_player_state_impl) (MediaPlayerControl *self, gint *state);
void media_player_control_implement_get_player_state (MediaPlayerControlClass *klass, 
                                                            media_player_control_get_player_state_impl impl);

typedef gboolean (*media_player_control_get_current_video_impl) (MediaPlayerControl *self, gint *cur_video);
void media_player_control_implement_get_current_video (MediaPlayerControlClass *klass,
                                                        media_player_control_get_current_video_impl impl);

typedef gboolean (*media_player_control_get_current_audio_impl) (MediaPlayerControl *self, gint *cur_audio);
void media_player_control_implement_get_current_audio (MediaPlayerControlClass *klass,
                                                        media_player_control_get_current_audio_impl impl);

typedef gboolean (*media_player_control_set_current_video_impl) (MediaPlayerControl *self, gint cur_video);
void media_player_control_implement_set_current_video (MediaPlayerControlClass *klass,
                                                        media_player_control_set_current_video_impl impl);

typedef gboolean (*media_player_control_set_current_audio_impl) (MediaPlayerControl *self, gint cur_audio);
void media_player_control_implement_set_current_audio (MediaPlayerControlClass *klass,
                                                        media_player_control_set_current_audio_impl impl);

typedef gboolean (*media_player_control_get_video_num_impl) (MediaPlayerControl *self, gint *video_num);
void media_player_control_implement_get_video_num (MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_num_impl impl);

typedef gboolean (*media_player_control_get_audio_num_impl) (MediaPlayerControl *self, gint *audio_num);
void media_player_control_implement_get_audio_num (MediaPlayerControlClass *klass,
                                                        media_player_control_get_audio_num_impl impl);

typedef gboolean (*media_player_control_set_proxy_impl) (MediaPlayerControl *self,
                                                                GHashTable *params);
void media_player_control_implement_set_proxy (MediaPlayerControlClass *klass,
                                                      media_player_control_set_proxy_impl impl);

typedef gboolean (*media_player_control_set_subtitle_uri_impl) (MediaPlayerControl *self,
                                                                      gchar *sub_uri);
void media_player_control_implement_set_subtitle_uri (MediaPlayerControlClass *klass,
                                                            media_player_control_set_subtitle_uri_impl impl);

typedef gboolean (*media_player_control_get_subtitle_num_impl) (MediaPlayerControl *self, gint *subtitle_num);
void media_player_control_implement_get_subtitle_num (MediaPlayerControlClass *klass,
                                                        media_player_control_get_subtitle_num_impl impl);

typedef gboolean (*media_player_control_get_current_subtitle_impl) (MediaPlayerControl *self, gint *cur_sub);
void media_player_control_implement_get_current_subtitle (MediaPlayerControlClass *klass,
                                                        media_player_control_get_current_subtitle_impl impl);

typedef gboolean (*media_player_control_set_current_subtitle_impl) (MediaPlayerControl *self, gint cur_sub);
void media_player_control_implement_set_current_subtitle (MediaPlayerControlClass *klass,
                                                        media_player_control_set_current_subtitle_impl impl);

typedef gboolean (*media_player_control_set_buffer_depth_impl) (MediaPlayerControl *self, 
                                                                      gint format, gint64 buf_val);
void media_player_control_implement_set_buffer_depth (MediaPlayerControlClass *klass,
                                                        media_player_control_set_buffer_depth_impl impl);

typedef gboolean (*media_player_control_get_buffer_depth_impl) (MediaPlayerControl *self, 
                                                                      gint format, gint64 *buf_val);
void media_player_control_implement_get_buffer_depth (MediaPlayerControlClass *klass,
                                                        media_player_control_get_buffer_depth_impl impl);

typedef gboolean (*media_player_control_set_mute_impl) (MediaPlayerControl *self, gint mute);
void media_player_control_implement_set_mute (MediaPlayerControlClass *klass,
                                                        media_player_control_set_mute_impl impl);

typedef gboolean (*media_player_control_is_mute_impl) (MediaPlayerControl *self, gint *mute);
void media_player_control_implement_is_mute (MediaPlayerControlClass *klass,
                                                        media_player_control_is_mute_impl impl);

typedef gboolean (*media_player_control_set_scale_mode_impl) (MediaPlayerControl *self, gint scale_mode);
void media_player_control_implement_set_scale_mode (MediaPlayerControlClass *klass,
                                                        media_player_control_set_scale_mode_impl impl);

typedef gboolean (*media_player_control_get_scale_mode_impl) (MediaPlayerControl *self, gint *scale_mode);
void media_player_control_implement_get_scale_mode (MediaPlayerControlClass *klass,
                                                        media_player_control_get_scale_mode_impl impl);

typedef gboolean (*media_player_control_suspend_impl) (MediaPlayerControl *self);
void media_player_control_implement_suspend (MediaPlayerControlClass *klass,
                                                        media_player_control_suspend_impl impl);

typedef gboolean (*media_player_control_restore_impl) (MediaPlayerControl *self);
void media_player_control_implement_restore (MediaPlayerControlClass *klass,
                                                        media_player_control_restore_impl impl);

typedef gboolean (*media_player_control_get_video_codec_impl) (MediaPlayerControl *self, 
                                                        gint channel, gchar ** video_codec);
void media_player_control_implement_get_video_codec (MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_codec_impl impl);

typedef gboolean (*media_player_control_get_audio_codec_impl) (MediaPlayerControl *self, 
                                                        gint channel, gchar ** audio_codec);
void media_player_control_implement_get_audio_codec (MediaPlayerControlClass *klass,
                                                        media_player_control_get_audio_codec_impl impl);

typedef gboolean (*media_player_control_get_video_bitrate_impl) (MediaPlayerControl *self, gint channel, gint *bit_rate);
void media_player_control_implement_get_video_bitrate (MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_bitrate_impl impl);

typedef gboolean (*media_player_control_get_audio_bitrate_impl) (MediaPlayerControl *self, gint channel, gint *bit_rate);
void media_player_control_implement_get_audio_bitrate (MediaPlayerControlClass *klass,
                                                        media_player_control_get_audio_bitrate_impl impl);

typedef gboolean (*media_player_control_get_encapsulation_impl) (MediaPlayerControl *self, 
                                                        gchar ** encapsulation);
void media_player_control_implement_get_encapsulation (MediaPlayerControlClass *klass,
                                                        media_player_control_get_encapsulation_impl impl);

typedef gboolean (*media_player_control_get_audio_samplerate_impl) (MediaPlayerControl *self, 
                                                        gint channel, gint * sample_rate);
void media_player_control_implement_get_audio_samplerate (MediaPlayerControlClass *klass,
                                                        media_player_control_get_audio_samplerate_impl impl);

typedef gboolean (*media_player_control_get_video_framerate_impl) (MediaPlayerControl *self, 
                                                        gint channel, gint * frame_rate_num, gint * frame_rate_denom);
void media_player_control_implement_get_video_framerate (MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_framerate_impl impl);

typedef gboolean (*media_player_control_get_video_resolution_impl) (MediaPlayerControl *self,
                                                        gint channel, gint * width, gint * height);
void media_player_control_implement_get_video_resolution(MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_resolution_impl impl);

typedef gboolean (*media_player_control_get_video_aspect_ratio_impl) (MediaPlayerControl *self,
                                                        gint channel, gint * ratio_num, gint * ratio_denom);
void media_player_control_implement_get_video_aspect_ratio(MediaPlayerControlClass *klass,
                                                        media_player_control_get_video_aspect_ratio_impl impl);

typedef gboolean (*media_player_control_get_protocol_name_impl) (MediaPlayerControl *self, gchar ** prot_name);
void media_player_control_implement_get_protocol_name(MediaPlayerControlClass *klass,
                                                        media_player_control_get_protocol_name_impl impl);

typedef gboolean (*media_player_control_get_current_uri_impl) (MediaPlayerControl *self, gchar ** uri);
void media_player_control_implement_get_current_uri(MediaPlayerControlClass *klass,
                                                        media_player_control_get_current_uri_impl impl);

typedef gboolean (*media_player_control_get_title_impl) (MediaPlayerControl *self, gchar ** uri);
void media_player_control_implement_get_title(MediaPlayerControlClass *klass,
                                                        media_player_control_get_title_impl impl);

typedef gboolean (*media_player_control_get_artist_impl) (MediaPlayerControl *self, gchar ** uri);
void media_player_control_implement_get_artist(MediaPlayerControlClass *klass,
                                                        media_player_control_get_artist_impl impl);

typedef gboolean (*media_player_control_record_impl) (MediaPlayerControl *self, gboolean to_record, gchar *location);
void media_player_control_implement_record (MediaPlayerControlClass *klass,
                                                        media_player_control_record_impl impl);
typedef gboolean (*media_player_control_get_pat_impl) (MediaPlayerControl *self, GPtrArray **pat);
void media_player_control_implement_get_pat (MediaPlayerControlClass *klass,
                                                        media_player_control_get_pat_impl impl);
typedef gboolean (*media_player_control_get_pmt_impl) (MediaPlayerControl *self, 
                                                            guint *program_num, guint *pcr_pid, GPtrArray **stream_info);
void media_player_control_implement_get_pmt (MediaPlayerControlClass *klass,
                                                        media_player_control_get_pmt_impl impl);

typedef gboolean (*media_player_control_get_associated_data_channel_impl) (MediaPlayerControl *self, gchar **ip, gint *port);
void media_player_control_implement_get_associated_data_channel (MediaPlayerControlClass *klass,
                                                        media_player_control_get_associated_data_channel_impl impl);


/*virtual function wrappers*/
gboolean media_player_control_set_uri (MediaPlayerControl *self, const gchar *in_uri);
gboolean media_player_control_set_target (MediaPlayerControl *self, gint type, GHashTable *params);
gboolean media_player_control_play (MediaPlayerControl *self);
gboolean media_player_control_pause (MediaPlayerControl *self);
gboolean media_player_control_stop (MediaPlayerControl *self);
gboolean media_player_control_set_position (MediaPlayerControl *self, gint64 in_pos);
gboolean media_player_control_get_position (MediaPlayerControl *self, gint64 *cur_time);
gboolean media_player_control_set_playback_rate (MediaPlayerControl *self, gdouble in_rate);
gboolean media_player_control_get_playback_rate (MediaPlayerControl *self, gdouble *out_rate);
gboolean media_player_control_set_volume (MediaPlayerControl *self, gint in_volume);
gboolean media_player_control_get_volume (MediaPlayerControl *self, gint *vol);
gboolean media_player_control_set_window_id (MediaPlayerControl *self, gdouble in_win_id);
gboolean media_player_control_set_video_size (MediaPlayerControl *self, guint in_x, guint in_y, guint in_w, guint in_h);
gboolean media_player_control_get_video_size (MediaPlayerControl *self, guint *w, guint *h);
gboolean media_player_control_get_buffered_bytes (MediaPlayerControl *self, gint64 *buffered_bytes);
gboolean media_player_control_get_buffered_time (MediaPlayerControl *self, gint64 *buffered_time);
gboolean media_player_control_get_media_size_time (MediaPlayerControl *self, gint64 *media_size_time);
gboolean media_player_control_get_media_size_bytes (MediaPlayerControl *self, gint64 *media_size_bytes);
gboolean media_player_control_has_audio (MediaPlayerControl *self, gboolean *hav_audio);
gboolean media_player_control_has_video (MediaPlayerControl *self, gboolean *has_video);
gboolean media_player_control_is_streaming (MediaPlayerControl *self, gboolean *is_streaming);
gboolean media_player_control_is_seekable (MediaPlayerControl *self, gboolean *seekable);
gboolean media_player_control_support_fullscreen (MediaPlayerControl *self, gboolean *support_fullscreen);
gboolean media_player_control_get_player_state (MediaPlayerControl *self, gint *state);
gboolean media_player_control_get_current_video (MediaPlayerControl *self, gint *cur_video);
gboolean media_player_control_get_current_audio (MediaPlayerControl *self, gint *cur_audio);
gboolean media_player_control_set_current_video (MediaPlayerControl *self, gint cur_video);
gboolean media_player_control_set_current_audio (MediaPlayerControl *self, gint cur_audio);
gboolean media_player_control_get_video_num (MediaPlayerControl *self, gint *video_num);
gboolean media_player_control_get_audio_num (MediaPlayerControl *self, gint *audio_num);
gboolean media_player_control_set_proxy (MediaPlayerControl *self, GHashTable *params);
gboolean media_player_control_set_subtitle_uri (MediaPlayerControl *self, gchar *sub_uri);
gboolean media_player_control_get_subtitle_num (MediaPlayerControl *self, gint *sub_num);
gboolean media_player_control_get_current_subtitle (MediaPlayerControl *self, gint *cur_sub);
gboolean media_player_control_set_current_subtitle (MediaPlayerControl *self, gint cur_sub);
gboolean media_player_control_set_buffer_depth (MediaPlayerControl *self, gint format, gint64 buf_val);
gboolean media_player_control_get_buffer_depth (MediaPlayerControl *self, gint format, gint64 *buf_val);
gboolean media_player_control_set_mute (MediaPlayerControl *self, gint mute);
gboolean media_player_control_is_mute (MediaPlayerControl *self, gint *mute);
gboolean media_player_control_set_scale_mode (MediaPlayerControl *self, gint scale_mode);
gboolean media_player_control_get_scale_mode (MediaPlayerControl *self, gint *scale_mode);
gboolean media_player_control_suspend (MediaPlayerControl *self);
gboolean media_player_control_restore (MediaPlayerControl *self);
gboolean media_player_control_get_video_codec (MediaPlayerControl *self, gint channel, gchar **video_codec);
gboolean media_player_control_get_audio_codec (MediaPlayerControl *self, gint channel, gchar **audio_codec);
gboolean media_player_control_get_video_bitrate (MediaPlayerControl *self, gint channel, gint *bit_rate);
gboolean media_player_control_get_audio_bitrate (MediaPlayerControl *self, gint channel, gint *bit_rate);
gboolean media_player_control_get_encapsulation(MediaPlayerControl *self, gchar ** encapsulation);
gboolean media_player_control_get_audio_samplerate(MediaPlayerControl *self, gint channel, gint * sample_rate);
gboolean media_player_control_get_video_framerate(MediaPlayerControl *self, gint channel,
                                                        gint * frame_rate_num, gint * frame_rate_denom);
gboolean media_player_control_get_video_resolution(MediaPlayerControl *self, gint channel,
                                                        gint * width, gint * height);
gboolean media_player_control_get_video_aspect_ratio(MediaPlayerControl *self,
                                                    gint channel, gint * ratio_num, gint * ratio_denom);
gboolean media_player_control_get_protocol_name(MediaPlayerControl *self, gchar ** prot_name);
gboolean media_player_control_get_current_uri (MediaPlayerControl *self, gchar ** uri);
gboolean media_player_control_get_title(MediaPlayerControl *self, gchar ** title);
gboolean media_player_control_get_artist(MediaPlayerControl *self, gchar ** artist);
gboolean media_player_control_record (MediaPlayerControl *self, gboolean to_record, gchar *location);
gboolean media_player_control_get_pat (MediaPlayerControl *self, GPtrArray **pat);
gboolean media_player_control_get_pmt (MediaPlayerControl *self, 
                                            guint *program_num, guint *pcr_pid, GPtrArray **stream_info);
gboolean media_player_control_get_associated_data_channel (MediaPlayerControl *self, gchar **ip, gint *port);



/*signal emitter*/
void media_player_control_emit_initialized (gpointer instance);
void media_player_control_emit_eof (gpointer instance);
void media_player_control_emit_error (gpointer instance, guint error_num, gchar *error_des);
void media_player_control_emit_seeked (gpointer instance);
void media_player_control_emit_stopped (gpointer instance);
void media_player_control_emit_request_window (gpointer instance);
void media_player_control_emit_buffering (gpointer instance);
void media_player_control_emit_buffered (gpointer instance);
void media_player_control_emit_player_state_changed (gpointer instance, gint old_state, gint cur_state);
void media_player_control_emit_target_ready(gpointer instance, GHashTable *infos);
void media_player_control_emit_video_tag_changed (gpointer instance, gint channel);
void media_player_control_emit_audio_tag_changed (gpointer instance, gint channel);
void media_player_control_emit_text_tag_changed (gpointer instance, gint channel);
void media_player_control_emit_metadata_changed(gpointer instance);
void media_player_control_emit_suspended (gpointer instance);
void media_player_control_emit_restored (gpointer instance);
void media_player_control_emit_record_start(gpointer instance);
void media_player_control_emit_record_stop(gpointer instance);

G_END_DECLS
#endif
