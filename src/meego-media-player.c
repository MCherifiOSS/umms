#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "umms-debug.h"
#include "umms-common.h"
#include "umms-error.h"
#include "umms-marshals.h"
#include "meego-media-player.h"
#include "meego-media-player-control.h"

G_DEFINE_TYPE (MeegoMediaPlayer, meego_media_player, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerPrivate))

#define GET_PRIVATE(o) ((MeegoMediaPlayer *)o)->priv

#define GET_CONTROL_IFACE(meego_media_player) (((MeegoMediaPlayer *)(meego_media_player))->player_control)

#define CHECK_ENGINE(engine, ret_val, e) \
  do {\
    if (engine == NULL) {\
      g_return_val_if_fail (e == NULL || *e == NULL, ret_val);\
      if (e != NULL) {\
        g_set_error (e, UMMS_ENGINE_ERROR, UMMS_ENGINE_ERROR_NOT_LOADED, \
                     "Pipeline engine not loaded, possible reason is SetUri failed or not be invoked.");\
      } \
      return ret_val;\
    }\
  }while(0)


#define   DEFAULT_VOLUME 50
#define   DEFAULT_X      0
#define   DEFAULT_Y      0
#define   DEFAULT_WIDTH  720
#define   DEFAULT_HIGHT  576


enum {
  PROP_0,
  PROP_NAME,
  PROP_ATTENDED,
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
  SIGNAL_MEDIA_PLAYER_NeedReply,
  SIGNAL_MEDIA_PLAYER_ClientNoReply,
  SIGNAL_MEDIA_PLAYER_TargetReady,
  SIGNAL_MEDIA_PLAYER_Suspended,
  SIGNAL_MEDIA_PLAYER_Restored,
  SIGNAL_MEDIA_PLAYER_NoResource,
  SIGNAL_MEDIA_PLAYER_VideoTagChanged,
  SIGNAL_MEDIA_PLAYER_AudioTagChanged,
  SIGNAL_MEDIA_PLAYER_TextTagChanged,
  SIGNAL_MEDIA_PLAYER_MetadataChanged,
  N_MEDIA_PLAYER_SIGNALS
};

static guint media_player_signals[N_MEDIA_PLAYER_SIGNALS] = {0};

struct _MeegoMediaPlayerPrivate {
  gchar    *name;
  gboolean attended;

  gchar    *uri;
  gint     volume;
  gint     mute;
  gint     scale_mode;
  gint     x;
  gint     y;
  guint    w;
  guint    h;

  //For client existence checking.
  guint    no_reply_time;
  guint    timeout_id;

  //Media player "snapshot", for suspend/restore operation.
  gint64   position;

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
player_state_changed_cb (MeegoMediaPlayerControl *iface, gint old_state, gint new_state, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged], 0, old_state, new_state);
}

static void
target_ready_cb (MeegoMediaPlayerControl *iface, GHashTable *infos, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_TargetReady], 0, infos);
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

static void
suspended_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Suspended], 0);
}

static void
restored_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Restored], 0);
}

static void
video_tag_changed_cb (MeegoMediaPlayerControl *iface, gint channel, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_VideoTagChanged], 0, channel);
}

static void
audio_tag_changed_cb (MeegoMediaPlayerControl *iface, gint channel, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_AudioTagChanged], 0, channel);
}

static void
text_tag_changed_cb (MeegoMediaPlayerControl *iface, gint channel, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_TextTagChanged], 0, channel);
}

static void
metadata_changed_cb (MeegoMediaPlayerControl *iface, MeegoMediaPlayer *player)
{
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_MetadataChanged], 0);
}

static gboolean
client_existence_check (MeegoMediaPlayer *player)
{
  gboolean ret = TRUE;
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

#define CHECK_INTERVAL (500)
#define CLI_REPLY_TIMEOUT (2500)

  if (priv->no_reply_time > CLI_REPLY_TIMEOUT) {
    UMMS_DEBUG ("Client didn't response for '%u' ms, stop this AV execution", priv->no_reply_time);
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_ClientNoReply], 0);
    ret = FALSE;
  } else {
    g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_NeedReply], 0);
    priv->no_reply_time  += CHECK_INTERVAL;
  }
  return ret;
}

gboolean
meego_media_player_reply (MeegoMediaPlayer *player, GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  priv->no_reply_time = 0;

  return TRUE;
}

gboolean
meego_media_player_set_uri (MeegoMediaPlayer *player,
    const gchar           *uri,
    GError **err)
{
  MeegoMediaPlayerClass *kclass = MEEGO_MEDIA_PLAYER_GET_CLASS (player);
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  gboolean new_engine;

  if (priv->uri) {
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);

  if (!kclass->load_engine) {
    UMMS_DEBUG ("virtual method \"load_engine\" not implemented");
    g_set_error (err, UMMS_ENGINE_ERROR, UMMS_ENGINE_ERROR_FAILED, "'load_engine()' not implemented");
    return FALSE;
  }

  if (!kclass->load_engine (player, uri, &new_engine)) {
    UMMS_DEBUG ("Pipeline engine load failed");
    g_set_error (err, UMMS_ENGINE_ERROR, UMMS_ENGINE_ERROR_FAILED, "Pipeline engine load failed");
    return FALSE;
  }

  if (new_engine) {
    g_signal_connect_object (player->player_control, "request-window",
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
    g_signal_connect_object (player->player_control, "target-ready",
        G_CALLBACK (target_ready_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "suspended",
        G_CALLBACK (suspended_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "restored",
        G_CALLBACK (restored_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "video-tag-changed",
        G_CALLBACK (video_tag_changed_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "audio-tag-changed",
        G_CALLBACK (audio_tag_changed_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "text-tag-changed",
        G_CALLBACK (text_tag_changed_cb),
        player,
        0);
    g_signal_connect_object (player->player_control, "metadata-changed",
        G_CALLBACK (metadata_changed_cb),
        player,
        0);
  }

  /* Set all the cached property. */
  meego_media_player_control_set_uri (player->player_control, uri);
  meego_media_player_control_set_volume (player->player_control, priv->volume);
  meego_media_player_control_set_mute (player->player_control, priv->mute);
  meego_media_player_control_set_scale_mode (player->player_control, priv->scale_mode);

  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized], 0);

exit:
  return TRUE;
}

//For convenience's sake, we don't cache the target info if player backend is not ready.
gboolean
meego_media_player_set_target (MeegoMediaPlayer *player, gint type, GHashTable *params,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_target (GET_CONTROL_IFACE (player), type, params);
  return TRUE;
}

gboolean
meego_media_player_play (MeegoMediaPlayer *player,
    GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (player);

  meego_media_player_control_play (player_control);
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
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
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
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (player);

  UMMS_DEBUG ("rectangle=\"%u,%u,%u,%u\"", in_x, in_y, in_w, in_h );

  if (player_control) {
    meego_media_player_control_set_video_size (GET_CONTROL_IFACE (player), in_x, in_y, in_w, in_h);
  } else {
    UMMS_DEBUG ("Cache the video size parameters since pipe engine has not been loaded");
    priv->x = in_x;
    priv->y = in_y;
    priv->w = in_w;
    priv->h = in_h;
  }
  return TRUE;
}


gboolean
meego_media_player_get_video_size(MeegoMediaPlayer *player, guint *w, guint *h,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_size (GET_CONTROL_IFACE (player), w, h);
  return TRUE;
}


gboolean
meego_media_player_is_seekable(MeegoMediaPlayer *player, gboolean *is_seekable, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_is_seekable (GET_CONTROL_IFACE (player), is_seekable);
  return TRUE;
}

gboolean
meego_media_player_set_position(MeegoMediaPlayer *player,
    gint64                 in_pos,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_position (GET_CONTROL_IFACE (player), in_pos);
  return TRUE;
}

gboolean
meego_media_player_get_position(MeegoMediaPlayer *player, gint64 *pos,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_position (GET_CONTROL_IFACE (player), pos);
  return TRUE;
}

gboolean
meego_media_player_set_playback_rate (MeegoMediaPlayer *player,
    gdouble          in_rate,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  return meego_media_player_control_set_playback_rate (GET_CONTROL_IFACE (player), in_rate);
}

gboolean
meego_media_player_get_playback_rate (MeegoMediaPlayer *player,
    gdouble *rate,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_playback_rate (GET_CONTROL_IFACE (player), rate);
  UMMS_DEBUG ("current rate = %f",  *rate);
  return TRUE;
}

gboolean
meego_media_player_set_volume (MeegoMediaPlayer *player,
    gint                  volume,
    GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (player);

  UMMS_DEBUG ("set volume to %d",  volume);

  if (player_control) {
    meego_media_player_control_set_volume (player_control, volume);
  } else {
    UMMS_DEBUG ("Cache the volume since pipe engine has not been loaded");
    priv->volume = volume;
  }

  return TRUE;
}

gboolean
meego_media_player_get_volume (MeegoMediaPlayer *player,
    gint *vol,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_volume (GET_CONTROL_IFACE (player), vol);
  UMMS_DEBUG ("current volume= %d",  *vol);
  return TRUE;
}

gboolean
meego_media_player_get_media_size_time (MeegoMediaPlayer *player,
    gint64 *duration,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_media_size_time(GET_CONTROL_IFACE (player), duration);
  UMMS_DEBUG ("duration = %lld",  *duration);
  return TRUE;
}

gboolean
meego_media_player_get_media_size_bytes (MeegoMediaPlayer *player,
    gint64 *size_bytes,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_media_size_bytes (GET_CONTROL_IFACE (player), size_bytes);
  UMMS_DEBUG ("media size = %lld",  *size_bytes);
  return TRUE;
}

gboolean
meego_media_player_has_video (MeegoMediaPlayer *player,
    gboolean *has_video,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_has_video (GET_CONTROL_IFACE (player), has_video);
  UMMS_DEBUG ("has_video = %d",  *has_video);
  return TRUE;
}

gboolean
meego_media_player_has_audio (MeegoMediaPlayer *player,
    gboolean *has_audio,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_has_audio (GET_CONTROL_IFACE (player), has_audio);
  UMMS_DEBUG ("has_audio= %d",  *has_audio);
  return TRUE;
}

gboolean
meego_media_player_support_fullscreen (MeegoMediaPlayer *player,
    gboolean *support_fullscreen,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_support_fullscreen (GET_CONTROL_IFACE (player), support_fullscreen);
  UMMS_DEBUG ("support_fullscreen = %d",  *support_fullscreen);
  return TRUE;
}

gboolean
meego_media_player_is_streaming (MeegoMediaPlayer *player,
    gboolean *is_streaming,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_is_streaming (GET_CONTROL_IFACE (player), is_streaming);
  UMMS_DEBUG ("is_streaming = %d",  *is_streaming);
  return TRUE;
}

gboolean
meego_media_player_get_player_state(MeegoMediaPlayer *player, gint *state, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_player_state (GET_CONTROL_IFACE (player), state);
  UMMS_DEBUG ("player state = %d",  *state);
  return TRUE;
}

gboolean
meego_media_player_get_buffered_bytes (MeegoMediaPlayer *player, gint64 *bytes, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_buffered_bytes (GET_CONTROL_IFACE (player), bytes);
  UMMS_DEBUG ("buffered bytes = %lld",  *bytes);
  return TRUE;
}

gboolean
meego_media_player_get_buffered_time (MeegoMediaPlayer *player, gint64 *size_time, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_buffered_time (GET_CONTROL_IFACE (player), size_time);
  UMMS_DEBUG ("buffered time = %lld",  *size_time);
  return TRUE;
}

gboolean
meego_media_player_get_current_video (MeegoMediaPlayer *player, gint *cur_video, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_current_video(GET_CONTROL_IFACE (player), cur_video);
  UMMS_DEBUG ("get the current video is %d", *cur_video);
  return TRUE;
}

gboolean
meego_media_player_get_current_audio (MeegoMediaPlayer *player, gint *cur_audio, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_current_audio(GET_CONTROL_IFACE (player), cur_audio);
  UMMS_DEBUG ("get the current audio is %d", *cur_audio);
  return TRUE;
}

gboolean
meego_media_player_set_current_video (MeegoMediaPlayer *player, gint cur_video, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_current_video(GET_CONTROL_IFACE (player), cur_video);
  UMMS_DEBUG ("set the current video to %d", cur_video);
  return TRUE;
}

gboolean
meego_media_player_set_current_audio (MeegoMediaPlayer *player, gint cur_audio, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_current_audio(GET_CONTROL_IFACE (player), cur_audio);
  UMMS_DEBUG ("set the current audio to %d", cur_audio);
  return TRUE;
}

gboolean
meego_media_player_get_video_num (MeegoMediaPlayer *player, gint *video_num, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_num(GET_CONTROL_IFACE (player), video_num);
  UMMS_DEBUG ("the total video number is %d", *video_num);
  return TRUE;
}

gboolean
meego_media_player_get_audio_num (MeegoMediaPlayer *player, gint *audio_num, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_audio_num(GET_CONTROL_IFACE (player), audio_num);
  UMMS_DEBUG ("the total audio number is %d", *audio_num);
  return TRUE;
}

gboolean
meego_media_player_set_proxy (MeegoMediaPlayer *player,
    GHashTable *params,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_proxy (GET_CONTROL_IFACE (player), params);
  return TRUE;
}

//Stop player, so that all resources occupied will be released.
gboolean
meego_media_player_suspend(MeegoMediaPlayer *player,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_suspend (GET_CONTROL_IFACE (player));
  return TRUE;
}

gboolean
meego_media_player_set_subtitle_uri (MeegoMediaPlayer *player, gchar *sub_uri, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  UMMS_DEBUG ("set the subtitle uri to %s", sub_uri);
  meego_media_player_control_set_subtitle_uri(GET_CONTROL_IFACE (player), sub_uri);
  return TRUE;
}

gboolean
meego_media_player_restore (MeegoMediaPlayer *player,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_restore (GET_CONTROL_IFACE (player));
  return TRUE;
}

meego_media_player_get_subtitle_num (MeegoMediaPlayer *player, gint *sub_num, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_subtitle_num(GET_CONTROL_IFACE (player), sub_num);
  UMMS_DEBUG ("the total subtitle number is %d", *sub_num);
  return TRUE;
}

gboolean
meego_media_player_get_current_subtitle (MeegoMediaPlayer *player, gint *cur_sub, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_current_subtitle(GET_CONTROL_IFACE (player), cur_sub);
  UMMS_DEBUG ("get the current subtitle is %d", *cur_sub);
  return TRUE;
}

gboolean
meego_media_player_set_current_subtitle (MeegoMediaPlayer *player, gint cur_sub, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_current_subtitle(GET_CONTROL_IFACE (player), cur_sub);
  UMMS_DEBUG ("set the current subtitle to %d", cur_sub);
  return TRUE;
}

gboolean
meego_media_player_set_buffer_depth (MeegoMediaPlayer *player, gint format, gint64 buf_val, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_set_buffer_depth(GET_CONTROL_IFACE (player), format, buf_val);
  UMMS_DEBUG ("set the format to %d, buffer to %lld", format, buf_val);
  return TRUE;
}

gboolean
meego_media_player_set_mute (MeegoMediaPlayer *player, gint mute, GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (player);

  UMMS_DEBUG ("will set mute to %d", mute);

  if (player_control) {
    meego_media_player_control_set_mute(GET_CONTROL_IFACE (player), mute);
  } else {
    UMMS_DEBUG ("Cache the mute to later loaded.");
    priv->mute = mute;
  }
  return TRUE;
}

gboolean
meego_media_player_is_mute (MeegoMediaPlayer *player, gint *mute, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);

  meego_media_player_control_is_mute (GET_CONTROL_IFACE (player), mute);
  UMMS_DEBUG ("current mute is %d", *mute);
  return TRUE;
}

gboolean
meego_media_player_set_scale_mode (MeegoMediaPlayer *player, gint scale_mode, GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  MeegoMediaPlayerControl *player_control = GET_CONTROL_IFACE (player);

  UMMS_DEBUG ("will set scale mode to %d", scale_mode);

  if (player_control) {
    meego_media_player_control_set_scale_mode (GET_CONTROL_IFACE (player), scale_mode);
  } else {
    UMMS_DEBUG ("Cache the scale mode to later loaded.");
    priv->scale_mode = scale_mode;
  }
  return TRUE;
}

gboolean
meego_media_player_get_scale_mode (MeegoMediaPlayer *player, gint *scale_mode, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_scale_mode(GET_CONTROL_IFACE (player), scale_mode);
  UMMS_DEBUG ("get the current scale mode is %d", *scale_mode);
  return TRUE;
}

gboolean
meego_media_player_get_video_codec (MeegoMediaPlayer *player, gint channel, gchar **video_codec, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_codec(GET_CONTROL_IFACE (player), channel, video_codec);
  UMMS_DEBUG ("We get the video codec: %s", *video_codec);
  return TRUE;
}

gboolean
meego_media_player_get_audio_codec (MeegoMediaPlayer *player, gint channel, gchar **audio_codec, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_audio_codec(GET_CONTROL_IFACE (player), channel, audio_codec);
  UMMS_DEBUG ("We get the audio codec: %s", *audio_codec);
  return TRUE;
}

gboolean
meego_media_player_get_video_bitrate (MeegoMediaPlayer *player, gint channel, gint *bit_rate, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_bitrate(GET_CONTROL_IFACE (player), channel, bit_rate);
  UMMS_DEBUG ("We get the video bitrate: %d", *bit_rate);
  return TRUE;
}

gboolean
meego_media_player_get_audio_bitrate (MeegoMediaPlayer *player, gint channel, gint *bit_rate, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_audio_bitrate(GET_CONTROL_IFACE (player), channel, bit_rate);
  UMMS_DEBUG ("We get the audio:%d bitrate: %d", channel, *bit_rate);
  return TRUE;
}

gboolean
meego_media_player_get_encapsulation (MeegoMediaPlayer *player, gchar **encapsulation, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_encapsulation(GET_CONTROL_IFACE (player), encapsulation);
  UMMS_DEBUG ("We get the encapsulation: %s", *encapsulation);
  return TRUE;
}

gboolean
meego_media_player_get_audio_samplerate (MeegoMediaPlayer *player, gint channel, gint *sample_rate, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_audio_samplerate(GET_CONTROL_IFACE (player), channel, sample_rate);
  UMMS_DEBUG ("We get the sample rate: %d", *sample_rate);
  return TRUE;
}

gboolean
meego_media_player_get_video_framerate (MeegoMediaPlayer *player, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_framerate(GET_CONTROL_IFACE (player),
      channel, frame_rate_num, frame_rate_denom);
  UMMS_DEBUG ("We get the sample rate: %f", *(gdouble*)frame_rate_num / *(gdouble*)frame_rate_denom);
  return TRUE;
}

gboolean
meego_media_player_get_video_resolution (MeegoMediaPlayer *player, gint channel,
    gint * width, gint * height, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_resolution(GET_CONTROL_IFACE (player), channel, width, height);
  UMMS_DEBUG ("We get the video resolution: %d X %d", *width, *height);
  return TRUE;
}

gboolean
meego_media_player_get_video_aspect_ratio (MeegoMediaPlayer *player, gint channel,
    gint * ratio_num, gint * ratio_denom, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_video_aspect_ratio(GET_CONTROL_IFACE (player),
      channel, ratio_num, ratio_denom);
  UMMS_DEBUG ("We get the video aspect ratio: %d : %d", *ratio_num, *ratio_denom);
  return TRUE;
}

gboolean
meego_media_player_get_protocol_name (MeegoMediaPlayer *player, gchar **protocol_name, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_protocol_name(GET_CONTROL_IFACE (player), protocol_name);
  UMMS_DEBUG ("We get the protocol name: %s", protocol_name);
  return TRUE;
}

gboolean
meego_media_player_get_current_uri (MeegoMediaPlayer *player, gchar **uri, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_current_uri(GET_CONTROL_IFACE (player), uri);
  UMMS_DEBUG ("We get the uri: %s", uri);
  return TRUE;
}

gboolean
meego_media_player_get_title (MeegoMediaPlayer *player, gchar **title, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_title(GET_CONTROL_IFACE (player), title);
  return TRUE;
}

gboolean
meego_media_player_get_artist (MeegoMediaPlayer *player, gchar **artist, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_artist(GET_CONTROL_IFACE (player), artist);
  return TRUE;
}
static void
meego_media_player_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_ATTENDED:
      g_value_set_boolean (value, priv->attended);
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

  switch (property_id) {
    case PROP_NAME:
      tmp = g_value_get_string (value);
      UMMS_DEBUG ("name= %s'", tmp);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_ATTENDED:
      priv->attended = g_value_get_boolean (value);
      UMMS_DEBUG ("attended=%d", priv->attended);
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

  if (priv->uri)
    g_free (priv->uri);

  if (priv->name)
    g_free (priv->name);

  if (priv->attended && priv->timeout_id > 0) {
    g_source_remove (priv->timeout_id);
  }


  G_OBJECT_CLASS (meego_media_player_parent_class)->dispose (object);
}

static void
meego_media_player_finalize (GObject *object)
{
  G_OBJECT_CLASS (meego_media_player_parent_class)->finalize (object);
}

static void
meego_media_player_constructed (GObject *player)
{
  MeegoMediaPlayerPrivate *priv;

  priv = GET_PRIVATE (player);

  UMMS_DEBUG ("enter");
  //For client existence checking
  if (priv->attended) {
    UMMS_DEBUG ("Attended execution, add client checking");
    priv->timeout_id = g_timeout_add (CHECK_INTERVAL, (GSourceFunc)client_existence_check, player);
  }
}

static void
meego_media_player_class_init (MeegoMediaPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MeegoMediaPlayerPrivate));

  object_class->get_property = meego_media_player_get_property;
  object_class->set_property = meego_media_player_set_property;
  object_class->constructed = meego_media_player_constructed;
  object_class->dispose = meego_media_player_dispose;
  object_class->finalize = meego_media_player_finalize;

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", "Name", "Name of the mediaplayer",
          NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_ATTENDED,
      g_param_spec_boolean ("attended", "Attended", "Flag to indicate whether this execution is attended",
          TRUE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized] =
    g_signal_new ("initialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Eof] =
    g_signal_new ("eof",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Error] =
    g_signal_new ("error",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
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
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Buffered] =
    g_signal_new ("buffered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_RequestWindow] =
    g_signal_new ("request-window",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Seeked] =
    g_signal_new ("seeked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Stopped] =
    g_signal_new ("stopped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged] =
    g_signal_new ("player-state-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  umms_marshal_VOID__INT_INT,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  media_player_signals[SIGNAL_MEDIA_PLAYER_NeedReply] =
    g_signal_new ("need-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_ClientNoReply] =
    g_signal_new ("client-no-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_TargetReady] =
    g_signal_new ("target-ready",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE));

  media_player_signals[SIGNAL_MEDIA_PLAYER_Suspended] =
    g_signal_new ("suspended",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_Restored] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_NoResource] =
    g_signal_new ("no-resource",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_signals[SIGNAL_MEDIA_PLAYER_VideoTagChanged] =
    g_signal_new ("video-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_signals[SIGNAL_MEDIA_PLAYER_AudioTagChanged] =
    g_signal_new ("audio-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_signals[SIGNAL_MEDIA_PLAYER_TextTagChanged] =
    g_signal_new ("text-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_signals[SIGNAL_MEDIA_PLAYER_MetadataChanged] =
    g_signal_new ("metadata-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
meego_media_player_set_default_params (MeegoMediaPlayer *player)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);
  priv->volume = DEFAULT_VOLUME;
  priv->x = DEFAULT_X;
  priv->y = DEFAULT_Y;
  priv->w = DEFAULT_WIDTH;
  priv->h = DEFAULT_HIGHT;
}

static void
meego_media_player_init (MeegoMediaPlayer *player)
{
  player->priv = PLAYER_PRIVATE (player);
  player->priv->uri = NULL;
  player->player_control = NULL;

  meego_media_player_set_default_params (player);
}



MeegoMediaPlayer *
meego_media_player_new (void)
{
  return g_object_new (MEEGO_TYPE_MEDIA_PLAYER, NULL);
}
