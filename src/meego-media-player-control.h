#ifndef __MEEGO_MEDIA_PLAYER_CONTROL_H__
#define __MEEGO_MEDIA_PLAYER_CONTROL_H__
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MeegoMediaPlayerControl:
 *
 * Dummy typedef representing any implementation of this interface.
 */
typedef struct _MeegoMediaPlayerControl MeegoMediaPlayerControl;

/**
 * MeegoMediaPlayerControlClass:
 *
 * The class of MeegoMediaPlayerControl.
 */
typedef struct _MeegoMediaPlayerControlClass MeegoMediaPlayerControlClass;

GType meego_media_player_control_get_type (void);
#define MEEGO_TYPE_MEDIA_PLAYER_CONTROL \
  (meego_media_player_control_get_type ())
#define MEEGO_MEDIA_PLAYER_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL, MeegoMediaPlayerControl))
#define MEEGO_IS_MEDIA_PLAYER_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL))
#define MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL, MeegoMediaPlayerControlClass))


typedef gboolean (*meego_media_player_control_set_uri_impl) (MeegoMediaPlayerControl *self, const gchar *in_uri);
void meego_media_player_control_implement_set_uri (MeegoMediaPlayerControlClass *klass, 
                                                   meego_media_player_control_set_uri_impl impl);

typedef gboolean (*meego_media_player_control_set_target_impl) (MeegoMediaPlayerControl *self,
                                                                gint type, GHashTable *params);
void meego_media_player_control_implement_set_target (MeegoMediaPlayerControlClass *klass,
                                                      meego_media_player_control_set_target_impl impl);

typedef gboolean (*meego_media_player_control_play_impl) (MeegoMediaPlayerControl *self);
void meego_media_player_control_implement_play (MeegoMediaPlayerControlClass *klass, 
                                                meego_media_player_control_play_impl impl);

typedef gboolean (*meego_media_player_control_pause_impl) (MeegoMediaPlayerControl *self);
void meego_media_player_control_implement_pause (MeegoMediaPlayerControlClass *klass,
                                                 meego_media_player_control_pause_impl impl);

typedef gboolean (*meego_media_player_control_stop_impl) (MeegoMediaPlayerControl *self);
void meego_media_player_control_implement_stop (MeegoMediaPlayerControlClass *klass, 
                                                meego_media_player_control_stop_impl impl);

typedef gboolean (*meego_media_player_control_set_position_impl) (MeegoMediaPlayerControl *self, gint64 in_pos);
void meego_media_player_control_implement_set_position (MeegoMediaPlayerControlClass *klass, 
                                                        meego_media_player_control_set_position_impl impl);

typedef gboolean (*meego_media_player_control_get_position_impl) (MeegoMediaPlayerControl *self, gint64 *cur_time);
void meego_media_player_control_implement_get_position (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_position_impl impl);

typedef gboolean (*meego_media_player_control_set_playback_rate_impl) (MeegoMediaPlayerControl *self, gdouble in_rate);
void meego_media_player_control_implement_set_playback_rate (MeegoMediaPlayerControlClass *klass, 
                                                            meego_media_player_control_set_playback_rate_impl impl);

typedef gboolean (*meego_media_player_control_get_playback_rate_impl) (MeegoMediaPlayerControl *self, gdouble *out_rate);
void meego_media_player_control_implement_get_playback_rate (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_playback_rate_impl impl);

typedef gboolean (*meego_media_player_control_set_volume_impl) (MeegoMediaPlayerControl *self, gint in_volume);
void meego_media_player_control_implement_set_volume (MeegoMediaPlayerControlClass *klass, 
                                                    meego_media_player_control_set_volume_impl impl);

typedef gboolean (*meego_media_player_control_get_volume_impl) (MeegoMediaPlayerControl *self, gint *vol);
void meego_media_player_control_implement_get_volume (MeegoMediaPlayerControlClass *klass,
                                                      meego_media_player_control_get_volume_impl impl);

typedef gboolean (*meego_media_player_control_set_window_id_impl) (MeegoMediaPlayerControl *self, gdouble in_win_id);
void meego_media_player_control_implement_set_window_id (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_window_id_impl impl);

typedef gboolean (*meego_media_player_control_set_video_size_impl) (MeegoMediaPlayerControl *self,
                    guint in_x,
                    guint in_y,
                    guint in_w,
                    guint in_h);
void meego_media_player_control_implement_set_video_size (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_video_size_impl impl);

typedef gboolean (*meego_media_player_control_get_video_size_impl) (MeegoMediaPlayerControl *self, guint *w, guint *h);
void meego_media_player_control_implement_get_video_size (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_size_impl impl);

typedef gboolean (*meego_media_player_control_get_buffered_bytes_impl) (MeegoMediaPlayerControl *self, gint64 *depth);
void meego_media_player_control_implement_get_buffered_bytes (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_buffered_bytes_impl impl);

typedef gboolean (*meego_media_player_control_get_buffered_time_impl) (MeegoMediaPlayerControl *self, gint64 *depth);
void meego_media_player_control_implement_get_buffered_time (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_buffered_time_impl impl);

typedef gboolean (*meego_media_player_control_get_media_size_time_impl) (MeegoMediaPlayerControl *self, gint64 *media_size_time);
void meego_media_player_control_implement_get_media_size_time (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_media_size_time_impl impl);

typedef gboolean (*meego_media_player_control_get_media_size_bytes_impl) (MeegoMediaPlayerControl *self, gint64 *media_size_bytes);
void meego_media_player_control_implement_get_media_size_bytes (MeegoMediaPlayerControlClass *klass,
                                                            meego_media_player_control_get_media_size_bytes_impl impl);

typedef gboolean (*meego_media_player_control_has_audio_impl) (MeegoMediaPlayerControl *self, gboolean *has_audio);
void meego_media_player_control_implement_has_audio (MeegoMediaPlayerControlClass *klass,
                                                     meego_media_player_control_has_audio_impl impl);

typedef gboolean (*meego_media_player_control_has_video_impl) (MeegoMediaPlayerControl *self, gboolean *has_video);
void meego_media_player_control_implement_has_video (MeegoMediaPlayerControlClass *klass,
                                                     meego_media_player_control_has_video_impl impl);

typedef gboolean (*meego_media_player_control_is_streaming_impl) (MeegoMediaPlayerControl *self, gboolean *is_streaming);
void meego_media_player_control_implement_is_streaming (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_is_streaming_impl impl);

typedef gboolean (*meego_media_player_control_is_seekable_impl) (MeegoMediaPlayerControl *self, gboolean *seekable);
void meego_media_player_control_implement_is_seekable (MeegoMediaPlayerControlClass *klass,
                                                       meego_media_player_control_is_seekable_impl impl);

typedef gboolean (*meego_media_player_control_support_fullscreen_impl) (MeegoMediaPlayerControl *self, gboolean *support_fullscreen);
void meego_media_player_control_implement_support_fullscreen (MeegoMediaPlayerControlClass *klass,
                                                            meego_media_player_control_support_fullscreen_impl impl);

typedef gboolean (*meego_media_player_control_get_player_state_impl) (MeegoMediaPlayerControl *self, gint *state);
void meego_media_player_control_implement_get_player_state (MeegoMediaPlayerControlClass *klass, 
                                                            meego_media_player_control_get_player_state_impl impl);

typedef gboolean (*meego_media_player_control_get_current_video_impl) (MeegoMediaPlayerControl *self, gint *cur_video);
void meego_media_player_control_implement_get_current_video (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_current_video_impl impl);

typedef gboolean (*meego_media_player_control_get_current_audio_impl) (MeegoMediaPlayerControl *self, gint *cur_audio);
void meego_media_player_control_implement_get_current_audio (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_current_audio_impl impl);

typedef gboolean (*meego_media_player_control_set_current_video_impl) (MeegoMediaPlayerControl *self, gint cur_video);
void meego_media_player_control_implement_set_current_video (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_current_video_impl impl);

typedef gboolean (*meego_media_player_control_set_current_audio_impl) (MeegoMediaPlayerControl *self, gint cur_audio);
void meego_media_player_control_implement_set_current_audio (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_current_audio_impl impl);

typedef gboolean (*meego_media_player_control_get_video_num_impl) (MeegoMediaPlayerControl *self, gint *video_num);
void meego_media_player_control_implement_get_video_num (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_num_impl impl);

typedef gboolean (*meego_media_player_control_get_audio_num_impl) (MeegoMediaPlayerControl *self, gint *audio_num);
void meego_media_player_control_implement_get_audio_num (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_audio_num_impl impl);

typedef gboolean (*meego_media_player_control_set_proxy_impl) (MeegoMediaPlayerControl *self,
                                                                GHashTable *params);
void meego_media_player_control_implement_set_proxy (MeegoMediaPlayerControlClass *klass,
                                                      meego_media_player_control_set_proxy_impl impl);

typedef gboolean (*meego_media_player_control_set_subtitle_uri_impl) (MeegoMediaPlayerControl *self,
                                                                      gchar *sub_uri);
void meego_media_player_control_implement_set_subtitle_uri (MeegoMediaPlayerControlClass *klass,
                                                            meego_media_player_control_set_subtitle_uri_impl impl);

typedef gboolean (*meego_media_player_control_get_subtitle_num_impl) (MeegoMediaPlayerControl *self, gint *subtitle_num);
void meego_media_player_control_implement_get_subtitle_num (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_subtitle_num_impl impl);

typedef gboolean (*meego_media_player_control_get_current_subtitle_impl) (MeegoMediaPlayerControl *self, gint *cur_sub);
void meego_media_player_control_implement_get_current_subtitle (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_current_subtitle_impl impl);

typedef gboolean (*meego_media_player_control_set_current_subtitle_impl) (MeegoMediaPlayerControl *self, gint cur_sub);
void meego_media_player_control_implement_set_current_subtitle (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_current_subtitle_impl impl);

typedef gboolean (*meego_media_player_control_set_buffer_depth_impl) (MeegoMediaPlayerControl *self, 
                                                                      gint format, gint64 buf_val);
void meego_media_player_control_implement_set_buffer_depth (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_buffer_depth_impl impl);

typedef gboolean (*meego_media_player_control_set_mute_impl) (MeegoMediaPlayerControl *self, gint mute);
void meego_media_player_control_implement_set_mute (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_mute_impl impl);

typedef gboolean (*meego_media_player_control_is_mute_impl) (MeegoMediaPlayerControl *self, gint *mute);
void meego_media_player_control_implement_is_mute (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_is_mute_impl impl);

typedef gboolean (*meego_media_player_control_set_scale_mode_impl) (MeegoMediaPlayerControl *self, gint scale_mode);
void meego_media_player_control_implement_set_scale_mode (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_set_scale_mode_impl impl);

typedef gboolean (*meego_media_player_control_get_scale_mode_impl) (MeegoMediaPlayerControl *self, gint *scale_mode);
void meego_media_player_control_implement_get_scale_mode (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_scale_mode_impl impl);

typedef gboolean (*meego_media_player_control_suspend_impl) (MeegoMediaPlayerControl *self);
void meego_media_player_control_implement_suspend (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_suspend_impl impl);

typedef gboolean (*meego_media_player_control_restore_impl) (MeegoMediaPlayerControl *self);
void meego_media_player_control_implement_restore (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_restore_impl impl);

typedef gboolean (*meego_media_player_control_get_video_codec_impl) (MeegoMediaPlayerControl *self, 
                                                        gint channel, gchar ** video_codec);
void meego_media_player_control_implement_get_video_codec (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_codec_impl impl);

typedef gboolean (*meego_media_player_control_get_audio_codec_impl) (MeegoMediaPlayerControl *self, 
                                                        gint channel, gchar ** audio_codec);
void meego_media_player_control_implement_get_audio_codec (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_audio_codec_impl impl);

typedef gboolean (*meego_media_player_control_get_video_bitrate_impl) (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate);
void meego_media_player_control_implement_get_video_bitrate (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_bitrate_impl impl);

typedef gboolean (*meego_media_player_control_get_audio_bitrate_impl) (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate);
void meego_media_player_control_implement_get_audio_bitrate (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_audio_bitrate_impl impl);

typedef gboolean (*meego_media_player_control_get_encapsulation_impl) (MeegoMediaPlayerControl *self, 
                                                        gchar ** encapsulation);
void meego_media_player_control_implement_get_encapsulation (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_encapsulation_impl impl);

typedef gboolean (*meego_media_player_control_get_audio_samplerate_impl) (MeegoMediaPlayerControl *self, 
                                                        gint channel, gint * sample_rate);
void meego_media_player_control_implement_get_audio_samplerate (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_audio_samplerate_impl impl);

typedef gboolean (*meego_media_player_control_get_video_framerate_impl) (MeegoMediaPlayerControl *self, 
                                                        gint channel, gint * frame_rate_num, gint * frame_rate_denom);
void meego_media_player_control_implement_get_video_framerate (MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_framerate_impl impl);

typedef gboolean (*meego_media_player_control_get_video_resolution_impl) (MeegoMediaPlayerControl *self,
                                                        gint channel, gint * width, gint * height);
void meego_media_player_control_implement_get_video_resolution(MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_resolution_impl impl);

typedef gboolean (*meego_media_player_control_get_video_aspect_ratio_impl) (MeegoMediaPlayerControl *self,
                                                        gint channel, gint * ratio_num, gint * ratio_denom);
void meego_media_player_control_implement_get_video_aspect_ratio(MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_video_aspect_ratio_impl impl);

typedef gboolean (*meego_media_player_control_get_protocol_name_impl) (MeegoMediaPlayerControl *self, gchar ** prot_name);
void meego_media_player_control_implement_get_protocol_name(MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_protocol_name_impl impl);

typedef gboolean (*meego_media_player_control_get_current_uri_impl) (MeegoMediaPlayerControl *self, gchar ** uri);
void meego_media_player_control_implement_get_current_uri(MeegoMediaPlayerControlClass *klass,
                                                        meego_media_player_control_get_current_uri_impl impl);

/*virtual function wrappers*/
gboolean meego_media_player_control_set_uri (MeegoMediaPlayerControl *self, const gchar *in_uri);
gboolean meego_media_player_control_set_target (MeegoMediaPlayerControl *self, gint type, GHashTable *params);
gboolean meego_media_player_control_play (MeegoMediaPlayerControl *self);
gboolean meego_media_player_control_pause (MeegoMediaPlayerControl *self);
gboolean meego_media_player_control_stop (MeegoMediaPlayerControl *self);
gboolean meego_media_player_control_set_position (MeegoMediaPlayerControl *self, gint64 in_pos);
gboolean meego_media_player_control_get_position (MeegoMediaPlayerControl *self, gint64 *cur_time);
gboolean meego_media_player_control_set_playback_rate (MeegoMediaPlayerControl *self, gdouble in_rate);
gboolean meego_media_player_control_get_playback_rate (MeegoMediaPlayerControl *self, gdouble *out_rate);
gboolean meego_media_player_control_set_volume (MeegoMediaPlayerControl *self, gint in_volume);
gboolean meego_media_player_control_get_volume (MeegoMediaPlayerControl *self, gint *vol);
gboolean meego_media_player_control_set_window_id (MeegoMediaPlayerControl *self, gdouble in_win_id);
gboolean meego_media_player_control_set_video_size (MeegoMediaPlayerControl *self, guint in_x, guint in_y, guint in_w, guint in_h);
gboolean meego_media_player_control_get_video_size (MeegoMediaPlayerControl *self, guint *w, guint *h);
gboolean meego_media_player_control_get_buffered_bytes (MeegoMediaPlayerControl *self, gint64 *buffered_bytes);
gboolean meego_media_player_control_get_buffered_time (MeegoMediaPlayerControl *self, gint64 *buffered_time);
gboolean meego_media_player_control_get_media_size_time (MeegoMediaPlayerControl *self, gint64 *media_size_time);
gboolean meego_media_player_control_get_media_size_bytes (MeegoMediaPlayerControl *self, gint64 *media_size_bytes);
gboolean meego_media_player_control_has_audio (MeegoMediaPlayerControl *self, gboolean *hav_audio);
gboolean meego_media_player_control_has_video (MeegoMediaPlayerControl *self, gboolean *has_video);
gboolean meego_media_player_control_is_streaming (MeegoMediaPlayerControl *self, gboolean *is_streaming);
gboolean meego_media_player_control_is_seekable (MeegoMediaPlayerControl *self, gboolean *seekable);
gboolean meego_media_player_control_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen);
gboolean meego_media_player_control_get_player_state (MeegoMediaPlayerControl *self, gint *state);
gboolean meego_media_player_control_get_current_video (MeegoMediaPlayerControl *self, gint *cur_video);
gboolean meego_media_player_control_get_current_audio (MeegoMediaPlayerControl *self, gint *cur_audio);
gboolean meego_media_player_control_set_current_video (MeegoMediaPlayerControl *self, gint cur_video);
gboolean meego_media_player_control_set_current_audio (MeegoMediaPlayerControl *self, gint cur_audio);
gboolean meego_media_player_control_get_video_num (MeegoMediaPlayerControl *self, gint *video_num);
gboolean meego_media_player_control_get_audio_num (MeegoMediaPlayerControl *self, gint *audio_num);
gboolean meego_media_player_control_set_proxy (MeegoMediaPlayerControl *self, GHashTable *params);
gboolean meego_media_player_control_set_subtitle_uri (MeegoMediaPlayerControl *self, gchar *sub_uri);
gboolean meego_media_player_control_get_subtitle_num (MeegoMediaPlayerControl *self, gint *sub_num);
gboolean meego_media_player_control_get_current_subtitle (MeegoMediaPlayerControl *self, gint *cur_sub);
gboolean meego_media_player_control_set_current_subtitle (MeegoMediaPlayerControl *self, gint cur_sub);
gboolean meego_media_player_control_set_buffer_depth (MeegoMediaPlayerControl *self, gint format, gint64 buf_val);
gboolean meego_media_player_control_set_mute (MeegoMediaPlayerControl *self, gint mute);
gboolean meego_media_player_control_is_mute (MeegoMediaPlayerControl *self, gint *mute);
gboolean meego_media_player_control_set_scale_mode (MeegoMediaPlayerControl *self, gint scale_mode);
gboolean meego_media_player_control_get_scale_mode (MeegoMediaPlayerControl *self, gint *scale_mode);
gboolean meego_media_player_control_suspend (MeegoMediaPlayerControl *self);
gboolean meego_media_player_control_restore (MeegoMediaPlayerControl *self);
gboolean meego_media_player_control_get_video_codec (MeegoMediaPlayerControl *self, gint channel, gchar **video_codec);
gboolean meego_media_player_control_get_audio_codec (MeegoMediaPlayerControl *self, gint channel, gchar **audio_codec);
gboolean meego_media_player_control_get_video_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate);
gboolean meego_media_player_control_get_audio_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate);
gboolean meego_media_player_control_get_encapsulation(MeegoMediaPlayerControl *self, gchar ** encapsulation);
gboolean meego_media_player_control_get_audio_samplerate(MeegoMediaPlayerControl *self, gint channel, gint * sample_rate);
gboolean meego_media_player_control_get_video_framerate(MeegoMediaPlayerControl *self, gint channel,
                                                        gint * frame_rate_num, gint * frame_rate_denom);
gboolean meego_media_player_control_get_video_resolution(MeegoMediaPlayerControl *self, gint channel,
                                                        gint * width, gint * height);
gboolean meego_media_player_control_get_video_aspect_ratio(MeegoMediaPlayerControl *self,
                                                    gint channel, gint * ratio_num, gint * ratio_denom);
gboolean meego_media_player_control_get_protocol_name(MeegoMediaPlayerControl *self, gchar ** prot_name);

/*signal emitter*/
void meego_media_player_control_emit_initialized (gpointer instance);
void meego_media_player_control_emit_eof (gpointer instance);
void meego_media_player_control_emit_error (gpointer instance, guint error_num, gchar *error_des);
void meego_media_player_control_emit_seeked (gpointer instance);
void meego_media_player_control_emit_stopped (gpointer instance);
void meego_media_player_control_emit_request_window (gpointer instance);
void meego_media_player_control_emit_buffering (gpointer instance);
void meego_media_player_control_emit_buffered (gpointer instance);
void meego_media_player_control_emit_player_state_changed (gpointer instance, gint cur_state);
void meego_media_player_control_emit_target_ready(gpointer instance, GHashTable *infos);

G_END_DECLS
#endif
