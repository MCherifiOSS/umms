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

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <dbus/dbus-glib.h>

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

#define   DEFAULT_SCALE_MODE  ScaleModeKeepAspectRatio
#define   DEFAULT_VOLUME 50
#define   DEFAULT_MUTE   FALSE
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
  gchar    *sub_uri;

  //member fields to stroe default params
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

  gint        target_type;
  GHashTable *target_params;
};

static void
connect_signals(MeegoMediaPlayer *player, MeegoMediaPlayerControl *control);

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

/* Create inner player engine which can handle this uri, and set all the cached properties*/
gboolean meego_media_player_load_engine (MeegoMediaPlayer *player, const gchar *uri, gboolean *new_engine)
{
  MeegoMediaPlayerClass *kclass = MEEGO_MEDIA_PLAYER_GET_CLASS (player);
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (!kclass->load_engine) {
    UMMS_DEBUG ("virtual method \"load_engine\" not implemented");
    return FALSE;
  }

  if (!kclass->load_engine (player, uri, new_engine)) {
    UMMS_DEBUG ("loading pipeline engine failed");
    return FALSE;
  }

  if (*new_engine) {
    connect_signals (player, player->player_control);
  }

  /* Set all the cached property. */
  meego_media_player_control_set_uri (player->player_control, priv->uri);
  meego_media_player_control_set_volume (player->player_control, priv->volume);
  meego_media_player_control_set_mute (player->player_control, priv->mute);
  meego_media_player_control_set_scale_mode (player->player_control, priv->scale_mode);
  if (priv->target_params)
    meego_media_player_control_set_target (player->player_control, priv->target_type, priv->target_params);
  if (priv->sub_uri)
    meego_media_player_control_set_subtitle_uri (GET_CONTROL_IFACE (player), priv->sub_uri);

  UMMS_DEBUG ("new: %d", *new_engine);
  g_signal_emit (player, media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized], 0);
  return TRUE;
}

gboolean
meego_media_player_set_uri (MeegoMediaPlayer *player,
    const gchar           *uri,
    GError **err)
{
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (!uri || uri[0] == '\0') {
    UMMS_DEBUG ("Invalid URI");
    g_set_error (err, UMMS_GENERIC_ERROR, UMMS_GENERIC_ERROR_INVALID_PARAM, "Invalid URI");
    return FALSE;
  }

  if (priv->uri) {
    meego_media_player_stop (player, NULL);
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);
  UMMS_DEBUG ("URI: %s", uri);
  return TRUE;
}

//For convenience's sake, we don't cache the target info if player backend is not ready.
gboolean
meego_media_player_set_target (MeegoMediaPlayer *player, gint type, GHashTable *params,
    GError **err)
{
  gboolean ret = TRUE;
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (player->player_control) {
    ret = meego_media_player_control_set_target (GET_CONTROL_IFACE (player), type, params);
  } else {
    priv->target_type = type;
    if (priv->target_params)
      g_hash_table_unref (priv->target_params);
    priv->target_params = g_hash_table_ref (params);
  }

  return ret;
}

gboolean meego_media_player_activate (MeegoMediaPlayer *player, PlayerState state)
{

  gboolean ret = TRUE;
  gboolean new_engine = FALSE;
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (!priv->uri) {
    UMMS_DEBUG ("No URI specified");
    return FALSE;
  }

  if (!player->player_control) {
    if (!meego_media_player_load_engine (player, priv->uri, &new_engine)) {
      return FALSE;
    }
  }

  switch (state) {
    case PlayerStatePaused:
      ret = meego_media_player_control_pause (player->player_control);
      break;
    case PlayerStatePlaying:
      ret = meego_media_player_control_play (player->player_control);
      break;
    default:
      UMMS_DEBUG ("Invalid target state: %d", state);
      ret = FALSE;
      break;
  }

  UMMS_DEBUG ("setting state: %d %s", state, (ret ? "succeed" : "failed"));
  return ret;
}

gboolean
meego_media_player_play (MeegoMediaPlayer *player,
    GError **err)
{
  return meego_media_player_activate (player, PlayerStatePlaying);
}

gboolean
meego_media_player_pause(MeegoMediaPlayer *player,
    GError **err)
{
  return meego_media_player_activate (player, PlayerStatePaused);
}

gboolean
meego_media_player_stop (MeegoMediaPlayer *player,
    GError **err)
{
  if (player->player_control) {
    meego_media_player_control_stop (player->player_control);
    g_object_unref (player->player_control);
    player->player_control = NULL;
  }
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
    UMMS_DEBUG ("MediaPlayer not ready, cache the volume.");
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
  gboolean ret = TRUE;
  MeegoMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (player->player_control) {
    ret = meego_media_player_control_set_subtitle_uri (GET_CONTROL_IFACE (player), sub_uri);
  } else {
    RESET_STR (priv->sub_uri);
    UMMS_DEBUG ("cache the suburi %s", sub_uri);
    priv->sub_uri = g_strdup (sub_uri);
  }

  return ret;
}

gboolean
meego_media_player_restore (MeegoMediaPlayer *player,
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_restore (GET_CONTROL_IFACE (player));
  return TRUE;
}

gboolean
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
meego_media_player_get_buffer_depth (MeegoMediaPlayer *player, gint format, gint64 *buf_val, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_buffer_depth(GET_CONTROL_IFACE (player), format, buf_val);
  UMMS_DEBUG ("set the format to %d, buffer to %lld", format, *buf_val);
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
    UMMS_DEBUG ("MediaPlayer not ready, cache the mute.");
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
    UMMS_DEBUG ("MediaPlayer not ready, cache the scale mode.");
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
  UMMS_DEBUG ("We get the protocol name: %s", *protocol_name);
  return TRUE;
}

gboolean
meego_media_player_get_current_uri (MeegoMediaPlayer *player, gchar **uri, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_current_uri(GET_CONTROL_IFACE (player), uri);
  UMMS_DEBUG ("We get the uri: %s", *uri);
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

  if (priv->target_params)
    g_hash_table_unref (priv->target_params);

  RESET_STR (priv->uri);
  RESET_STR (priv->sub_uri);
  RESET_STR (priv->name);

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

  priv->scale_mode = DEFAULT_SCALE_MODE;
  priv->volume = DEFAULT_VOLUME;
  priv->mute   = DEFAULT_MUTE;
  priv->x = DEFAULT_X;
  priv->y = DEFAULT_Y;
  priv->w = DEFAULT_WIDTH;
  priv->h = DEFAULT_HIGHT;
}

static void
meego_media_player_init (MeegoMediaPlayer *player)
{
  MeegoMediaPlayerPrivate *priv;
  priv = player->priv = PLAYER_PRIVATE (player);

  player->player_control = NULL;
  priv->uri     = NULL;
  priv->sub_uri = NULL;
  priv->target_params = NULL;
  meego_media_player_set_default_params (player);
}

MeegoMediaPlayer *
meego_media_player_new (void)
{
  return g_object_new (MEEGO_TYPE_MEDIA_PLAYER, NULL);
}

static void
connect_signals(MeegoMediaPlayer *player, MeegoMediaPlayerControl *control)
{
  g_signal_connect_object (control, "request-window",
      G_CALLBACK (request_window_cb),
      player,
      0);
  g_signal_connect_object (control, "eof",
      G_CALLBACK (eof_cb),
      player,
      0);
  g_signal_connect_object (control, "error",
      G_CALLBACK (error_cb),
      player,
      0);

  g_signal_connect_object (control, "seeked",
      G_CALLBACK (seeked_cb),
      player,
      0);

  g_signal_connect_object (control, "stopped",
      G_CALLBACK (stopped_cb),
      player,
      0);

  g_signal_connect_object (control, "buffering",
      G_CALLBACK (buffering_cb),
      player,
      0);

  g_signal_connect_object (control, "buffered",
      G_CALLBACK (buffered_cb),
      player,
      0);
  g_signal_connect_object (control, "player-state-changed",
      G_CALLBACK (player_state_changed_cb),
      player,
      0);
  g_signal_connect_object (control, "target-ready",
      G_CALLBACK (target_ready_cb),
      player,
      0);
  g_signal_connect_object (control, "suspended",
      G_CALLBACK (suspended_cb),
      player,
      0);
  g_signal_connect_object (control, "restored",
      G_CALLBACK (restored_cb),
      player,
      0);
  g_signal_connect_object (control, "video-tag-changed",
      G_CALLBACK (video_tag_changed_cb),
      player,
      0);
  g_signal_connect_object (control, "audio-tag-changed",
      G_CALLBACK (audio_tag_changed_cb),
      player,
      0);
  g_signal_connect_object (control, "text-tag-changed",
      G_CALLBACK (text_tag_changed_cb),
      player,
      0);
  g_signal_connect_object (control, "metadata-changed",
      G_CALLBACK (metadata_changed_cb),
      player,
      0);
}

gboolean 
meego_media_player_record (MeegoMediaPlayer *player, gboolean to_record, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_record (GET_CONTROL_IFACE (player), to_record);
  return TRUE;
}

gboolean 
meego_media_player_get_pat (MeegoMediaPlayer *player, GPtrArray **pat, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_pat (GET_CONTROL_IFACE (player), pat);
  return TRUE;
}

gboolean 
meego_media_player_get_pmt (MeegoMediaPlayer *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info, 
    GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_pmt (GET_CONTROL_IFACE (player), program_num, pcr_pid, stream_info);
  return TRUE;
}

gboolean 
meego_media_player_get_associated_data_channel (MeegoMediaPlayer *player, gchar **ip, gint *port, GError **err)
{
  CHECK_ENGINE(GET_CONTROL_IFACE (player), FALSE, err);
  meego_media_player_control_get_associated_data_channel (GET_CONTROL_IFACE (player), ip, port);
  return TRUE;
}
