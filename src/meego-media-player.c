#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "umms-common.h"
#include "umms-marshals.h"
#include "meego-media-player.h"
#include "meego-media-player-control.h"

G_DEFINE_TYPE (MeegoMediaPlayer, meego_media_player, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerPrivate))

#define GET_PRIVATE(o) ((MeegoMediaPlayer *)o)->priv

#define GET_CONTROL_IFACE(meego_media_player) (((MeegoMediaPlayer *)(meego_media_player))->player_control)
#define MEEGO_MEDIA_PLAYER_DEBUG(x...) g_debug (G_STRLOC ": "x)

enum 
{
	PROP_0,
	PROP_NAME,
	PROP_UNATTENDED,
	PROP_LAST
};

enum {
    SIGNAL_MEDIA_PLAYER_Initialized,
    SIGNAL_MEDIA_PLAYER_Eof,
    SIGNAL_MEDIA_PLAYER_Error,
    SIGNAL_MEDIA_PLAYER_Buffering,
    SIGNAL_MEDIA_PLAYER_Buffered,
    SIGNAL_MEDIA_PLAYER_RequestWindow,
    SIGNAL_MEDIA_PLAYER_Seeked,
    SIGNAL_MEDIA_PLAYER_Stopped,
    SIGNAL_MEDIA_PLAYER_PlayerStateChanged,
    N_MEDIA_PLAYER_SIGNALS
};

static guint media_player_signals[N_MEDIA_PLAYER_SIGNALS] = {0};

    struct _MeegoMediaPlayerPrivate
{
	gchar    *name;
    gboolean unattended;

    gboolean volume_cached;
    gint     volume;
};

    static void
request_window_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_RequestWindow], 0);
    
}

    static void
eof_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Eof], 0);
}
    static void
error_cb (MeegoMediaPlayerControl *iface, guint error_num, gchar *error_des, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Error], 0, error_num, error_des);
}

    static void
buffering_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Buffering], 0);
}

    static void
buffered_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Buffered], 0);
}

    static void
player_state_changed_cb (MeegoMediaPlayerControl *iface, gint state, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged], 0, state);
}

    static void
seeked_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Seeked], 0);
}
    static void
stopped_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Stopped], 0);
}


gboolean
meego_media_player_set_uri (MeegoMediaPlayer *player,
                            const gchar           *uri,
                            GError **err)
{
    MeegoMediaPlayerClass *kclass = MEEGO_MEDIA_PLAYER_GET_CLASS (player);
    gboolean new_engine;


    if (!kclass->load_engine) {
        UMMS_DEBUG ("virtual method \"load_engine\" not implemented");
        goto exit;
    }

    if (!kclass->load_engine (player, uri, &new_engine)){
        UMMS_DEBUG ("backend engine load failed");
        goto exit;
    }

    if(new_engine) {
        g_signal_connect_object (player->player_control, "request_window",
                G_CALLBACK (request_window_cb),
                player,
                0);
        g_signal_connect_object (player->player_control, "eof",
                G_CALLBACK (eof_cb),
                player,
                0);
        g_signal_connect_object (player->player_control, "error",
                G_CALLBACK (error_cb),
                player,
                0);

        g_signal_connect_object (player->player_control, "seeked",
                G_CALLBACK (seeked_cb),
                player,
                0);

        g_signal_connect_object (player->player_control, "stopped",
                G_CALLBACK (stopped_cb),
                player,
                0);

        g_signal_connect_object (player->player_control, "buffering",
                G_CALLBACK (buffering_cb),
                player,
                0);

        g_signal_connect_object (player->player_control, "buffered",
                G_CALLBACK (buffered_cb),
                player,
                0);
        g_signal_connect_object (player->player_control, "player-state-changed",
                G_CALLBACK (player_state_changed_cb),
                player,
                0);
    }

    meego_media_player_control_set_uri (player->player_control, uri);
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized], 0);

exit:
    return TRUE;
}

gboolean  
meego_media_player_play (MeegoMediaPlayer *player,
                         GError **err)
{
    MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
    MeegoMediaPlayerControl *control = GET_CONTROL_IFACE (player);

    if (priv->volume_cached) {
        UMMS_DEBUG ("%s:set the cached volume(%d) to pipe engine", __FUNCTION__, priv->volume); 
        meego_media_player_control_set_volume (control, priv->volume);
        priv->volume_cached = FALSE;
        priv->volume = -1;
    }
    meego_media_player_control_play (control);
    return TRUE;
}

    gboolean
meego_media_player_pause(MeegoMediaPlayer *player, 
                         GError **err)
{
    meego_media_player_control_pause (GET_CONTROL_IFACE (player));
    return TRUE;
}

    gboolean
meego_media_player_stop (MeegoMediaPlayer *player, 
                         GError **err)
{
    meego_media_player_control_stop (GET_CONTROL_IFACE (player));
    return TRUE;
}

   gboolean 
meego_media_player_set_window_id (MeegoMediaPlayer *player,
                                  gdouble window_id,
                                  GError **err)
{
    meego_media_player_control_set_window_id (GET_CONTROL_IFACE (player), window_id);
    return TRUE;
}

    gboolean
meego_media_player_set_video_size(MeegoMediaPlayer *player,
                                  guint in_x, 
                                  guint in_y, 
                                  guint in_w, 
                                  guint in_h,
                                  GError **err)
{
    UMMS_DEBUG ("rectangle=\"%u,%u,%u,%u\"", in_x, in_y, in_w, in_h ); 
    meego_media_player_control_set_video_size (GET_CONTROL_IFACE (player), in_x, in_y, in_w, in_h);
    return TRUE;
}


    gboolean
meego_media_player_get_video_size(MeegoMediaPlayer *player, guint *w, guint *h,
                                  GError **err)
{
    meego_media_player_control_get_video_size (GET_CONTROL_IFACE (player), w, h);
    return TRUE;
}


    gboolean
meego_media_player_is_seekable(MeegoMediaPlayer *player, gboolean *is_seekable, GError **err)
{
    meego_media_player_control_is_seekable (GET_CONTROL_IFACE (player), is_seekable);
    return TRUE;
}

    gboolean
meego_media_player_set_position(MeegoMediaPlayer *player, 
                                gint64                 in_pos, 
                                GError **err)
{
    meego_media_player_control_set_position (GET_CONTROL_IFACE (player), in_pos);
    return TRUE;
}

    gboolean
meego_media_player_get_position(MeegoMediaPlayer *player, gint64 *pos,
                                GError **err)
{
    meego_media_player_control_get_position (GET_CONTROL_IFACE (player), pos);
    return TRUE;
}

    gboolean
meego_media_player_set_playback_rate (MeegoMediaPlayer *player, 
                                      gdouble          in_rate, 
                                      GError **err)
{
    meego_media_player_control_set_playback_rate (GET_CONTROL_IFACE (player), in_rate);
    return TRUE;
}

    gboolean
meego_media_player_get_playback_rate (MeegoMediaPlayer *player, 
                                      gdouble *rate,
                                      GError **err)
{
    meego_media_player_control_get_playback_rate (GET_CONTROL_IFACE (player), rate);
    UMMS_DEBUG ("%s:current rate = %f", __FUNCTION__, *rate); 
    return TRUE;
}

    gboolean
meego_media_player_set_volume (MeegoMediaPlayer *player, 
                               gint                  volume, 
                               GError **err)
{
    MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
    MeegoMediaPlayerControl *control = GET_CONTROL_IFACE (player);

    UMMS_DEBUG ("%s:set volume to %d", __FUNCTION__, volume); 

    if (control) {
        meego_media_player_control_set_volume (control, volume);
    } else {
        UMMS_DEBUG ("%s:cache the volume since pipe engine has not been loaded", __FUNCTION__); 
        priv->volume_cached = TRUE;
        priv->volume = volume;
    }

    return TRUE;
}

    gboolean
meego_media_player_get_volume (MeegoMediaPlayer *player, 
                               gint *vol,
                               GError **err)
{
    meego_media_player_control_get_volume (GET_CONTROL_IFACE (player), vol);
    UMMS_DEBUG ("%s:current volume= %d", __FUNCTION__, *vol); 
    return TRUE;
}

    gboolean
meego_media_player_get_media_size_time (MeegoMediaPlayer *player, 
                                        gint64 *duration,
                                        GError **err)
{
    meego_media_player_control_get_media_size_time(GET_CONTROL_IFACE (player), duration);
    UMMS_DEBUG ("%s:duration = %lld", __FUNCTION__, *duration); 
    return TRUE;
}

    gboolean
meego_media_player_get_media_size_bytes (MeegoMediaPlayer *player, 
                                         gint64 *size_bytes,
                                         GError **err)
{
    meego_media_player_control_get_media_size_bytes (GET_CONTROL_IFACE (player), size_bytes);
    UMMS_DEBUG ("%s:media size = %lld", __FUNCTION__, *size_bytes); 
    return TRUE;
}

    gboolean
meego_media_player_has_video (MeegoMediaPlayer *player, 
                              gboolean *has_video,
                              GError **err)
{
    meego_media_player_control_has_video (GET_CONTROL_IFACE (player), has_video);
    UMMS_DEBUG ("%s:has_video = %d", __FUNCTION__, *has_video); 
    return TRUE;
}

    gboolean
meego_media_player_has_audio (MeegoMediaPlayer *player, 
                              gboolean *has_audio,
                              GError **err)
{
    meego_media_player_control_has_audio (GET_CONTROL_IFACE (player), has_audio);
    UMMS_DEBUG ("%s:has_audio= %d", __FUNCTION__, *has_audio); 
    return TRUE;
}

    gboolean
meego_media_player_support_fullscreen (MeegoMediaPlayer *player, 
                                       gboolean *support_fullscreen,
                                       GError **err)
{
    meego_media_player_control_support_fullscreen (GET_CONTROL_IFACE (player), support_fullscreen);
    UMMS_DEBUG ("%s:support_fullscreen = %d", __FUNCTION__, *support_fullscreen); 
    return TRUE;
}

    gboolean
meego_media_player_is_streaming (MeegoMediaPlayer *player, 
                                 gboolean *is_streaming,
                                 GError **err)
{
    meego_media_player_control_is_streaming (GET_CONTROL_IFACE (player), is_streaming);
    UMMS_DEBUG ("%s:is_streaming = %d", __FUNCTION__, *is_streaming); 
    return TRUE;
}

gboolean meego_media_player_get_player_state(MeegoMediaPlayer *player, gint *state, GError **err)
{
    meego_media_player_control_get_player_state (GET_CONTROL_IFACE (player), state);
    UMMS_DEBUG ("%s:player state = %d", __FUNCTION__, *state); 
    return TRUE;
}

gboolean meego_media_player_get_buffered_bytes (MeegoMediaPlayer *player, gint64 *bytes, GError **err)
{
    meego_media_player_control_get_buffered_bytes (GET_CONTROL_IFACE (player), bytes);
    UMMS_DEBUG ("%s:buffered bytes = %lld", __FUNCTION__, *bytes); 
    return TRUE;
}

gboolean meego_media_player_get_buffered_time (MeegoMediaPlayer *player, gint64 *size_time, GError **err)
{
    meego_media_player_control_get_buffered_time (GET_CONTROL_IFACE (player), size_time);
    UMMS_DEBUG ("%s:buffered time = %lld", __FUNCTION__, *size_time); 
    return TRUE;
}


    static void
meego_media_player_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
	MeegoMediaPlayerPrivate *priv = GET_PRIVATE (object);

    switch (property_id)
    {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_UNATTENDED:
			g_value_set_boolean (value, priv->unattended);
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

    static void
meego_media_player_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	MeegoMediaPlayerPrivate *priv = GET_PRIVATE (object);
	const gchar *tmp;

    switch (property_id)
	{
		case PROP_NAME:
			tmp = g_value_get_string (value);
		    MEEGO_MEDIA_PLAYER_DEBUG ("name='%p : %s'", tmp, tmp);	
			priv->name = g_value_dup_string (value);
			break;
		case PROP_UNATTENDED:
			priv->unattended = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

    static void
meego_media_player_dispose (GObject *object)
{
	MeegoMediaPlayerPrivate *priv = GET_PRIVATE (object);
    MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (object);

    if (player_control) {
        meego_media_player_control_stop (player_control);
        g_object_unref (player_control);
    }

	if (priv->name)
		g_free (priv->name);

    G_OBJECT_CLASS (meego_media_player_parent_class)->dispose (object);
}

    static void
meego_media_player_finalize (GObject *object)
{
    G_OBJECT_CLASS (meego_media_player_parent_class)->finalize (object);
}

    static void
meego_media_player_class_init (MeegoMediaPlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MeegoMediaPlayerPrivate));

    object_class->get_property = meego_media_player_get_property;
    object_class->set_property = meego_media_player_set_property;
    object_class->dispose = meego_media_player_dispose;
    object_class->finalize = meego_media_player_finalize;

    g_object_class_install_property (object_class, PROP_NAME,
            g_param_spec_string ("name", "Name", "Name of the mediaplayer",
                NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class, PROP_UNATTENDED,
            g_param_spec_boolean ("unattended", "Unattended", "Flag to indicate whether this execution is unattended",
                FALSE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized] =
        g_signal_new ("initialized",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_Eof] =
        g_signal_new ("eof",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_Error] =
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

    media_player_signals[SIGNAL_MEDIA_PLAYER_Buffering] =
        g_signal_new ("buffering",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_Buffered] =
        g_signal_new ("buffered",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_RequestWindow] =
        g_signal_new ("request-window",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_Seeked] =
        g_signal_new ("seeked",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_Stopped] =
        g_signal_new ("stopped",
                G_OBJECT_CLASS_TYPE (klass),
                G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

    media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged] =
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
meego_media_player_init (MeegoMediaPlayer *player)
{
    player->priv = PLAYER_PRIVATE (player);
    player->priv->volume = -1;
}

    MeegoMediaPlayer *
meego_media_player_new (void)
{
    return g_object_new (MEEGO_TYPE_MEDIA_PLAYER, NULL);
}

