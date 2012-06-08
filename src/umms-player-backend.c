#include <string.h>
#include "umms-debug.h"
#include "umms-plugin.h"
#include "umms-error.h"
#include "umms-utils.h"
#include "umms-player-backend.h"
#include "umms-marshals.h"

G_DEFINE_TYPE (UmmsPlayerBackend, umms_player_backend, G_TYPE_OBJECT);

#define UMMS_PLAYER_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), UMMS_TYPE_PLAYER_BACKEND, UmmsPlayerBackendPrivate))

/* list of URIs that we consider to be live source. */
static gchar *live_src_uri[] = {"mms://", "mmsh://", "rtsp://",
                                "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://", "udp://",
                                NULL
                               };

struct _UmmsPlayerBackendPrivate {
  gint dummy;
};

enum {
  SIGNAL_UMMS_PLAYER_BACKEND_Initialized,
  SIGNAL_UMMS_PLAYER_BACKEND_Eof,
  SIGNAL_UMMS_PLAYER_BACKEND_Error,
  SIGNAL_UMMS_PLAYER_BACKEND_Buffering,
  SIGNAL_UMMS_PLAYER_BACKEND_Buffered,
  SIGNAL_UMMS_PLAYER_BACKEND_Seeked,
  SIGNAL_UMMS_PLAYER_BACKEND_Stopped,
  SIGNAL_UMMS_PLAYER_BACKEND_PlayerStateChanged,
  SIGNAL_UMMS_PLAYER_BACKEND_NeedReply,
  SIGNAL_UMMS_PLAYER_BACKEND_ClientNoReply,
  SIGNAL_UMMS_PLAYER_BACKEND_Suspended,
  SIGNAL_UMMS_PLAYER_BACKEND_Restored,
  SIGNAL_UMMS_PLAYER_BACKEND_NoResource,
  SIGNAL_UMMS_PLAYER_BACKEND_VideoTagChanged,
  SIGNAL_UMMS_PLAYER_BACKEND_AudioTagChanged,
  SIGNAL_UMMS_PLAYER_BACKEND_TextTagChanged,
  SIGNAL_UMMS_PLAYER_BACKEND_MetadataChanged,
  SIGNAL_UMMS_PLAYER_BACKEND_RecordStart,
  SIGNAL_UMMS_PLAYER_BACKEND_RecordStop,
  N_UMMS_PLAYER_BACKEND_SIGNALS
};

static guint umms_player_backend_signals[N_UMMS_PLAYER_BACKEND_SIGNALS] = {0};

static void
umms_player_backend_dispose (GObject *object)
{
  UmmsPlayerBackend *self = UMMS_PLAYER_BACKEND (object);

  umms_player_backend_stop (self, NULL);

  G_OBJECT_CLASS (umms_player_backend_parent_class)->dispose (object);
}

static void
umms_player_backend_finalize (GObject *object)
{
  UmmsPlayerBackend *self = UMMS_PLAYER_BACKEND (object);

  umms_player_backend_release_resource (self);
  RESET_STR(self->uri);
  RESET_STR(self->title);
  RESET_STR(self->artist);
  RESET_STR(self->proxy_uri);
  RESET_STR(self->proxy_id);
  RESET_STR(self->proxy_pw);

  G_OBJECT_CLASS (umms_player_backend_parent_class)->finalize (object);
}

static void
umms_player_backend_class_init (UmmsPlayerBackendClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (UmmsPlayerBackendPrivate));
  gobject_class->dispose = umms_player_backend_dispose;
  gobject_class->finalize = umms_player_backend_finalize;
  //gobject_class->set_property = umms_player_backend_set_property;
  //gobject_class->get_property = umms_player_backend_get_property;

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Initialized] =
    g_signal_new ("initialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Eof] =
    g_signal_new ("eof",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Error] =
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

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Buffering] =
    g_signal_new ("buffering",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Buffered] =
    g_signal_new ("buffered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Seeked] =
    g_signal_new ("seeked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Stopped] =
    g_signal_new ("stopped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_PlayerStateChanged] =
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

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_NeedReply] =
    g_signal_new ("need-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_ClientNoReply] =
    g_signal_new ("client-no-reply",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Suspended] =
    g_signal_new ("suspended",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Restored] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_NoResource] =
    g_signal_new ("no-resource",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_VideoTagChanged] =
    g_signal_new ("video-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_AudioTagChanged] =
    g_signal_new ("audio-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_TextTagChanged] =
    g_signal_new ("text-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_MetadataChanged] =
    g_signal_new ("metadata-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_RecordStart] =
    g_signal_new ("record-start",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_RecordStop] =
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
umms_player_backend_init (UmmsPlayerBackend *self)
{
  self->priv = UMMS_PLAYER_BACKEND_GET_PRIVATE (self);
  self->res_mngr = umms_resource_manager_new ();
}

gboolean
umms_player_backend_set_uri (UmmsPlayerBackend *self,
                             const gchar *uri, GError **err)
{
  UMMS_DEBUG ("old = \"%s\", new = \"%s\"", self->uri, uri);
  if (self->uri) {
    g_free (self->uri);
  }
  self->uri = g_strdup (uri);
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_uri, self->uri, err);
}

gboolean
umms_player_backend_set_target (UmmsPlayerBackend *self,
                                gint type, GHashTable *params, GError **err)
{
  gboolean ret = FALSE;
  UmmsPlayerBackendClass *klass = UMMS_PLAYER_BACKEND_GET_CLASS (self);

  if (klass->set_target) {
    ret = klass->set_target (self, type, params, err);
  } else {
    UMMS_WARNING ("%s: %s\n", __FUNCTION__, get_mesg_str (MSG_NOT_IMPLEMENTED));
    g_set_error (err, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_METHOD_NOT_IMPLEMENTED, get_mesg_str (MSG_NOT_IMPLEMENTED));
  }

  return ret;
}

gboolean
umms_player_backend_play (UmmsPlayerBackend *self, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, play, err);
}

gboolean
umms_player_backend_pause (UmmsPlayerBackend *self, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, pause, err);
}

gboolean
umms_player_backend_stop (UmmsPlayerBackend *self, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, stop, err);
}

gboolean
umms_player_backend_set_position (UmmsPlayerBackend *self,
                                  gint64 in_pos, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_position, in_pos, err);
}

gboolean
umms_player_backend_get_position (UmmsPlayerBackend *self, gint64 *cur_time, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_position, cur_time, err);
}

gboolean
umms_player_backend_set_playback_rate (UmmsPlayerBackend *self,
                                       gdouble in_rate, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_playback_rate, in_rate, err);
}

gboolean
umms_player_backend_get_playback_rate (UmmsPlayerBackend *self, gdouble *out_rate, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_playback_rate, out_rate, err);
}

gboolean
umms_player_backend_set_volume (UmmsPlayerBackend *self,
                                gint in_volume, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_volume, in_volume, err);
}

gboolean
umms_player_backend_get_volume (UmmsPlayerBackend *self, gint *vol, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_volume, vol, err);
}

gboolean
umms_player_backend_set_video_size (UmmsPlayerBackend *self, guint x, guint y, guint w, guint h, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_video_size, x, y, w, h, err);
}

gboolean
umms_player_backend_get_video_size (UmmsPlayerBackend *self, guint *w, guint *h, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_size, w, h, err);
}

gboolean
umms_player_backend_get_buffered_time (UmmsPlayerBackend *self, gint64 *buffered_time, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_buffered_time, buffered_time, err);
}

gboolean
umms_player_backend_get_buffered_bytes (UmmsPlayerBackend *self, gint64 *buffered_time, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_buffered_bytes, buffered_time, err);
}

gboolean
umms_player_backend_get_media_size_time (UmmsPlayerBackend *self, gint64 *media_size_time, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_media_size_time, media_size_time, err);
}

gboolean
umms_player_backend_get_media_size_bytes (UmmsPlayerBackend *self, gint64 *media_size_bytes, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_media_size_bytes, media_size_bytes, err);
}

gboolean
umms_player_backend_has_video (UmmsPlayerBackend *self, gboolean *has_video, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, has_video, has_video, err);
}

gboolean
umms_player_backend_has_audio (UmmsPlayerBackend *self, gboolean *has_audio, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, has_audio, has_audio, err);
}

gboolean
umms_player_backend_is_streaming (UmmsPlayerBackend *self, gboolean *is_streaming, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, is_streaming, is_streaming, err);
}

gboolean
umms_player_backend_is_seekable (UmmsPlayerBackend *self, gboolean *seekable, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, is_seekable, seekable, err);
}

gboolean
umms_player_backend_support_fullscreen (UmmsPlayerBackend *self, gboolean *support_fullscreen, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, support_fullscreen, support_fullscreen, err);
}

gboolean
umms_player_backend_get_player_state (UmmsPlayerBackend *self, gint *state, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_player_state, state, err);
}

gboolean
umms_player_backend_get_current_video (UmmsPlayerBackend *self, gint *cur_video, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_current_video, cur_video, err);
}

gboolean
umms_player_backend_get_current_audio (UmmsPlayerBackend *self, gint *cur_audio, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_current_audio, cur_audio, err);
}

gboolean
umms_player_backend_set_current_video (UmmsPlayerBackend *self, gint cur_video, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_current_video, cur_video, err);
}

gboolean
umms_player_backend_set_current_audio (UmmsPlayerBackend *self, gint cur_audio, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_current_audio, cur_audio, err);
}

gboolean
umms_player_backend_get_video_num (UmmsPlayerBackend *self, gint *video_num, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_num, video_num, err);
}

gboolean
umms_player_backend_get_audio_num (UmmsPlayerBackend *self, gint *audio_num, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_audio_num, audio_num, err);
}

umms_player_backend_set_proxy (UmmsPlayerBackend *self,
                               GHashTable *params, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_proxy, params, err);
}

gboolean
umms_player_backend_set_subtitle_uri (UmmsPlayerBackend *self, gchar *sub_uri, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_subtitle_uri, sub_uri, err);
}

gboolean
umms_player_backend_get_subtitle_num (UmmsPlayerBackend *self, gint *sub_num, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_subtitle_num, sub_num, err);
}

gboolean
umms_player_backend_get_current_subtitle (UmmsPlayerBackend *self, gint *cur_sub, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_current_subtitle, cur_sub, err);
}

gboolean
umms_player_backend_set_current_subtitle (UmmsPlayerBackend *self, gint cur_sub, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_current_subtitle, cur_sub, err);
}

gboolean
umms_player_backend_set_buffer_depth (UmmsPlayerBackend *self, gint format, gint64 buf_val, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_buffer_depth, format, buf_val, err);
}

gboolean
umms_player_backend_get_buffer_depth (UmmsPlayerBackend *self, gint format, gint64 *buf_val, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_buffer_depth, format, buf_val, err);
}

gboolean
umms_player_backend_set_mute (UmmsPlayerBackend *self, gint mute, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_mute, mute, err);
}

gboolean
umms_player_backend_is_mute (UmmsPlayerBackend *self, gint *mute, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, is_mute, mute, err);
}

gboolean umms_player_backend_set_scale_mode (UmmsPlayerBackend *self, gint scale_mode, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, set_scale_mode, scale_mode, err);
}

gboolean umms_player_backend_get_scale_mode (UmmsPlayerBackend *self, gint *scale_mode, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_scale_mode, scale_mode, err);
}

gboolean umms_player_backend_suspend (UmmsPlayerBackend *self, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, suspend, err);
}

gboolean umms_player_backend_restore (UmmsPlayerBackend *self, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, restore, err);
}

gboolean umms_player_backend_get_video_codec (UmmsPlayerBackend *self, gint channel, gchar **video_codec, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_codec, channel, video_codec, err);
}

gboolean umms_player_backend_get_audio_codec (UmmsPlayerBackend *self, gint channel, gchar **audio_codec, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_audio_codec, channel, audio_codec, err);
}

gboolean umms_player_backend_get_video_bitrate (UmmsPlayerBackend *self, gint channel, gint *bit_rate, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_bitrate, channel, bit_rate, err);
}

gboolean umms_player_backend_get_audio_bitrate (UmmsPlayerBackend *self, gint channel, gint *bit_rate, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_audio_bitrate, channel, bit_rate, err);
}

gboolean umms_player_backend_get_encapsulation(UmmsPlayerBackend *self, gchar ** encapsulation, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_encapsulation, encapsulation, err);
}

gboolean umms_player_backend_get_audio_samplerate(UmmsPlayerBackend *self, gint channel,
    gint * sample_rate, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_audio_samplerate, channel, sample_rate, err);
}

gboolean umms_player_backend_get_video_framerate(UmmsPlayerBackend *self, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_framerate, channel, frame_rate_num, frame_rate_denom, err);
}

gboolean umms_player_backend_get_video_resolution(UmmsPlayerBackend *self, gint channel,
    gint * width, gint * height, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_resolution, channel, width, height, err);
}


gboolean umms_player_backend_get_video_aspect_ratio(UmmsPlayerBackend *self,
    gint channel, gint * ratio_num, gint * ratio_denom, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_video_aspect_ratio, channel, ratio_num, ratio_denom, err);
}

gboolean umms_player_backend_get_protocol_name(UmmsPlayerBackend *self, gchar ** prot_name, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_protocol_name, prot_name, err);
}

gboolean umms_player_backend_get_current_uri(UmmsPlayerBackend *self, gchar ** uri, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_current_uri, uri, err);
}

gboolean umms_player_backend_get_title (UmmsPlayerBackend *self, gchar ** title, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_title, title, err);
}

gboolean umms_player_backend_get_artist(UmmsPlayerBackend *self, gchar ** artist, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_artist, artist, err);
}

gboolean umms_player_backend_record (UmmsPlayerBackend *self, gboolean to_record, gchar *location, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, record, to_record, location, err);
}

gboolean umms_player_backend_get_pat (UmmsPlayerBackend *self, GPtrArray **pat, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_pat, pat, err);
}

gboolean umms_player_backend_get_pmt (UmmsPlayerBackend *self, guint *program_num, guint *pcr_pid,
                                      GPtrArray **stream_info, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_pmt, program_num, pcr_pid, stream_info, err);
}

gboolean
umms_player_backend_get_associated_data_channel (UmmsPlayerBackend *self, gchar **ip, gint *port, GError **err)
{
  TYPE_VMETHOD_CALL (PLAYER_BACKEND, get_associated_data_channel, ip, port, err);
}

void
umms_player_backend_emit_initialized (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Initialized],
                 0);
}

void
umms_player_backend_emit_eof (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Eof],
                 0);
}

void
umms_player_backend_emit_error (UmmsPlayerBackend *self,
                                guint error_num, gchar *error_des)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Error],
                 0,
                 error_num, error_des);
}

void
umms_player_backend_emit_buffered (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Buffered],
                 0);
}
void
umms_player_backend_emit_buffering (UmmsPlayerBackend *self, gint percent)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Buffering],
                 0, percent);
}

void
umms_player_backend_emit_player_state_changed (UmmsPlayerBackend *self, gint old_state, gint new_state)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_PlayerStateChanged],
                 0, old_state, new_state);
}

void
umms_player_backend_emit_seeked (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Seeked],
                 0);
}

void
umms_player_backend_emit_stopped (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Stopped],
                 0);
}

void
umms_player_backend_emit_suspended (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Suspended],
                 0);
}

void
umms_player_backend_emit_restored (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_Restored],
                 0);
}

void
umms_player_backend_emit_video_tag_changed (UmmsPlayerBackend *self, gint channel)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_VideoTagChanged],
                 0, channel);
}

void
umms_player_backend_emit_audio_tag_changed (UmmsPlayerBackend *self, gint channel)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));


  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_AudioTagChanged],
                 0, channel);
}

void
umms_player_backend_emit_text_tag_changed (UmmsPlayerBackend *self, gint channel)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));


  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_TextTagChanged],
                 0, channel);
}

void
umms_player_backend_emit_metadata_changed (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));


  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_MetadataChanged],
                 0);
}

void
umms_player_backend_emit_record_start (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));


  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_RecordStart],
                 0);
}

void
umms_player_backend_emit_record_stop (UmmsPlayerBackend *self)
{
  g_return_if_fail (UMMS_IS_PLAYER_BACKEND (self));

  g_signal_emit (self,
                 umms_player_backend_signals[SIGNAL_UMMS_PLAYER_BACKEND_RecordStop],
                 0);
}

void
umms_player_backend_set_plugin (UmmsPlayerBackend *self, UmmsPlugin *plugin)
{


  self->plugin = plugin;
}

gboolean umms_player_backend_support_prot (UmmsPlayerBackend *self, const gchar *prot)
{


  g_return_val_if_fail (self->plugin, FALSE);
  return umms_plugin_support_protocol (self->plugin, prot);
}

void
umms_player_backend_release_resource (UmmsPlayerBackend *self)
{
  GList *g;

  if (!self->res_list)
    return;

  for (g = self->res_list; g; g = g->next) {
    Resource *res = (Resource *) (g->data);
    umms_resource_manager_release_resource (self->res_mngr, res);
  }

  g_list_free (self->res_list);
  self->res_list = NULL;
  return;
}

gboolean
umms_player_backend_is_live_uri (const gchar *uri)
{
  gboolean ret = FALSE;
  gchar ** src = live_src_uri;
  while (*src) {
    if (!g_ascii_strncasecmp (uri, *src, strlen(*src))) {
      ret = TRUE;
      break;
    }
    src++;
  }
  return ret;
}

const gchar *
umms_player_backend_state_get_name (PlayerState state)
{
  switch (state) {
  case PlayerStateNull:
    return "PlayerStateNULL";
  case PlayerStateStopped:
    return "PlayerStateStopped";
  case PlayerStatePaused:
    return "PlayerStatePaused";
  case PlayerStatePlaying:
    return "PlayerStatePlaying";
  default:
    return ("UNKNOWN!");
  }
}
