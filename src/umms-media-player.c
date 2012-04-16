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

#include <dbus/dbus-glib.h>
#include "umms-server.h"
#include "umms-debug.h"
#include "umms-types.h"
#include "umms-error.h"
#include "umms-utils.h"
#include "umms-marshals.h"
#include "umms-media-player.h"
#include "umms-backend-factory.h"
#include "umms-player-backend.h"

G_DEFINE_TYPE (UmmsMediaPlayer, umms_media_player, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_MEDIA_PLAYER, UmmsMediaPlayerPrivate))
#define GET_PRIVATE(o) ((UmmsMediaPlayer *)o)->priv
#define GET_BACKEND(o) (((UmmsMediaPlayer *)(o))->priv->backend)
#define CHECK_BACKEND(backend, ret_val, e) \
  do {\
    if (backend == NULL) {\
      g_return_val_if_fail (e == NULL || *e == NULL, ret_val);\
      if (e != NULL) {\
        g_set_error (e, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_NOT_LOADED, \
                     "Pipeline backend not loaded, possible reason is SetUri failed or not be invoked.");\
      } \
      return ret_val;\
    }\
  }while(0)

#define   DEFAULT_SCALE_MODE  ScaleModeKeepAspectRatio
#define   DEFAULT_VOLUME 50
#define   DEFAULT_MUTE   FALSE
#define   DEFAULT_X      0
#define   DEFAULT_Y      0
#define   DEFAULT_WIDTH  0
#define   DEFAULT_HIGHT  0

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
  SIGNAL_MEDIA_PLAYER_Seeked,
  SIGNAL_MEDIA_PLAYER_Stopped,
  SIGNAL_MEDIA_PLAYER_PlayerStateChanged,
  SIGNAL_MEDIA_PLAYER_NeedReply,
  SIGNAL_MEDIA_PLAYER_ClientNoReply,
  SIGNAL_MEDIA_PLAYER_Suspended,
  SIGNAL_MEDIA_PLAYER_Restored,
  SIGNAL_MEDIA_PLAYER_NoResource,
  SIGNAL_MEDIA_PLAYER_VideoTagChanged,
  SIGNAL_MEDIA_PLAYER_AudioTagChanged,
  SIGNAL_MEDIA_PLAYER_TextTagChanged,
  SIGNAL_MEDIA_PLAYER_MetadataChanged,
  SIGNAL_MEDIA_PLAYER_RecordStart,
  SIGNAL_MEDIA_PLAYER_RecordStop,
  N_MEDIA_PLAYER_SIGNALS
};

static guint umms_media_player_signals[N_MEDIA_PLAYER_SIGNALS] = {0};

struct _UmmsMediaPlayerPrivate {
  UmmsPlayerBackend *backend;
  gchar    *name;
  gboolean attended;

  /*Parameters need to be cached due to the unavailable of underlying player.*/
  gchar    *uri;
  gboolean uri_dirty;
  gchar    *sub_uri;

  gint     volume;
  gint     mute;
  gint     scale_mode;
  gint     x;
  gint     y;
  guint    w;
  guint    h;
  gboolean video_size_cached;
  gint        target_type;
  GHashTable *target_params;
  GHashTable *http_proxy_params;

  //For client existence checking.
  guint    no_reply_time;
  guint    timeout_id;

  //Media player "snapshot", for suspend/restore operation.
  gint64   position;
};

static void
umms_media_player_reset_backend (UmmsMediaPlayer *self)
{
  UmmsMediaPlayerPrivate *priv = self->priv;

  if (priv->backend) {
    umms_player_backend_stop (priv->backend, NULL);
    g_object_unref (priv->backend);
    priv->backend = NULL;
  }
}

static void
umms_media_player_set_default_params (UmmsMediaPlayer *player)
{
  UmmsMediaPlayerPrivate *priv = player->priv;

  priv->scale_mode = DEFAULT_SCALE_MODE;
  priv->volume = DEFAULT_VOLUME;
  priv->mute   = DEFAULT_MUTE;
  priv->x = DEFAULT_X;
  priv->y = DEFAULT_Y;
  priv->w = DEFAULT_WIDTH;
  priv->h = DEFAULT_HIGHT;
}

static void
eof_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Eof], 0);
}

static void
error_cb (UmmsPlayerBackend *iface, guint error_num, gchar *error_des, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Error], 0, error_num, error_des);
}

static void
buffering_cb (UmmsPlayerBackend *iface, gint percent, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Buffering], 0, percent);
}

static void
buffered_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Buffered], 0);
}

static void
player_state_changed_cb (UmmsPlayerBackend *iface, gint old_state, gint new_state, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged], 0, old_state, new_state);
}

static void
seeked_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Seeked], 0);
}
static void
stopped_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Stopped], 0);
}

static void
suspended_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Suspended], 0);
}

static void
restored_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Restored], 0);
}

static void
video_tag_changed_cb (UmmsPlayerBackend *iface, gint channel, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_VideoTagChanged], 0, channel);
}

static void
audio_tag_changed_cb (UmmsPlayerBackend *iface, gint channel, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_AudioTagChanged], 0, channel);
}

static void
text_tag_changed_cb (UmmsPlayerBackend *iface, gint channel, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_TextTagChanged], 0, channel);
}

static void
metadata_changed_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_MetadataChanged], 0);
}

static void
record_start_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_RecordStart], 0);
}

static void
record_stop_cb (UmmsPlayerBackend *iface, UmmsMediaPlayer *player)
{
  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_RecordStop], 0);
}

static void
connect_signals(UmmsMediaPlayer *player, UmmsPlayerBackend *backend)
{
  g_signal_connect_object (backend, "eof",
                           G_CALLBACK (eof_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "error",
                           G_CALLBACK (error_cb),
                           player,
                           0);

  g_signal_connect_object (backend, "seeked",
                           G_CALLBACK (seeked_cb),
                           player,
                           0);

  g_signal_connect_object (backend, "stopped",
                           G_CALLBACK (stopped_cb),
                           player,
                           0);

  g_signal_connect_object (backend, "buffering",
                           G_CALLBACK (buffering_cb),
                           player,
                           0);

  g_signal_connect_object (backend, "buffered",
                           G_CALLBACK (buffered_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "player-state-changed",
                           G_CALLBACK (player_state_changed_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "suspended",
                           G_CALLBACK (suspended_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "restored",
                           G_CALLBACK (restored_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "video-tag-changed",
                           G_CALLBACK (video_tag_changed_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "audio-tag-changed",
                           G_CALLBACK (audio_tag_changed_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "text-tag-changed",
                           G_CALLBACK (text_tag_changed_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "metadata-changed",
                           G_CALLBACK (metadata_changed_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "record-start",
                           G_CALLBACK (record_start_cb),
                           player,
                           0);
  g_signal_connect_object (backend, "record-stop",
                           G_CALLBACK (record_stop_cb),
                           player,
                           0);
}

static gboolean
client_existence_check (UmmsMediaPlayer *player)
{
  gboolean ret = TRUE;
  UmmsMediaPlayerPrivate *priv = player->priv;

#define CHECK_INTERVAL (500)
#define CLI_REPLY_TIMEOUT (2500)

  if (priv->no_reply_time > CLI_REPLY_TIMEOUT) {
    UMMS_DEBUG ("Client didn't response for '%u' ms, stop this AV execution", priv->no_reply_time);
    g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_ClientNoReply], 0);
    ret = FALSE;
  } else {
    g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_NeedReply], 0);
    priv->no_reply_time  += CHECK_INTERVAL;
  }
  return ret;
}

gboolean
umms_media_player_reply (UmmsMediaPlayer *player, GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;
  priv->no_reply_time = 0;

  return TRUE;
}

/*
 * Create inner player backend which can handle this uri.
 * Connect signals if needed. Set all the cached properties.
 */
static gboolean
umms_media_player_load_backend (UmmsMediaPlayer *player, const gchar *uri)
{
  UmmsMediaPlayerPrivate *priv = player->priv;

  g_assert (priv->backend == NULL);
  if (!(priv->backend = umms_player_backend_make_from_uri (uri))) {
    UMMS_WARNING ("Failed to create backend");
    return FALSE;
  }

  connect_signals (player, priv->backend);

  /* Set all the cached property. */
  umms_player_backend_set_uri (priv->backend, priv->uri, NULL);
  priv->uri_dirty = FALSE;
  umms_player_backend_set_volume (priv->backend, priv->volume, NULL);
  umms_player_backend_set_mute (priv->backend, priv->mute, NULL);
  umms_player_backend_set_scale_mode (priv->backend, priv->scale_mode, NULL);

  if (priv->video_size_cached)
    umms_player_backend_set_video_size (priv->backend, priv->x, priv->y, priv->w, priv->h, NULL);
  priv->video_size_cached = FALSE;

  if (umms_ctx->proxy_uri) {
    if (priv->http_proxy_params)
      g_hash_table_unref (priv->http_proxy_params);
    priv->http_proxy_params = param_table_create ("proxy-uri", G_TYPE_STRING, umms_ctx->proxy_uri,
                              "proxy-id", G_TYPE_STRING, umms_ctx->proxy_id,
                              "proxy-pw", G_TYPE_STRING, umms_ctx->proxy_pw,
                              NULL);
  }

  if (priv->http_proxy_params)
    umms_player_backend_set_proxy (priv->backend, priv->http_proxy_params, NULL);

  if (priv->target_params)
    umms_player_backend_set_target (priv->backend, priv->target_type, priv->target_params, NULL);
  if (priv->sub_uri)
    umms_player_backend_set_subtitle_uri (priv->backend, priv->sub_uri, NULL);

  g_signal_emit (player, umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized], 0);

  return TRUE;
}

gboolean
umms_media_player_set_uri (UmmsMediaPlayer *player,
                      const gchar           *uri,
                      GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;

  if (!uri || uri[0] == '\0') {
    UMMS_DEBUG ("Invalid URI");
    g_set_error (err, UMMS_GENERIC_ERROR, UMMS_GENERIC_ERROR_INVALID_PARAM, "Invalid URI");
    return FALSE;
  }

  if (priv->uri) {
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);
  priv->uri_dirty = TRUE;
  UMMS_DEBUG ("URI: %s", uri);
  return TRUE;
}

gboolean
umms_media_player_set_target (UmmsMediaPlayer *player, gint type, GHashTable *params, GError **err)
{
  gboolean ret = TRUE;
  UmmsMediaPlayerPrivate *priv = player->priv;

  if (priv->backend) {
    ret = umms_player_backend_set_target (priv->backend, type, params, err);
  } else {
    priv->target_type = type;
    if (priv->target_params)
      g_hash_table_unref (priv->target_params);
    priv->target_params = g_hash_table_ref (params);
  }

  return ret;
}

gboolean umms_media_player_activate (UmmsMediaPlayer *player, PlayerState state, GError **err)
{
  gchar *prot = NULL;
  gboolean ret = TRUE;
  UmmsMediaPlayerPrivate *priv = player->priv;

  UMMS_DEBUG ("setting backend to state: %d ", state);
  if (!priv->uri) {
    UMMS_DEBUG ("No URI specified");
    return FALSE;
  }

  if (priv->backend) {
    prot = uri_get_protocol (priv->uri);
    if (!umms_player_backend_support_prot (priv->backend, prot)) {
      umms_media_player_reset_backend (player);
    }
  }

  if (!priv->backend) {
    if (!umms_media_player_load_backend (player, priv->uri)) {
      g_set_error (err, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_FAILED, "failed to load backend");
      return FALSE;
    }
  }

  if (priv->uri_dirty) {
    //we have a new uri, stop the backend firstly.
    if (!umms_player_backend_stop (priv->backend, err))
      return FALSE;

    if (!umms_player_backend_set_uri (priv->backend, priv->uri, err))
      return FALSE;
    priv->uri_dirty = FALSE;
  }

  switch (state) {
  case PlayerStatePaused:
    ret = umms_player_backend_pause (priv->backend, err);
    break;
  case PlayerStatePlaying:
    ret = umms_player_backend_play (priv->backend, err);
    break;
  default:
    UMMS_DEBUG ("Invalid target state: %d", state);
    ret = FALSE;
    break;
  }
  return ret;
}

gboolean
umms_media_player_play (UmmsMediaPlayer *player,
                   GError **err)
{
  return umms_media_player_activate (player, PlayerStatePlaying, err);
}

gboolean
umms_media_player_pause(UmmsMediaPlayer *player,
                   GError **err)
{
  return umms_media_player_activate (player, PlayerStatePaused, err);
}

gboolean
umms_media_player_stop (UmmsMediaPlayer *player,
                   GError **err)
{
  umms_media_player_reset_backend (player);
  return TRUE;
}

gboolean
umms_media_player_set_video_size(UmmsMediaPlayer *player,
                            guint in_x,
                            guint in_y,
                            guint in_w,
                            guint in_h,
                            GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;
  UmmsPlayerBackend *backend = priv->backend;

  UMMS_DEBUG ("rectangle=\"%u,%u,%u,%u\"", in_x, in_y, in_w, in_h );

  if (backend) {
    umms_player_backend_set_video_size (priv->backend, in_x, in_y, in_w, in_h, err);
  } else {
    UMMS_DEBUG ("Cache the video size parameters since pipe backend has not been loaded");
    priv->x = in_x;
    priv->y = in_y;
    priv->w = in_w;
    priv->h = in_h;
    priv->video_size_cached = TRUE;
  }
  return TRUE;
}


gboolean
umms_media_player_get_video_size(UmmsMediaPlayer *player, guint *w, guint *h,
                            GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_size (player->priv->backend, w, h, err);
}


gboolean
umms_media_player_is_seekable(UmmsMediaPlayer *player, gboolean *is_seekable, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_is_seekable (player->priv->backend, is_seekable, err);
}

gboolean
umms_media_player_set_position(UmmsMediaPlayer *player,
                          gint64                 in_pos,
                          GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_position (player->priv->backend, in_pos, err);
}

gboolean
umms_media_player_get_position(UmmsMediaPlayer *player, gint64 *pos,
                          GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_position (player->priv->backend, pos, err);
}

gboolean
umms_media_player_set_playback_rate (UmmsMediaPlayer *player,
                                gdouble          in_rate,
                                GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_playback_rate (player->priv->backend, in_rate, err);
}

gboolean
umms_media_player_get_playback_rate (UmmsMediaPlayer *player,
                                gdouble *rate,
                                GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_playback_rate (player->priv->backend, rate, err);
}

gboolean
umms_media_player_set_volume (UmmsMediaPlayer *player,
                         gint                  volume,
                         GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;
  UmmsPlayerBackend *backend = priv->backend;
  gboolean ret = TRUE;

  UMMS_DEBUG ("set volume to %d",  volume);

  if (backend) {
    ret = umms_player_backend_set_volume (backend, volume, err);
  } else {
    UMMS_DEBUG ("UmmsMediaPlayer not ready, cache the volume.");
    priv->volume = volume;
  }

  return ret;
}

gboolean
umms_media_player_get_volume (UmmsMediaPlayer *player,
                         gint *vol,
                         GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_volume (player->priv->backend, vol, err);
}

gboolean
umms_media_player_get_media_size_time (UmmsMediaPlayer *player,
                                  gint64 *duration,
                                  GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_media_size_time(player->priv->backend, duration, err);
}

gboolean
umms_media_player_get_media_size_bytes (UmmsMediaPlayer *player,
                                   gint64 *size_bytes,
                                   GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_media_size_bytes (player->priv->backend, size_bytes, err);
}

gboolean
umms_media_player_has_video (UmmsMediaPlayer *player,
                        gboolean *has_video,
                        GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_has_video (player->priv->backend, has_video, err);
}

gboolean
umms_media_player_has_audio (UmmsMediaPlayer *player,
                        gboolean *has_audio,
                        GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_has_audio (player->priv->backend, has_audio, err);
}

gboolean
umms_media_player_support_fullscreen (UmmsMediaPlayer *player,
                                 gboolean *support_fullscreen,
                                 GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_support_fullscreen (player->priv->backend, support_fullscreen, err);
}

gboolean
umms_media_player_is_streaming (UmmsMediaPlayer *player,
                           gboolean *is_streaming,
                           GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_is_streaming (player->priv->backend, is_streaming, err);
}

gboolean
umms_media_player_get_player_state(UmmsMediaPlayer *player, gint *state, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_player_state (player->priv->backend, state, err);
}

gboolean
umms_media_player_get_buffered_bytes (UmmsMediaPlayer *player, gint64 *bytes, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_buffered_bytes (player->priv->backend, bytes, err);
}

gboolean
umms_media_player_get_buffered_time (UmmsMediaPlayer *player, gint64 *size_time, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_buffered_time (player->priv->backend, size_time, err);
}

gboolean
umms_media_player_get_current_video (UmmsMediaPlayer *player, gint *cur_video, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_current_video(player->priv->backend, cur_video, err);
}

gboolean
umms_media_player_get_current_audio (UmmsMediaPlayer *player, gint *cur_audio, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_current_audio(player->priv->backend, cur_audio, err);
}

gboolean
umms_media_player_set_current_video (UmmsMediaPlayer *player, gint cur_video, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_current_video(player->priv->backend, cur_video, err);
}

gboolean
umms_media_player_set_current_audio (UmmsMediaPlayer *player, gint cur_audio, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_current_audio(player->priv->backend, cur_audio, err);
}

gboolean
umms_media_player_get_video_num (UmmsMediaPlayer *player, gint *video_num, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_num(player->priv->backend, video_num, err);
}

gboolean
umms_media_player_get_audio_num (UmmsMediaPlayer *player, gint *audio_num, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_audio_num(player->priv->backend, audio_num, err);
}

gboolean
umms_media_player_set_proxy (UmmsMediaPlayer *player,
                        GHashTable *params,
                        GError **err)
{
  gboolean ret = TRUE;
  UmmsMediaPlayerPrivate *priv = player->priv;

  if (priv->backend) {
    ret = umms_player_backend_set_proxy (priv->backend, params, err);
  } else {
    if (priv->http_proxy_params)
      g_hash_table_unref (priv->http_proxy_params);
    priv->http_proxy_params = g_hash_table_ref (params);
  }

  return ret;
}

//Stop player, so that all resources occupied will be released.
gboolean
umms_media_player_suspend(UmmsMediaPlayer *player,
                     GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_suspend (player->priv->backend, err);
}

gboolean
umms_media_player_set_subtitle_uri (UmmsMediaPlayer *player, gchar *sub_uri, GError **err)
{
  gboolean ret = TRUE;
  UmmsMediaPlayerPrivate *priv = player->priv;

  UMMS_DEBUG ("Want to set the suburi to %s", sub_uri);
  if (priv->backend) {
    ret = umms_player_backend_set_subtitle_uri (priv->backend, sub_uri, err);
  } else {
    RESET_STR (priv->sub_uri);
    UMMS_DEBUG ("cache the suburi %s", sub_uri);
    priv->sub_uri = g_strdup (sub_uri);
  }

  return ret;
}

gboolean
umms_media_player_restore (UmmsMediaPlayer *player,
                      GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_restore (player->priv->backend, err);
}

gboolean
umms_media_player_get_subtitle_num (UmmsMediaPlayer *player, gint *sub_num, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_subtitle_num(player->priv->backend, sub_num, err);
}

gboolean
umms_media_player_get_current_subtitle (UmmsMediaPlayer *player, gint *cur_sub, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_current_subtitle(player->priv->backend, cur_sub, err);
}

gboolean
umms_media_player_set_current_subtitle (UmmsMediaPlayer *player, gint cur_sub, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_current_subtitle(player->priv->backend, cur_sub, err);
}

gboolean
umms_media_player_set_buffer_depth (UmmsMediaPlayer *player, gint format, gint64 buf_val, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_set_buffer_depth(player->priv->backend, format, buf_val, err);
}

gboolean
umms_media_player_get_buffer_depth (UmmsMediaPlayer *player, gint format, gint64 *buf_val, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_buffer_depth(player->priv->backend, format, buf_val, err);
}

gboolean
umms_media_player_set_mute (UmmsMediaPlayer *player, gint mute, GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;
  UmmsPlayerBackend *backend = priv->backend;
  gboolean ret = TRUE;

  UMMS_DEBUG ("will set mute to %d", mute);

  if (backend) {
    ret = umms_player_backend_set_mute(priv->backend, mute, err);
  } else {
    UMMS_DEBUG ("UmmsMediaPlayer not ready, cache the mute.");
    priv->mute = mute;
  }
  return ret;
}

gboolean
umms_media_player_is_mute (UmmsMediaPlayer *player, gint *mute, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_is_mute (player->priv->backend, mute, err);
}

gboolean
umms_media_player_set_scale_mode (UmmsMediaPlayer *player, gint scale_mode, GError **err)
{
  UmmsMediaPlayerPrivate *priv = player->priv;
  UmmsPlayerBackend *backend = priv->backend;
  gboolean ret = TRUE;

  UMMS_DEBUG ("setting scale mode to %d", scale_mode);
  if (backend) {
    ret = umms_player_backend_set_scale_mode (priv->backend, scale_mode, err);
  } else {
    UMMS_DEBUG ("UmmsMediaPlayer not ready, cache the scale mode.");
    priv->scale_mode = scale_mode;
  }
  return ret;
}

gboolean
umms_media_player_get_scale_mode (UmmsMediaPlayer *player, gint *scale_mode, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_scale_mode(player->priv->backend, scale_mode, err);
}

gboolean
umms_media_player_get_video_codec (UmmsMediaPlayer *player, gint channel, gchar **video_codec, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_codec(player->priv->backend, channel, video_codec, err);
}

gboolean
umms_media_player_get_audio_codec (UmmsMediaPlayer *player, gint channel, gchar **audio_codec, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_audio_codec(player->priv->backend, channel, audio_codec, err);
}

gboolean
umms_media_player_get_video_bitrate (UmmsMediaPlayer *player, gint channel, gint *bit_rate, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_bitrate(player->priv->backend, channel, bit_rate, err);
}

gboolean
umms_media_player_get_audio_bitrate (UmmsMediaPlayer *player, gint channel, gint *bit_rate, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_audio_bitrate(player->priv->backend, channel, bit_rate, err);
}

gboolean
umms_media_player_get_encapsulation (UmmsMediaPlayer *player, gchar **encapsulation, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_encapsulation(player->priv->backend, encapsulation, err);
}

gboolean
umms_media_player_get_audio_samplerate (UmmsMediaPlayer *player, gint channel, gint *sample_rate, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_audio_samplerate(player->priv->backend, channel, sample_rate, err);
}

gboolean
umms_media_player_get_video_framerate (UmmsMediaPlayer *player, gint channel,
                                  gint * frame_rate_num, gint * frame_rate_denom, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_framerate(player->priv->backend, channel, frame_rate_num, frame_rate_denom, err);
}

gboolean
umms_media_player_get_video_resolution (UmmsMediaPlayer *player, gint channel,
                                   gint * width, gint * height, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_resolution(player->priv->backend, channel, width, height, err);
}

gboolean
umms_media_player_get_video_aspect_ratio (UmmsMediaPlayer *player, gint channel,
                                     gint * ratio_num, gint * ratio_denom, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_video_aspect_ratio(player->priv->backend, channel, ratio_num, ratio_denom, err);
}

gboolean
umms_media_player_get_protocol_name (UmmsMediaPlayer *player, gchar **protocol_name, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_protocol_name(player->priv->backend, protocol_name, err);
}

gboolean
umms_media_player_get_current_uri (UmmsMediaPlayer *player, gchar **uri, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_current_uri(player->priv->backend, uri, err);
}

gboolean
umms_media_player_get_title (UmmsMediaPlayer *player, gchar **title, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_title(player->priv->backend, title, err);
}

gboolean
umms_media_player_get_artist (UmmsMediaPlayer *player, gchar **artist, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_artist(player->priv->backend, artist, err);
}

gboolean
umms_media_player_record (UmmsMediaPlayer *player, gboolean to_record, gchar *location, GError **err)
{
  gboolean ret;
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  ret = umms_player_backend_record (player->priv->backend, to_record, location, err);
  if (!ret) {
    g_set_error (err, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_FAILED, "Record failed");
  }
  return ret;
}

gboolean
umms_media_player_get_pat (UmmsMediaPlayer *player, GPtrArray **pat, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_pat (player->priv->backend, pat, err);
}

gboolean
umms_media_player_get_pmt (UmmsMediaPlayer *player, guint *program_num, guint *pcr_pid, GPtrArray **stream_info,
                      GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_pmt (player->priv->backend, program_num, pcr_pid, stream_info, err);
}

gboolean
umms_media_player_get_associated_data_channel (UmmsMediaPlayer *player, gchar **ip, gint *port, GError **err)
{
  CHECK_BACKEND(player->priv->backend, FALSE, err);
  return umms_player_backend_get_associated_data_channel (player->priv->backend, ip, port, err);
}

static void
umms_media_player_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  UmmsMediaPlayerPrivate *priv = GET_PRIVATE (object);

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
umms_media_player_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  UmmsMediaPlayerPrivate *priv = GET_PRIVATE (object);
  const gchar *tmp;

  switch (property_id) {
  case PROP_NAME:
    tmp = g_value_get_string (value);
    priv->name = g_value_dup_string (value);
    break;
  case PROP_ATTENDED:
    priv->attended = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_media_player_dispose (GObject *object)
{
  UmmsMediaPlayerPrivate *priv = GET_PRIVATE (object);

  RESET_STR (priv->name);
  RESET_STR (priv->uri);
  RESET_STR (priv->sub_uri);
  if (priv->http_proxy_params)
    g_hash_table_unref (priv->http_proxy_params);
  if (priv->target_params)
    g_hash_table_unref (priv->target_params);

  G_OBJECT_CLASS (umms_media_player_parent_class)->dispose (object);
}

static void
umms_media_player_finalize (GObject *object)
{
  UmmsMediaPlayerPrivate *priv = GET_PRIVATE (object);

  if (priv->backend)
    g_object_unref (G_OBJECT (priv->backend));

  if (priv->attended && priv->timeout_id > 0) {
    g_source_remove (priv->timeout_id);
  }

  G_OBJECT_CLASS (umms_media_player_parent_class)->finalize (object);
}

static void
umms_media_player_constructed (GObject *player)
{
  UmmsMediaPlayerPrivate *priv = GET_PRIVATE (player);

  if (priv->attended) {
    priv->timeout_id = g_timeout_add (CHECK_INTERVAL, (GSourceFunc)client_existence_check, player);
  }
}

static void
umms_media_player_class_init (UmmsMediaPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsMediaPlayerPrivate));

  object_class->get_property = umms_media_player_get_property;
  object_class->set_property = umms_media_player_set_property;
  object_class->constructed = umms_media_player_constructed;
  object_class->dispose = umms_media_player_dispose;
  object_class->finalize = umms_media_player_finalize;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", "Name", "Name of the mediaplayer",
                                       NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_ATTENDED,
                                   g_param_spec_boolean ("attended", "Attended", "Flag to indicate whether this execution is attended",
                                       TRUE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Initialized] =
    g_signal_new ("initialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Eof] =
    g_signal_new ("eof",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Error] =
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

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Buffering] =
    g_signal_new ("buffering",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Buffered] =
    g_signal_new ("buffered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Seeked] =
    g_signal_new ("seeked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Stopped] =
    g_signal_new ("stopped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_PlayerStateChanged] =
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

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_NeedReply] =
    g_signal_new ("need-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_ClientNoReply] =
    g_signal_new ("client-no-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Suspended] =
    g_signal_new ("suspended",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_Restored] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_NoResource] =
    g_signal_new ("no-resource",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_VideoTagChanged] =
    g_signal_new ("video-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_AudioTagChanged] =
    g_signal_new ("audio-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_TextTagChanged] =
    g_signal_new ("text-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_MetadataChanged] =
    g_signal_new ("metadata-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_RecordStart] =
    g_signal_new ("record-start",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_media_player_signals[SIGNAL_MEDIA_PLAYER_RecordStart] =
    g_signal_new ("record-stop",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
umms_media_player_init (UmmsMediaPlayer *player)
{
  UmmsMediaPlayerPrivate *priv;
  priv = player->priv = PLAYER_PRIVATE (player);

  priv->backend = NULL;
  priv->uri     = NULL;
  priv->uri_dirty = FALSE;
  priv->sub_uri = NULL;
  priv->target_params = NULL;
  umms_media_player_set_default_params (player);
}

UmmsMediaPlayer *
umms_media_player_new (void)
{
  return g_object_new (UMMS_TYPE_MEDIA_PLAYER, NULL);
}
