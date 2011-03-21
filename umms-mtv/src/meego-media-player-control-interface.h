#ifndef __MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_H__
#define __MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_H__
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MeegoMediaPlayerControlInterface:
 *
 * Dummy typedef representing any implementation of this interface.
 */
typedef struct _MeegoMediaPlayerControlInterface MeegoMediaPlayerControlInterface;

/**
 * MeegoMediaPlayerControlInterfaceClass:
 *
 * The class of MeegoMediaPlayerControlInterface.
 */
typedef struct _MeegoMediaPlayerControlInterfaceClass MeegoMediaPlayerControlInterfaceClass;

GType meego_media_player_control_interface_get_type (void);
#define MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE \
  (meego_media_player_control_interface_get_type ())
#define MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE, MeegoMediaPlayerControlInterface))
#define MEEGO_IS_MEDIA_PLAYER_CONTROL_INTERFACE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE))
#define MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE((obj), MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE, MeegoMediaPlayerControlInterfaceClass))


typedef gboolean (*meego_media_player_control_interface_set_uri_impl) (MeegoMediaPlayerControlInterface *self,
    const gchar *in_uri);
void meego_media_player_control_interface_implement_set_uri (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_uri_impl impl);

typedef gboolean (*meego_media_player_control_interface_play_impl) (MeegoMediaPlayerControlInterface *self);
void meego_media_player_control_interface_implement_play (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_play_impl impl);

typedef gboolean (*meego_media_player_control_interface_pause_impl) (MeegoMediaPlayerControlInterface *self);
void meego_media_player_control_interface_implement_pause (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_pause_impl impl);

typedef gboolean (*meego_media_player_control_interface_stop_impl) (MeegoMediaPlayerControlInterface *self);
void meego_media_player_control_interface_implement_stop (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_stop_impl impl);

typedef gboolean (*meego_media_player_control_interface_set_position_impl) (MeegoMediaPlayerControlInterface *self,
    gint64 in_pos);
void meego_media_player_control_interface_implement_set_position (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_position_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_position_impl) (MeegoMediaPlayerControlInterface *self, gint64 *cur_time);
void meego_media_player_control_interface_implement_get_position (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_position_impl impl);

typedef gboolean (*meego_media_player_control_interface_set_playback_rate_impl) (MeegoMediaPlayerControlInterface *self,
    gdouble in_rate);
void meego_media_player_control_interface_implement_set_playback_rate (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_playback_rate_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_playback_rate_impl) (MeegoMediaPlayerControlInterface *self, gdouble *out_rate);
void meego_media_player_control_interface_implement_get_playback_rate (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_playback_rate_impl impl);

typedef gboolean (*meego_media_player_control_interface_set_volume_impl) (MeegoMediaPlayerControlInterface *self,
    gint in_volume);
void meego_media_player_control_interface_implement_set_volume (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_volume_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_volume_impl) (MeegoMediaPlayerControlInterface *self, gint *vol);
void meego_media_player_control_interface_implement_get_volume (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_volume_impl impl);

typedef gboolean (*meego_media_player_control_interface_set_window_id_impl) (MeegoMediaPlayerControlInterface *self,
    gdouble in_win_id);
void meego_media_player_control_interface_implement_set_window_id (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_window_id_impl impl);

typedef gboolean (*meego_media_player_control_interface_set_video_size_impl) (MeegoMediaPlayerControlInterface *self,
    guint in_x,
    guint in_y,
    guint in_w,
    guint in_h);
void meego_media_player_control_interface_implement_set_video_size (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_video_size_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_video_size_impl) (MeegoMediaPlayerControlInterface *self, guint *w, guint *h);
void meego_media_player_control_interface_implement_get_video_size (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_video_size_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_buffered_bytes_impl) (MeegoMediaPlayerControlInterface *self, gint64 *depth);
void meego_media_player_control_interface_implement_get_buffered_bytes (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_buffered_bytes_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_buffered_time_impl) (MeegoMediaPlayerControlInterface *self, gint64 *depth);
void meego_media_player_control_interface_implement_get_buffered_time (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_buffered_time_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_media_size_time_impl) (MeegoMediaPlayerControlInterface *self, gint64 *media_size_time);
void meego_media_player_control_interface_implement_get_media_size_time (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_media_size_time_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_media_size_bytes_impl) (MeegoMediaPlayerControlInterface *self, gint64 *media_size_bytes);
void meego_media_player_control_interface_implement_get_media_size_bytes (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_media_size_bytes_impl impl);

typedef gboolean (*meego_media_player_control_interface_has_audio_impl) (MeegoMediaPlayerControlInterface *self, gboolean *has_audio);
void meego_media_player_control_interface_implement_has_audio (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_has_audio_impl impl);

typedef gboolean (*meego_media_player_control_interface_has_video_impl) (MeegoMediaPlayerControlInterface *self, gboolean *has_video);
void meego_media_player_control_interface_implement_has_video (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_has_video_impl impl);

typedef gboolean (*meego_media_player_control_interface_is_streaming_impl) (MeegoMediaPlayerControlInterface *self, gboolean *is_streaming);
void meego_media_player_control_interface_implement_is_streaming (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_is_streaming_impl impl);

typedef gboolean (*meego_media_player_control_interface_is_seekable_impl) (MeegoMediaPlayerControlInterface *self, gboolean *seekable);
void meego_media_player_control_interface_implement_is_seekable (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_is_seekable_impl impl);

typedef gboolean (*meego_media_player_control_interface_support_fullscreen_impl) (MeegoMediaPlayerControlInterface *self, gboolean *support_fullscreen);
void meego_media_player_control_interface_implement_support_fullscreen (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_support_fullscreen_impl impl);

typedef gboolean (*meego_media_player_control_interface_get_player_state_impl) (MeegoMediaPlayerControlInterface *self, gint *state);
void meego_media_player_control_interface_implement_get_player_state (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_player_state_impl impl);


/*virtual function wrappers*/
gboolean meego_media_player_control_interface_set_uri (MeegoMediaPlayerControlInterface *self, const gchar *in_uri);
gboolean meego_media_player_control_interface_play (MeegoMediaPlayerControlInterface *self);
gboolean meego_media_player_control_interface_pause (MeegoMediaPlayerControlInterface *self);
gboolean meego_media_player_control_interface_stop (MeegoMediaPlayerControlInterface *self);
gboolean meego_media_player_control_interface_set_position (MeegoMediaPlayerControlInterface *self, gint64 in_pos);
gboolean meego_media_player_control_interface_get_position (MeegoMediaPlayerControlInterface *self, gint64 *cur_time);
gboolean meego_media_player_control_interface_set_playback_rate (MeegoMediaPlayerControlInterface *self, gdouble in_rate);
gboolean meego_media_player_control_interface_get_playback_rate (MeegoMediaPlayerControlInterface *self, gdouble *out_rate);
gboolean meego_media_player_control_interface_set_volume (MeegoMediaPlayerControlInterface *self, gint in_volume);
gboolean meego_media_player_control_interface_get_volume (MeegoMediaPlayerControlInterface *self, gint *vol);
gboolean meego_media_player_control_interface_set_window_id (MeegoMediaPlayerControlInterface *self, gdouble in_win_id);
gboolean meego_media_player_control_interface_set_video_size (MeegoMediaPlayerControlInterface *self, guint in_x, guint in_y, guint in_w, guint in_h);
gboolean meego_media_player_control_interface_get_video_size (MeegoMediaPlayerControlInterface *self, guint *w, guint *h);
gboolean meego_media_player_control_interface_get_buffered_bytes (MeegoMediaPlayerControlInterface *self, gint64 *buffered_bytes);
gboolean meego_media_player_control_interface_get_buffered_time (MeegoMediaPlayerControlInterface *self, gint64 *buffered_time);
gboolean meego_media_player_control_interface_get_media_size_time (MeegoMediaPlayerControlInterface *self, gint64 *media_size_time);
gboolean meego_media_player_control_interface_get_media_size_bytes (MeegoMediaPlayerControlInterface *self, gint64 *media_size_bytes);
gboolean meego_media_player_control_interface_has_audio (MeegoMediaPlayerControlInterface *self, gboolean *hav_audio);
gboolean meego_media_player_control_interface_has_video (MeegoMediaPlayerControlInterface *self, gboolean *has_video);
gboolean meego_media_player_control_interface_is_streaming (MeegoMediaPlayerControlInterface *self, gboolean *is_streaming);
gboolean meego_media_player_control_interface_is_seekable (MeegoMediaPlayerControlInterface *self, gboolean *seekable);
gboolean meego_media_player_control_interface_support_fullscreen (MeegoMediaPlayerControlInterface *self, gboolean *support_fullscreen);
gboolean meego_media_player_control_interface_get_player_state (MeegoMediaPlayerControlInterface *self, gint *state);

/*signal emitter*/
void meego_media_player_control_interface_emit_initialized (gpointer instance);
void meego_media_player_control_interface_emit_eof (gpointer instance);
void meego_media_player_control_interface_emit_error (gpointer instance, guint error_num, gchar *error_des);
void meego_media_player_control_interface_emit_seeked (gpointer instance);
void meego_media_player_control_interface_emit_stopped (gpointer instance);
void meego_media_player_control_interface_emit_request_window (gpointer instance);
void meego_media_player_control_interface_emit_buffering (gpointer instance);
void meego_media_player_control_interface_emit_buffered (gpointer instance);
void meego_media_player_control_interface_emit_player_state_changed (gpointer instance, gint cur_state);

G_END_DECLS
#endif
