#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "umms-common.h"
#include "meego-media-player.h"
#include "umms-ginterface.h"
#include "meego-media-player-control-interface.h"

static void umms_media_player_iface_init (UmmsMediaPlayerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MeegoMediaPlayer, meego_media_player, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (UMMS_TYPE_MEDIA_PLAYER_IFACE, umms_media_player_iface_init))

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerPrivate))

#define GET_PRIVATE(o) ((MeegoMediaPlayer *)o)->priv

#define GET_CONTROL_IFACE(meego_media_player) (((MeegoMediaPlayer *)(meego_media_player))->player_control)


    struct _MeegoMediaPlayerPrivate
{
    gint dummy;
};

    static void
request_window_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_request_window (player);
}

    static void
eof_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_eof (player);
}
    static void
error_cb (MeegoMediaPlayerControlInterface *iface, guint error_num, gchar *error_des, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_error (player, error_num, error_des);
}

    static void
buffering_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_buffering (player);
}

    static void
buffered_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_buffered (player);
}

    static void
player_state_changed_cb (MeegoMediaPlayerControlInterface *iface, gint state, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_player_state_changed (player, state);
}

    static void
seeked_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_seeked (player);
}
    static void
stopped_cb (MeegoMediaPlayerControlInterface *iface, MeegoMediaPlayer *player)
{
    umms_media_player_iface_emit_stopped (player);
}


    static void
meego_media_player_set_uri (UmmsMediaPlayerIface   *iface,
                            const gchar           *uri,
                            DBusGMethodInvocation *context)
{
//  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (iface);
    MeegoMediaPlayer *player = MEEGO_MEDIA_PLAYER (iface);
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

    meego_media_player_control_interface_set_uri (player->player_control, uri);
    umms_media_player_iface_emit_initialized (player);

exit:
    umms_media_player_iface_return_from_set_uri (context);
}

    static void
meego_media_player_play (UmmsMediaPlayerIface   *player,
                         DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_play (GET_CONTROL_IFACE (player));
    umms_media_player_iface_return_from_play (context);
}

    static void 
meego_media_player_pause(UmmsMediaPlayerIface   *player, 
                         DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_pause (GET_CONTROL_IFACE (player));
    umms_media_player_iface_return_from_pause(context);
}

    static void 
meego_media_player_stop (UmmsMediaPlayerIface   *player, 
                         DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_stop (GET_CONTROL_IFACE (player));
    umms_media_player_iface_return_from_stop (context);
}

    static void
meego_media_player_set_window_id (UmmsMediaPlayerIface *player,
                                  gdouble window_id,
                                  DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_set_window_id (GET_CONTROL_IFACE (player), window_id);
    umms_media_player_iface_return_from_set_window_id(context);
}

    static void 
meego_media_player_set_video_size(UmmsMediaPlayerIface   *player,
                                  guint in_x, 
                                  guint in_y, 
                                  guint in_w, 
                                  guint in_h,
                                  DBusGMethodInvocation *context)
{
    UMMS_DEBUG ("rectangle=\"%u,%u,%u,%u\"", in_x, in_y, in_w, in_h ); 
    meego_media_player_control_interface_set_video_size (GET_CONTROL_IFACE (player), in_x, in_y, in_w, in_h);
    umms_media_player_iface_return_from_set_video_size(context);
}


    static void 
meego_media_player_get_video_size(UmmsMediaPlayerIface   *player, 
                                  DBusGMethodInvocation *context)
{
    guint w, h;
    meego_media_player_control_interface_get_video_size (GET_CONTROL_IFACE (player), &w, &h);
    umms_media_player_iface_return_from_get_video_size(context, w, h);
}


    static void 
meego_media_player_is_seekable(UmmsMediaPlayerIface   *player, DBusGMethodInvocation *context)
{
    gboolean seekable;
    meego_media_player_control_interface_is_seekable (GET_CONTROL_IFACE (player), &seekable);
    umms_media_player_iface_return_from_is_seekable(context, seekable);
}

    static void 
meego_media_player_set_position(UmmsMediaPlayerIface   *player, 
                                gint64                 in_pos, 
                                DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_set_position (GET_CONTROL_IFACE (player), in_pos);
    umms_media_player_iface_return_from_set_position(context);
}

    static void 
meego_media_player_get_position(UmmsMediaPlayerIface   *player, 
                                DBusGMethodInvocation *context)
{
    gint64 cur_pos;
    meego_media_player_control_interface_get_position (GET_CONTROL_IFACE (player), &cur_pos);
    umms_media_player_iface_return_from_get_position(context, cur_pos);
}

    static void 
meego_media_player_set_playback_rate (UmmsMediaPlayerIface   *player, 
                                      gdouble               in_rate, 
                                      DBusGMethodInvocation *context)
{
    meego_media_player_control_interface_set_playback_rate (GET_CONTROL_IFACE (player), in_rate);
    umms_media_player_iface_return_from_set_playback_rate (context);
}

    static void 
meego_media_player_get_playback_rate (UmmsMediaPlayerIface   *player, 
                                      DBusGMethodInvocation *context)
{
    gdouble cur_rate;
    meego_media_player_control_interface_get_playback_rate (GET_CONTROL_IFACE (player), &cur_rate);
    UMMS_DEBUG ("%s:current rate = %f", __FUNCTION__, cur_rate); 
    umms_media_player_iface_return_from_get_playback_rate (context, cur_rate);
}

    static void 
meego_media_player_set_volume (UmmsMediaPlayerIface   *player, 
                               gint                  volume, 
                               DBusGMethodInvocation *context)
{
    UMMS_DEBUG ("%s:set volume to %d", __FUNCTION__, volume); 
    meego_media_player_control_interface_set_volume (GET_CONTROL_IFACE (player), volume);
    umms_media_player_iface_return_from_set_volume (context);
}

    static void 
meego_media_player_get_volume (UmmsMediaPlayerIface   *player, 
                               DBusGMethodInvocation *context)
{
    gint cur_volume;
    meego_media_player_control_interface_get_volume (GET_CONTROL_IFACE (player), &cur_volume);
    UMMS_DEBUG ("%s:current volume= %d", __FUNCTION__, cur_volume); 
    umms_media_player_iface_return_from_get_volume (context, cur_volume);
}

    static void 
meego_media_player_get_media_size_time (UmmsMediaPlayerIface   *player, 
                                        DBusGMethodInvocation *context)
{
    gint64 duration;
    meego_media_player_control_interface_get_media_size_time(GET_CONTROL_IFACE (player), &duration);
    UMMS_DEBUG ("%s:duration = %lld", __FUNCTION__, duration); 
    umms_media_player_iface_return_from_get_media_size_time (context, duration);
}

    static void 
meego_media_player_get_media_size_bytes (UmmsMediaPlayerIface   *player, 
                                         DBusGMethodInvocation *context)
{
    gint64 size;
    meego_media_player_control_interface_get_media_size_bytes (GET_CONTROL_IFACE (player), &size);
    UMMS_DEBUG ("%s:media size = %lld", __FUNCTION__, size); 
    umms_media_player_iface_return_from_get_media_size_bytes (context, size);
}

    static void 
meego_media_player_has_video (UmmsMediaPlayerIface   *player, 
                              DBusGMethodInvocation *context)
{
    gboolean has_video;
    meego_media_player_control_interface_has_video (GET_CONTROL_IFACE (player), &has_video);
    UMMS_DEBUG ("%s:has_video = %d", __FUNCTION__, has_video); 
    umms_media_player_iface_return_from_has_video (context, has_video);
}

    static void 
meego_media_player_has_audio (UmmsMediaPlayerIface   *player, 
                              DBusGMethodInvocation *context)
{
    gboolean has_audio;
    meego_media_player_control_interface_has_audio (GET_CONTROL_IFACE (player), &has_audio);
    UMMS_DEBUG ("%s:has_audio= %d", __FUNCTION__, has_audio); 
    umms_media_player_iface_return_from_has_audio (context, has_audio);
}

    static void 
meego_media_player_support_fullscreen (UmmsMediaPlayerIface   *player, 
                                       DBusGMethodInvocation *context)
{
    gboolean support_fullscreen;
    meego_media_player_control_interface_support_fullscreen (GET_CONTROL_IFACE (player), &support_fullscreen);
    UMMS_DEBUG ("%s:support_fullscreen = %d", __FUNCTION__, support_fullscreen); 
    umms_media_player_iface_return_from_support_fullscreen (context, support_fullscreen);
}

    static void 
meego_media_player_is_streaming (UmmsMediaPlayerIface   *player, 
                                 DBusGMethodInvocation *context)
{
    gboolean is_streaming;
    meego_media_player_control_interface_is_streaming (GET_CONTROL_IFACE (player), &is_streaming);
    UMMS_DEBUG ("%s:is_streaming = %d", __FUNCTION__, is_streaming); 
    umms_media_player_iface_return_from_is_streaming (context, is_streaming);
}

static void meego_media_player_get_player_state(UmmsMediaPlayerIface   *player, DBusGMethodInvocation *context)
{
    gint state;

    meego_media_player_control_interface_get_player_state (GET_CONTROL_IFACE (player), &state);
    UMMS_DEBUG ("%s:player state = %d", __FUNCTION__, state); 
    umms_media_player_iface_return_from_get_player_state(context, state);
}

static void meego_media_player_get_buffered_bytes (UmmsMediaPlayerIface   *player, DBusGMethodInvocation *context)
{
    gint64 bytes;

    meego_media_player_control_interface_get_buffered_bytes (GET_CONTROL_IFACE (player), &bytes);
    UMMS_DEBUG ("%s:buffered bytes = %lld", __FUNCTION__, bytes); 
    umms_media_player_iface_return_from_get_buffered_bytes (context, bytes);
}

static void meego_media_player_get_buffered_time (UmmsMediaPlayerIface   *player, DBusGMethodInvocation *context)
{
    gint64 size_time;

    meego_media_player_control_interface_get_buffered_time (GET_CONTROL_IFACE (player), &size_time);
    UMMS_DEBUG ("%s:buffered time = %lld", __FUNCTION__, size_time); 
    umms_media_player_iface_return_from_get_buffered_time (context, size_time);
}

    static void
umms_media_player_iface_init (UmmsMediaPlayerIface *iface)
{
    UmmsMediaPlayerIfaceClass *klass = (UmmsMediaPlayerIfaceClass *)iface;

    umms_media_player_iface_implement_set_uri (klass, meego_media_player_set_uri);
    umms_media_player_iface_implement_play (klass, meego_media_player_play);
    umms_media_player_iface_implement_pause (klass, meego_media_player_pause);
    umms_media_player_iface_implement_stop (klass, meego_media_player_stop);
    umms_media_player_iface_implement_set_position (klass, meego_media_player_set_position);
    umms_media_player_iface_implement_get_position (klass, meego_media_player_get_position);
    umms_media_player_iface_implement_set_playback_rate (klass, meego_media_player_set_playback_rate);
    umms_media_player_iface_implement_get_playback_rate (klass, meego_media_player_get_playback_rate);
    umms_media_player_iface_implement_set_volume (klass, meego_media_player_set_volume);
    umms_media_player_iface_implement_get_volume (klass, meego_media_player_get_volume);
    umms_media_player_iface_implement_set_video_size (klass, meego_media_player_set_video_size);
    umms_media_player_iface_implement_get_video_size (klass, meego_media_player_get_video_size);
    umms_media_player_iface_implement_get_buffered_time (klass, meego_media_player_get_buffered_time);
    umms_media_player_iface_implement_get_buffered_bytes (klass, meego_media_player_get_buffered_bytes);
    umms_media_player_iface_implement_get_media_size_time (klass, meego_media_player_get_media_size_time);
    umms_media_player_iface_implement_get_media_size_bytes (klass, meego_media_player_get_media_size_bytes);
    umms_media_player_iface_implement_has_video (klass, meego_media_player_has_video);
    umms_media_player_iface_implement_has_audio (klass, meego_media_player_has_audio);
    umms_media_player_iface_implement_is_streaming (klass, meego_media_player_is_streaming);
    umms_media_player_iface_implement_is_seekable (klass, meego_media_player_is_seekable);
    umms_media_player_iface_implement_support_fullscreen (klass, meego_media_player_support_fullscreen);
    umms_media_player_iface_implement_get_player_state (klass, meego_media_player_get_player_state);
    umms_media_player_iface_implement_set_window_id (klass, meego_media_player_set_window_id);
}

    static void
meego_media_player_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    switch (property_id)
    {
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
    switch (property_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

    static void
meego_media_player_dispose (GObject *object)
{
    MeegoMediaPlayerControlInterface *player_control = GET_CONTROL_IFACE (object);
    if (player_control) {
        g_object_unref (player_control);
    }

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
}

    static void
meego_media_player_init (MeegoMediaPlayer *self)
{
    self->priv = PLAYER_PRIVATE (self);
}

    MeegoMediaPlayer *
meego_media_player_new (void)
{
    return g_object_new (MEEGO_TYPE_MEDIA_PLAYER, NULL);
}

