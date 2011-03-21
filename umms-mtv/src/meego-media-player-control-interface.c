#include "umms-marshals.h"
#include "meego-media-player-control-interface.h"

struct _MeegoMediaPlayerControlInterfaceClass {
    GTypeInterface parent_class;
    meego_media_player_control_interface_set_uri_impl set_uri;
    meego_media_player_control_interface_play_impl play;
    meego_media_player_control_interface_pause_impl pause;
    meego_media_player_control_interface_stop_impl stop;
    meego_media_player_control_interface_set_position_impl set_position;
    meego_media_player_control_interface_get_position_impl get_position;
    meego_media_player_control_interface_set_playback_rate_impl set_playback_rate;
    meego_media_player_control_interface_get_playback_rate_impl get_playback_rate;
    meego_media_player_control_interface_set_volume_impl set_volume;
    meego_media_player_control_interface_get_volume_impl get_volume;
    meego_media_player_control_interface_set_window_id_impl set_window_id;
    meego_media_player_control_interface_set_video_size_impl set_video_size;
    meego_media_player_control_interface_get_video_size_impl get_video_size;
    meego_media_player_control_interface_get_buffered_time_impl get_buffered_time;
    meego_media_player_control_interface_get_buffered_bytes_impl get_buffered_bytes;
    meego_media_player_control_interface_get_media_size_time_impl get_media_size_time;
    meego_media_player_control_interface_get_media_size_bytes_impl get_media_size_bytes;
    meego_media_player_control_interface_has_video_impl has_video;
    meego_media_player_control_interface_has_audio_impl has_audio;
    meego_media_player_control_interface_is_streaming_impl is_streaming;
    meego_media_player_control_interface_is_seekable_impl is_seekable;
    meego_media_player_control_interface_support_fullscreen_impl support_fullscreen;
    meego_media_player_control_interface_get_player_state_impl get_player_state;
};

enum {
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Initialized,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_EOF,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Error,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Seeked,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Stopped,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_RequestWindow,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffering,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffered,
    SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_PlayerStateChanged,
    N_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_SIGNALS
};
static guint meego_media_player_control_interface_signals[N_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_SIGNALS] = {0};

static void meego_media_player_control_interface_base_init (gpointer klass);

    GType
meego_media_player_control_interface_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        static const GTypeInfo info = {
            sizeof (MeegoMediaPlayerControlInterfaceClass),
            meego_media_player_control_interface_base_init, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            0,
            0, /* n_preallocs */
            NULL /* instance_init */
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                "MeegoMediaPlayerControlInterface", &info, 0);
    }

    return type;
}

    gboolean
meego_media_player_control_interface_set_uri (MeegoMediaPlayerControlInterface *self,
        const gchar *in_uri)
{
    meego_media_player_control_interface_set_uri_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_uri);

    if (impl != NULL)
    {
        (impl) (self,
                in_uri);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}

void meego_media_player_control_interface_implement_set_uri (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_uri_impl impl)
{
    klass->set_uri = impl;
}

    gboolean
meego_media_player_control_interface_play (MeegoMediaPlayerControlInterface *self)
{
    meego_media_player_control_interface_play_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->play);

    if (impl != NULL)
    {
        (impl) (self);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_play (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_play_impl impl)
{
    klass->play = impl;
}

    gboolean
meego_media_player_control_interface_pause (MeegoMediaPlayerControlInterface *self)
{
    meego_media_player_control_interface_pause_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->pause);

    if (impl != NULL)
    {
        (impl) (self);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_pause (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_pause_impl impl)
{
    klass->pause = impl;
}

    gboolean
meego_media_player_control_interface_stop (MeegoMediaPlayerControlInterface *self)
{
    meego_media_player_control_interface_stop_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->stop);

    if (impl != NULL)
    {
        (impl) (self);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_stop (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_stop_impl impl)
{
    klass->stop = impl;
}


    gboolean
meego_media_player_control_interface_set_position (MeegoMediaPlayerControlInterface *self,
        gint64 in_pos)
{
    meego_media_player_control_interface_set_position_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_position);

    if (impl != NULL)
    {
        (impl) (self,
                in_pos);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_set_position (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_position_impl impl)
{
    klass->set_position = impl;
}

    gboolean
meego_media_player_control_interface_get_position (MeegoMediaPlayerControlInterface *self, gint64 *cur_time)
{
    meego_media_player_control_interface_get_position_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_position);

    if (impl != NULL)
    {
        (impl) (self, cur_time);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_position (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_position_impl impl)
{
    klass->get_position = impl;
}


    gboolean
meego_media_player_control_interface_set_playback_rate (MeegoMediaPlayerControlInterface *self,
        gdouble in_rate)
{
    meego_media_player_control_interface_set_playback_rate_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_playback_rate);

    if (impl != NULL)
    {
        (impl) (self,
                in_rate);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_set_playback_rate (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_playback_rate_impl impl)
{
    klass->set_playback_rate = impl;
}

    gboolean
meego_media_player_control_interface_get_playback_rate (MeegoMediaPlayerControlInterface *self, gdouble *out_rate)
{
    meego_media_player_control_interface_get_playback_rate_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_playback_rate);

    if (impl != NULL)
    {
        (impl) (self, out_rate);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_playback_rate (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_playback_rate_impl impl)
{
    klass->get_playback_rate = impl;
}


    gboolean
meego_media_player_control_interface_set_volume (MeegoMediaPlayerControlInterface *self,
        gint in_volume)
{
    meego_media_player_control_interface_set_volume_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_volume);

    if (impl != NULL)
    {
        (impl) (self,
                in_volume);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_set_volume (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_volume_impl impl)
{
    klass->set_volume = impl;
}

    gboolean
meego_media_player_control_interface_get_volume (MeegoMediaPlayerControlInterface *self, gint *vol)
{
    meego_media_player_control_interface_get_volume_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_volume);

    if (impl != NULL)
    {
        (impl) (self, vol);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_volume (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_volume_impl impl)
{
    klass->get_volume = impl;
}


    gboolean
meego_media_player_control_interface_set_window_id (MeegoMediaPlayerControlInterface *self,
        gdouble in_win_id)
{
    meego_media_player_control_interface_set_window_id_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_window_id);

    if (impl != NULL)
    {
        (impl) (self,
                in_win_id);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_set_window_id (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_window_id_impl impl)
{
    klass->set_window_id = impl;
}





    gboolean
meego_media_player_control_interface_set_video_size (MeegoMediaPlayerControlInterface *self,
        guint in_x,
        guint in_y,
        guint in_w,
        guint in_h)
{
    meego_media_player_control_interface_set_video_size_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->set_video_size);

    if (impl != NULL)
    {
        (impl) (self,
                in_x,
                in_y,
                in_w,
                in_h);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_set_video_size (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_set_video_size_impl impl)
{
    klass->set_video_size = impl;
}

    gboolean
meego_media_player_control_interface_get_video_size (MeegoMediaPlayerControlInterface *self, guint *w, guint *h)
{
    meego_media_player_control_interface_get_video_size_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_video_size);

    if (impl != NULL)
    {
        (impl) (self, w, h);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_video_size (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_video_size_impl impl)
{
    klass->get_video_size = impl;
}

    gboolean
meego_media_player_control_interface_get_buffered_time (MeegoMediaPlayerControlInterface *self, gint64 *buffered_time)
{
    meego_media_player_control_interface_get_buffered_time_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_buffered_time);

    if (impl != NULL)
    {
        (impl) (self, buffered_time);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_buffered_time (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_buffered_time_impl impl)
{
    klass->get_buffered_time = impl;
}

    gboolean
meego_media_player_control_interface_get_buffered_bytes (MeegoMediaPlayerControlInterface *self, gint64 *buffered_time)
{
    meego_media_player_control_interface_get_buffered_bytes_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_buffered_bytes);

    if (impl != NULL)
    {
        (impl) (self, buffered_time);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_buffered_bytes (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_buffered_bytes_impl impl)
{
    klass->get_buffered_bytes = impl;
}

    gboolean
meego_media_player_control_interface_get_media_size_time (MeegoMediaPlayerControlInterface *self, gint64 *media_size_time)
{
    meego_media_player_control_interface_get_media_size_time_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_media_size_time);

    if (impl != NULL)
    {
        (impl) (self, media_size_time);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_media_size_time (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_media_size_time_impl impl)
{
    klass->get_media_size_time = impl;
}

    gboolean
meego_media_player_control_interface_get_media_size_bytes (MeegoMediaPlayerControlInterface *self, gint64 *media_size_bytes)
{
    meego_media_player_control_interface_get_media_size_bytes_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_media_size_bytes);

    if (impl != NULL)
    {
        (impl) (self, media_size_bytes);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_get_media_size_bytes (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_media_size_bytes_impl impl)
{
    klass->get_media_size_bytes = impl;
}

    gboolean
meego_media_player_control_interface_has_video (MeegoMediaPlayerControlInterface *self, gboolean *has_video)
{
    meego_media_player_control_interface_has_video_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->has_video);

    if (impl != NULL)
    {
        (impl) (self, has_video);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_has_video (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_has_video_impl impl)
{
    klass->has_video = impl;
}

    gboolean
meego_media_player_control_interface_has_audio (MeegoMediaPlayerControlInterface *self, gboolean *has_audio)
{
    meego_media_player_control_interface_has_audio_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->has_audio);

    if (impl != NULL)
    {
        (impl) (self, has_audio);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_has_audio (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_has_audio_impl impl)
{
    klass->has_audio = impl;
}

    gboolean
meego_media_player_control_interface_is_streaming (MeegoMediaPlayerControlInterface *self, gboolean *is_streaming)
{
    meego_media_player_control_interface_is_streaming_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->is_streaming);

    if (impl != NULL)
    {
        (impl) (self, is_streaming);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_is_streaming (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_is_streaming_impl impl)
{
    klass->is_streaming = impl;
}

    gboolean
meego_media_player_control_interface_is_seekable (MeegoMediaPlayerControlInterface *self, gboolean *seekable)
{
    meego_media_player_control_interface_is_seekable_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->is_seekable);

    if (impl != NULL)
    {
        (impl) (self, seekable);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_is_seekable (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_is_seekable_impl impl)
{
    klass->is_seekable = impl;
}

    gboolean
meego_media_player_control_interface_support_fullscreen (MeegoMediaPlayerControlInterface *self, gboolean *support_fullscreen)
{
    meego_media_player_control_interface_support_fullscreen_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->support_fullscreen);

    if (impl != NULL)
    {
        (impl) (self, support_fullscreen);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


void meego_media_player_control_interface_implement_support_fullscreen (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_support_fullscreen_impl impl)
{
    klass->support_fullscreen = impl;
}

    gboolean
meego_media_player_control_interface_get_player_state (MeegoMediaPlayerControlInterface *self, gint *state)
{
    meego_media_player_control_interface_get_player_state_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_GET_CLASS (self)->get_player_state);

    if (impl != NULL)
    {
        (impl) (self, state);
    }
    else
    {
        g_warning ("Method not implemented\n");
    }
    return TRUE;
}


    void
meego_media_player_control_interface_implement_get_player_state (MeegoMediaPlayerControlInterfaceClass *klass, meego_media_player_control_interface_get_player_state_impl impl)
{
    klass->get_player_state = impl;
}
    void
meego_media_player_control_interface_emit_initialized (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Initialized],
            0);
}
    void
meego_media_player_control_interface_emit_eof (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_EOF],
            0);
}

    void
meego_media_player_control_interface_emit_error (gpointer instance,
        guint error_num, gchar *error_des)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Error],
            0,
            error_num, error_des);
}

    void
meego_media_player_control_interface_emit_buffered (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffered],
            0);
}
    void
meego_media_player_control_interface_emit_buffering (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffering],
            0);
}

    void
meego_media_player_control_interface_emit_player_state_changed (gpointer instance, gint player_state)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_PlayerStateChanged],
            0, player_state);
}
    void
meego_media_player_control_interface_emit_seeked (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Seeked],
            0);
}
    void
meego_media_player_control_interface_emit_stopped (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Stopped],
            0);
}

    void
meego_media_player_control_interface_emit_request_window (gpointer instance)
{
    g_assert (instance != NULL);
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL_INTERFACE));
    g_signal_emit (instance,
            meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_RequestWindow],
            0);
}

    static inline void
meego_media_player_control_interface_base_init_once (gpointer klass G_GNUC_UNUSED)
{

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Initialized] =
        g_signal_new ("initialized",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_EOF] =
        g_signal_new ("eof",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Error] =
        g_signal_new ("error",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                umms_marshal_VOID__UINT_STRING,
                G_TYPE_NONE,
                2,
                G_TYPE_UINT, 
                G_TYPE_STRING);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Seeked] =
        g_signal_new ("seeked",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Stopped] =
        g_signal_new ("stopped",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_RequestWindow] =
        g_signal_new ("request-window",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffering] =
        g_signal_new ("buffering",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_Buffered] =
        g_signal_new ("buffered",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    meego_media_player_control_interface_signals[SINGLA_MEEGO_MEDIA_PLAYER_CONTROL_INTERFACE_PlayerStateChanged] =
        g_signal_new ("player-state-changed",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__INT,
                G_TYPE_NONE,
                1,
                G_TYPE_INT);

}

    static void
meego_media_player_control_interface_base_init (gpointer klass)
{
    static gboolean initialized = FALSE;

    if (!initialized)
    {
        initialized = TRUE;
        meego_media_player_control_interface_base_init_once (klass);
    }
}
