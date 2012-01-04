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
#include "umms-marshals.h"
#include "media-player-control.h"

struct _MediaPlayerControlClass {
  GTypeInterface parent_class;
  media_player_control_set_uri_impl set_uri;
  media_player_control_set_target_impl set_target;
  media_player_control_play_impl play;
  media_player_control_pause_impl pause;
  media_player_control_stop_impl stop;
  media_player_control_set_position_impl set_position;
  media_player_control_get_position_impl get_position;
  media_player_control_set_playback_rate_impl set_playback_rate;
  media_player_control_get_playback_rate_impl get_playback_rate;
  media_player_control_set_volume_impl set_volume;
  media_player_control_get_volume_impl get_volume;
  media_player_control_set_window_id_impl set_window_id;
  media_player_control_set_video_size_impl set_video_size;
  media_player_control_get_video_size_impl get_video_size;
  media_player_control_get_buffered_time_impl get_buffered_time;
  media_player_control_get_buffered_bytes_impl get_buffered_bytes;
  media_player_control_get_media_size_time_impl get_media_size_time;
  media_player_control_get_media_size_bytes_impl get_media_size_bytes;
  media_player_control_has_video_impl has_video;
  media_player_control_has_audio_impl has_audio;
  media_player_control_is_streaming_impl is_streaming;
  media_player_control_is_seekable_impl is_seekable;
  media_player_control_support_fullscreen_impl support_fullscreen;
  media_player_control_get_player_state_impl get_player_state;
  media_player_control_get_current_video_impl get_current_video;
  media_player_control_get_current_audio_impl get_current_audio;
  media_player_control_set_current_video_impl set_current_video;
  media_player_control_set_current_audio_impl set_current_audio;
  media_player_control_get_video_num_impl get_video_num;
  media_player_control_get_audio_num_impl get_audio_num;
  media_player_control_set_proxy_impl set_proxy;
  media_player_control_set_subtitle_uri_impl set_subtitle_uri;
  media_player_control_get_audio_num_impl get_subtitle_num;
  media_player_control_get_current_subtitle_impl get_current_subtitle;
  media_player_control_set_current_subtitle_impl set_current_subtitle;
  media_player_control_set_buffer_depth_impl set_buffer_depth;
  media_player_control_get_buffer_depth_impl get_buffer_depth;
  media_player_control_set_mute_impl set_mute;
  media_player_control_is_mute_impl is_mute;
  media_player_control_set_scale_mode_impl set_scale_mode;
  media_player_control_get_scale_mode_impl get_scale_mode;
  media_player_control_suspend_impl suspend;
  media_player_control_restore_impl restore;
  media_player_control_get_video_codec_impl get_video_codec;
  media_player_control_get_audio_codec_impl get_audio_codec;
  media_player_control_get_video_bitrate_impl get_video_bitrate;
  media_player_control_get_audio_bitrate_impl get_audio_bitrate;
  media_player_control_get_encapsulation_impl get_encapsulation;
  media_player_control_get_audio_samplerate_impl get_audio_samplerate;
  media_player_control_get_video_framerate_impl get_video_framerate;
  media_player_control_get_video_resolution_impl get_video_resolution;
  media_player_control_get_video_aspect_ratio_impl get_video_aspect_ratio;
  media_player_control_get_protocol_name_impl get_protocol_name;
  media_player_control_get_current_uri_impl get_current_uri;
  media_player_control_get_title_impl get_title;
  media_player_control_get_artist_impl get_artist;
  media_player_control_record_impl record;
  media_player_control_get_pat_impl get_pat;
  media_player_control_get_pmt_impl get_pmt;
  media_player_control_get_associated_data_channel_impl get_associated_data_channel;
};

enum {
  SIGNAL_MEDIA_PLAYER_CONTROL_Initialized,
  SIGNAL_MEDIA_PLAYER_CONTROL_EOF,
  SIGNAL_MEDIA_PLAYER_CONTROL_Error,
  SIGNAL_MEDIA_PLAYER_CONTROL_Seeked,
  SIGNAL_MEDIA_PLAYER_CONTROL_Stopped,
  SIGNAL_MEDIA_PLAYER_CONTROL_RequestWindow,
  SIGNAL_MEDIA_PLAYER_CONTROL_Buffering,
  SIGNAL_MEDIA_PLAYER_CONTROL_Buffered,
  SIGNAL_MEDIA_PLAYER_CONTROL_PlayerStateChanged,
  SIGNAL_MEDIA_PLAYER_CONTROL_TargetReady,
  SIGNAL_MEDIA_PLAYER_CONTROL_Suspended,
  SIGNAL_MEDIA_PLAYER_CONTROL_Restored,
  SIGNAL_MEDIA_PLAYER_CONTROL_VideoTagChanged,
  SIGNAL_MEDIA_PLAYER_CONTROL_AudioTagChanged,
  SIGNAL_MEDIA_PLAYER_CONTROL_TextTagChanged,
  SIGNAL_MEDIA_PLAYER_CONTROL_MetadataChanged,
  SIGNAL_MEDIA_PLAYER_CONTROL_RecordStart,
  SIGNAL_MEDIA_PLAYER_CONTROL_RecordStop,
  N_MEDIA_PLAYER_CONTROL_SIGNALS
};
static guint media_player_control_signals[N_MEDIA_PLAYER_CONTROL_SIGNALS] = {0};

static void media_player_control_base_init (gpointer klass);

GType
media_player_control_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0)) {
    static const GTypeInfo info = {
      sizeof (MediaPlayerControlClass),
      media_player_control_base_init, /* base_init */
      NULL, /* base_finalize */
      NULL, /* class_init */
      NULL, /* class_finalize */
      NULL, /* class_data */
      0,
      0, /* n_preallocs */
      NULL /* instance_init */
    };

    type = g_type_register_static (G_TYPE_INTERFACE,
           "MediaPlayerControl", &info, 0);
  }

  return type;
}

gboolean
media_player_control_set_uri (MediaPlayerControl *self,
    const gchar *in_uri)
{
  media_player_control_set_uri_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_uri);

  if (impl != NULL) {
    (impl) (self,
            in_uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_uri (MediaPlayerControlClass *klass,
    media_player_control_set_uri_impl impl)
{
  klass->set_uri = impl;
}

gboolean
media_player_control_set_target (MediaPlayerControl *self,
    gint type, GHashTable *params)
{
  media_player_control_set_target_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_target);

  if (impl != NULL) {
    (impl) (self, type, params);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_target (MediaPlayerControlClass *klass,
    media_player_control_set_target_impl impl)
{
  klass->set_target = impl;
}



gboolean
media_player_control_play (MediaPlayerControl *self)
{
  media_player_control_play_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->play);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_play (MediaPlayerControlClass *klass,
    media_player_control_play_impl impl)
{
  klass->play = impl;
}

gboolean
media_player_control_pause (MediaPlayerControl *self)
{
  media_player_control_pause_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->pause);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_pause (MediaPlayerControlClass *klass,
    media_player_control_pause_impl impl)
{
  klass->pause = impl;
}

gboolean
media_player_control_stop (MediaPlayerControl *self)
{
  media_player_control_stop_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->stop);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_stop (MediaPlayerControlClass *klass,
    media_player_control_stop_impl impl)
{
  klass->stop = impl;
}


gboolean
media_player_control_set_position (MediaPlayerControl *self,
    gint64 in_pos)
{
  media_player_control_set_position_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_position);

  if (impl != NULL) {
    (impl) (self,
            in_pos);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_position (MediaPlayerControlClass *klass,
    media_player_control_set_position_impl impl)
{
  klass->set_position = impl;
}

gboolean
media_player_control_get_position (MediaPlayerControl *self, gint64 *cur_time)
{
  gboolean ret = FALSE;
  media_player_control_get_position_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_position);

  if (impl != NULL) {
    ret = (impl) (self, cur_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return ret;
}


void media_player_control_implement_get_position (MediaPlayerControlClass *klass,
    media_player_control_get_position_impl impl)
{
  klass->get_position = impl;
}


gboolean
media_player_control_set_playback_rate (MediaPlayerControl *self,
    gdouble in_rate)
{
  media_player_control_set_playback_rate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_playback_rate);

  if (impl != NULL) {
    return (impl) (self,
                   in_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_playback_rate (MediaPlayerControlClass *klass,
    media_player_control_set_playback_rate_impl impl)
{
  klass->set_playback_rate = impl;
}

gboolean
media_player_control_get_playback_rate (MediaPlayerControl *self, gdouble *out_rate)
{
  media_player_control_get_playback_rate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_playback_rate);

  if (impl != NULL) {
    (impl) (self, out_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_playback_rate (MediaPlayerControlClass *klass,
    media_player_control_get_playback_rate_impl impl)
{
  klass->get_playback_rate = impl;
}


gboolean
media_player_control_set_volume (MediaPlayerControl *self,
    gint in_volume)
{
  media_player_control_set_volume_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_volume);

  if (impl != NULL) {
    (impl) (self,
            in_volume);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_volume (MediaPlayerControlClass *klass,
    media_player_control_set_volume_impl impl)
{
  klass->set_volume = impl;
}

gboolean
media_player_control_get_volume (MediaPlayerControl *self, gint *vol)
{
  media_player_control_get_volume_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_volume);

  if (impl != NULL) {
    (impl) (self, vol);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_volume (MediaPlayerControlClass *klass,
    media_player_control_get_volume_impl impl)
{
  klass->get_volume = impl;
}


gboolean
media_player_control_set_window_id (MediaPlayerControl *self,
    gdouble in_win_id)
{
  media_player_control_set_window_id_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_window_id);

  if (impl != NULL) {
    (impl) (self,
            in_win_id);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_window_id (MediaPlayerControlClass *klass,
    media_player_control_set_window_id_impl impl)
{
  klass->set_window_id = impl;
}





gboolean
media_player_control_set_video_size (MediaPlayerControl *self,
    guint in_x,
    guint in_y,
    guint in_w,
    guint in_h)
{
  media_player_control_set_video_size_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_video_size);

  if (impl != NULL) {
    (impl) (self,
            in_x,
            in_y,
            in_w,
            in_h);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_video_size (MediaPlayerControlClass *klass,
    media_player_control_set_video_size_impl impl)
{
  klass->set_video_size = impl;
}

gboolean
media_player_control_get_video_size (MediaPlayerControl *self, guint *w, guint *h)
{
  media_player_control_get_video_size_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_size);

  if (impl != NULL) {
    (impl) (self, w, h);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_video_size (MediaPlayerControlClass *klass,
    media_player_control_get_video_size_impl impl)
{
  klass->get_video_size = impl;
}

gboolean
media_player_control_get_buffered_time (MediaPlayerControl *self, gint64 *buffered_time)
{
  media_player_control_get_buffered_time_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffered_time);

  if (impl != NULL) {
    (impl) (self, buffered_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_buffered_time (MediaPlayerControlClass *klass,
    media_player_control_get_buffered_time_impl impl)
{
  klass->get_buffered_time = impl;
}

gboolean
media_player_control_get_buffered_bytes (MediaPlayerControl *self, gint64 *buffered_time)
{
  media_player_control_get_buffered_bytes_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffered_bytes);

  if (impl != NULL) {
    (impl) (self, buffered_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_buffered_bytes (MediaPlayerControlClass *klass,
    media_player_control_get_buffered_bytes_impl impl)
{
  klass->get_buffered_bytes = impl;
}

gboolean
media_player_control_get_media_size_time (MediaPlayerControl *self, gint64 *media_size_time)
{
  media_player_control_get_media_size_time_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_media_size_time);

  if (impl != NULL) {
    (impl) (self, media_size_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_media_size_time (MediaPlayerControlClass *klass,
    media_player_control_get_media_size_time_impl impl)
{
  klass->get_media_size_time = impl;
}

gboolean
media_player_control_get_media_size_bytes (MediaPlayerControl *self, gint64 *media_size_bytes)
{
  media_player_control_get_media_size_bytes_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_media_size_bytes);

  if (impl != NULL) {
    (impl) (self, media_size_bytes);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_media_size_bytes (MediaPlayerControlClass *klass,
    media_player_control_get_media_size_bytes_impl impl)
{
  klass->get_media_size_bytes = impl;
}

gboolean
media_player_control_has_video (MediaPlayerControl *self, gboolean *has_video)
{
  media_player_control_has_video_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->has_video);

  if (impl != NULL) {
    (impl) (self, has_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_has_video (MediaPlayerControlClass *klass,
    media_player_control_has_video_impl impl)
{
  klass->has_video = impl;
}

gboolean
media_player_control_has_audio (MediaPlayerControl *self, gboolean *has_audio)
{
  media_player_control_has_audio_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->has_audio);

  if (impl != NULL) {
    (impl) (self, has_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_has_audio (MediaPlayerControlClass *klass,
    media_player_control_has_audio_impl impl)
{
  klass->has_audio = impl;
}

gboolean
media_player_control_is_streaming (MediaPlayerControl *self, gboolean *is_streaming)
{
  media_player_control_is_streaming_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_streaming);

  if (impl != NULL) {
    (impl) (self, is_streaming);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_is_streaming (MediaPlayerControlClass *klass,
    media_player_control_is_streaming_impl impl)
{
  klass->is_streaming = impl;
}

gboolean
media_player_control_is_seekable (MediaPlayerControl *self, gboolean *seekable)
{
  media_player_control_is_seekable_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_seekable);

  if (impl != NULL) {
    (impl) (self, seekable);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_is_seekable (MediaPlayerControlClass *klass,
    media_player_control_is_seekable_impl impl)
{
  klass->is_seekable = impl;
}

gboolean
media_player_control_support_fullscreen (MediaPlayerControl *self, gboolean *support_fullscreen)
{
  media_player_control_support_fullscreen_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->support_fullscreen);

  if (impl != NULL) {
    (impl) (self, support_fullscreen);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_support_fullscreen (MediaPlayerControlClass *klass,
    media_player_control_support_fullscreen_impl impl)
{
  klass->support_fullscreen = impl;
}

gboolean
media_player_control_get_player_state (MediaPlayerControl *self, gint *state)
{
  media_player_control_get_player_state_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_player_state);

  if (impl != NULL) {
    (impl) (self, state);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_player_state (MediaPlayerControlClass *klass,
    media_player_control_get_player_state_impl impl)
{
  klass->get_player_state = impl;
}


gboolean
media_player_control_get_current_video (MediaPlayerControl *self, gint *cur_video)
{
  media_player_control_get_current_video_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_video);

  if (impl != NULL) {
    (impl) (self, cur_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_current_video (MediaPlayerControlClass *klass,
    media_player_control_get_current_video_impl impl)
{
  klass->get_current_video = impl;
}


gboolean
media_player_control_get_current_audio (MediaPlayerControl *self, gint *cur_audio)
{
  media_player_control_get_current_audio_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_audio);

  if (impl != NULL) {
    (impl) (self, cur_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_current_audio (MediaPlayerControlClass *klass,
    media_player_control_get_current_audio_impl impl)
{
  klass->get_current_audio = impl;
}


gboolean
media_player_control_set_current_video (MediaPlayerControl *self, gint cur_video)
{
  media_player_control_set_current_video_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_video);

  if (impl != NULL) {
    (impl) (self, cur_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_current_video (MediaPlayerControlClass *klass,
    media_player_control_set_current_video_impl impl)
{
  klass->set_current_video = impl;
}


gboolean
media_player_control_set_current_audio (MediaPlayerControl *self, gint cur_audio)
{
  media_player_control_set_current_audio_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_audio);

  if (impl != NULL) {
    (impl) (self, cur_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_set_current_audio (MediaPlayerControlClass *klass,
    media_player_control_set_current_audio_impl impl)
{
  klass->set_current_audio = impl;
}


gboolean
media_player_control_get_video_num (MediaPlayerControl *self, gint *video_num)
{
  media_player_control_get_video_num_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_num);

  if (impl != NULL) {
    (impl) (self, video_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_video_num (MediaPlayerControlClass *klass,
    media_player_control_get_video_num_impl impl)
{
  klass->get_video_num = impl;
}


gboolean
media_player_control_get_audio_num (MediaPlayerControl *self, gint *audio_num)
{
  media_player_control_get_audio_num_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_num);

  if (impl != NULL) {
    (impl) (self, audio_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void media_player_control_implement_get_audio_num (MediaPlayerControlClass *klass,
    media_player_control_get_audio_num_impl impl)
{
  klass->get_audio_num = impl;
}

gboolean
media_player_control_set_proxy (MediaPlayerControl *self,
    GHashTable *params)
{
  media_player_control_set_proxy_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_proxy);

  if (impl != NULL) {
    (impl) (self, params);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_proxy (MediaPlayerControlClass *klass,
    media_player_control_set_proxy_impl impl)
{
  klass->set_proxy = impl;
}

gboolean
media_player_control_set_subtitle_uri (MediaPlayerControl *self, gchar *sub_uri)
{
  media_player_control_set_subtitle_uri_impl impl = (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_subtitle_uri);

  if (impl != NULL) {
    (impl) (self, sub_uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_subtitle_uri (MediaPlayerControlClass *klass,
    media_player_control_set_subtitle_uri_impl impl)
{
  klass->set_subtitle_uri = impl;
}

gboolean
media_player_control_get_subtitle_num (MediaPlayerControl *self, gint *sub_num)
{
  media_player_control_get_subtitle_num_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_subtitle_num);

  if (impl != NULL) {
    (impl) (self, sub_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_subtitle_num (MediaPlayerControlClass *klass,
    media_player_control_get_subtitle_num_impl impl)
{
  klass->get_subtitle_num = impl;
}

gboolean
media_player_control_get_current_subtitle (MediaPlayerControl *self, gint *cur_sub)
{
  media_player_control_get_current_subtitle_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_subtitle);

  if (impl != NULL) {
    (impl) (self, cur_sub);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_current_subtitle (MediaPlayerControlClass *klass,
    media_player_control_get_current_subtitle_impl impl)
{
  klass->get_current_subtitle = impl;
}

gboolean
media_player_control_set_current_subtitle (MediaPlayerControl *self, gint cur_sub)
{
  media_player_control_set_current_subtitle_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_subtitle);

  if (impl != NULL) {
    (impl) (self, cur_sub);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_current_subtitle (MediaPlayerControlClass *klass,
    media_player_control_set_current_subtitle_impl impl)
{
  klass->set_current_subtitle = impl;
}

gboolean
media_player_control_set_buffer_depth (MediaPlayerControl *self, gint format, gint64 buf_val)
{
  media_player_control_set_buffer_depth_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_buffer_depth);

  if (impl != NULL) {
    (impl) (self, format, buf_val);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_buffer_depth (MediaPlayerControlClass *klass,
    media_player_control_set_buffer_depth_impl impl)
{
  klass->set_buffer_depth = impl;
}

gboolean
media_player_control_get_buffer_depth (MediaPlayerControl *self, gint format, gint64 *buf_val)
{
  media_player_control_get_buffer_depth_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffer_depth);

  if (impl != NULL) {
    (impl) (self, format, buf_val);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_buffer_depth (MediaPlayerControlClass *klass,
    media_player_control_get_buffer_depth_impl impl)
{
  klass->get_buffer_depth = impl;
}

gboolean
media_player_control_set_mute (MediaPlayerControl *self, gint mute)
{
  media_player_control_set_mute_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_mute);

  if (impl != NULL) {
    (impl) (self, mute);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_mute (MediaPlayerControlClass *klass,
    media_player_control_set_mute_impl impl)
{
  klass->set_mute = impl;
}

gboolean
media_player_control_is_mute (MediaPlayerControl *self, gint *mute)
{
  media_player_control_is_mute_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_mute);

  if (impl != NULL) {
    (impl) (self, mute);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_is_mute (MediaPlayerControlClass *klass,
    media_player_control_is_mute_impl impl)
{
  klass->is_mute = impl;
}

gboolean media_player_control_set_scale_mode (MediaPlayerControl *self, gint scale_mode)
{
  media_player_control_set_scale_mode_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_scale_mode);

  if (impl != NULL) {
    (impl) (self, scale_mode);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_set_scale_mode (MediaPlayerControlClass *klass,
    media_player_control_set_scale_mode_impl impl)
{
  klass->set_scale_mode = impl;
}

gboolean media_player_control_get_scale_mode (MediaPlayerControl *self, gint *scale_mode)
{
  media_player_control_get_scale_mode_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_scale_mode);

  if (impl != NULL) {
    (impl) (self, scale_mode);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_scale_mode (MediaPlayerControlClass *klass,
    media_player_control_get_scale_mode_impl impl)
{
  klass->get_scale_mode = impl;
}

gboolean media_player_control_suspend (MediaPlayerControl *self)
{
  media_player_control_suspend_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->suspend);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_suspend (MediaPlayerControlClass *klass,
    media_player_control_suspend_impl impl)
{
  klass->suspend = impl;
}

gboolean media_player_control_restore (MediaPlayerControl *self)
{
  media_player_control_restore_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->restore);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_restore (MediaPlayerControlClass *klass,
    media_player_control_restore_impl impl)
{
  klass->restore = impl;
}

gboolean media_player_control_get_video_codec (MediaPlayerControl *self,
    gint channel,
    gchar **video_codec)
{
  media_player_control_get_video_codec_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_codec);

  if (impl != NULL) {
    (impl) (self, channel, video_codec);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_video_codec (MediaPlayerControlClass *klass,
    media_player_control_get_video_codec_impl impl)
{
  klass->get_video_codec = impl;
}

gboolean media_player_control_get_audio_codec (MediaPlayerControl *self,
    gint channel,
    gchar **audio_codec)
{
  media_player_control_get_audio_codec_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_codec);

  if (impl != NULL) {
    (impl) (self, channel, audio_codec);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_audio_codec (MediaPlayerControlClass *klass,
    media_player_control_get_audio_codec_impl impl)
{
  klass->get_audio_codec = impl;
}

gboolean media_player_control_get_video_bitrate (MediaPlayerControl *self, gint channel, gint *bit_rate)
{
  media_player_control_get_video_bitrate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_bitrate);

  if (impl != NULL) {
    (impl) (self, channel, bit_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_video_bitrate (MediaPlayerControlClass *klass,
    media_player_control_get_video_bitrate_impl impl)
{
  klass->get_video_bitrate = impl;
}

gboolean media_player_control_get_audio_bitrate (MediaPlayerControl *self, gint channel, gint *bit_rate)
{
  media_player_control_get_audio_bitrate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_bitrate);

  if (impl != NULL) {
    (impl) (self, channel, bit_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_audio_bitrate (MediaPlayerControlClass *klass,
    media_player_control_get_audio_bitrate_impl impl)
{
  klass->get_audio_bitrate = impl;
}

gboolean media_player_control_get_encapsulation(MediaPlayerControl *self, gchar ** encapsulation)
{
  media_player_control_get_encapsulation_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_encapsulation);

  if (impl != NULL) {
    (impl) (self, encapsulation);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_encapsulation (MediaPlayerControlClass *klass,
    media_player_control_get_encapsulation_impl impl)
{
  klass->get_encapsulation = impl;
}

gboolean media_player_control_get_audio_samplerate(MediaPlayerControl *self, gint channel,
    gint * sample_rate)
{
  media_player_control_get_audio_samplerate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_samplerate);

  if (impl != NULL) {
    (impl) (self, channel, sample_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_audio_samplerate (MediaPlayerControlClass *klass,
    media_player_control_get_audio_samplerate_impl impl)
{
  klass->get_audio_samplerate = impl;
}

gboolean media_player_control_get_video_framerate(MediaPlayerControl *self, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom)
{
  media_player_control_get_video_framerate_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_framerate);

  if (impl != NULL) {
    (impl) (self, channel, frame_rate_num, frame_rate_denom);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_video_framerate (MediaPlayerControlClass *klass,
    media_player_control_get_video_framerate_impl impl)
{
  klass->get_video_framerate = impl;
}

gboolean media_player_control_get_video_resolution(MediaPlayerControl *self, gint channel,
    gint * width, gint * height)
{
  media_player_control_get_video_resolution_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_resolution);

  if (impl != NULL) {
    (impl) (self, channel, width, height);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_video_resolution(MediaPlayerControlClass *klass,
    media_player_control_get_video_resolution_impl impl)
{
  klass->get_video_resolution = impl;
}


gboolean media_player_control_get_video_aspect_ratio(MediaPlayerControl *self,
    gint channel, gint * ratio_num, gint * ratio_denom)
{
  media_player_control_get_video_aspect_ratio_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_aspect_ratio);

  if (impl != NULL) {
    (impl) (self, channel, ratio_num, ratio_denom);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_video_aspect_ratio(MediaPlayerControlClass *klass,
    media_player_control_get_video_aspect_ratio_impl impl)
{
  klass->get_video_aspect_ratio = impl;
}

gboolean media_player_control_get_protocol_name(MediaPlayerControl *self, gchar ** prot_name)
{
  media_player_control_get_protocol_name_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_protocol_name);

  if (impl != NULL) {
    (impl) (self, prot_name);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_protocol_name(MediaPlayerControlClass *klass,
    media_player_control_get_protocol_name_impl impl)
{
  klass->get_protocol_name = impl;
}

gboolean media_player_control_get_current_uri(MediaPlayerControl *self, gchar ** uri)
{
  media_player_control_get_current_uri_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_uri);

  if (impl != NULL) {
    (impl) (self, uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_current_uri(MediaPlayerControlClass *klass,
    media_player_control_get_current_uri_impl impl)
{
  klass->get_current_uri = impl;
}

gboolean media_player_control_get_title (MediaPlayerControl *self, gchar ** title)
{
  media_player_control_get_title_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_title);

  if (impl != NULL) {
    (impl) (self, title);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_title(MediaPlayerControlClass *klass,
    media_player_control_get_title_impl impl)
{
  klass->get_title = impl;
}

gboolean media_player_control_get_artist(MediaPlayerControl *self, gchar ** artist)
{
  media_player_control_get_artist_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_artist);

  if (impl != NULL) {
    (impl) (self, artist);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_artist(MediaPlayerControlClass *klass,
    media_player_control_get_artist_impl impl)
{
  klass->get_artist = impl;
}

gboolean media_player_control_record (MediaPlayerControl *self, gboolean to_record, gchar *location)
{
  gboolean ret = FALSE;
  media_player_control_record_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->record);

  if (impl != NULL) {
    ret = (impl) (self, to_record, location);
  } else {
    g_warning ("Method not implemented\n");
  }
  return ret;
}

void media_player_control_implement_record  (MediaPlayerControlClass *klass,
    media_player_control_record_impl impl)
{
  klass->record = impl;
}

gboolean media_player_control_get_pat (MediaPlayerControl *self, GPtrArray **pat)
{
  media_player_control_get_pat_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_pat);

  if (impl != NULL) {
    (impl) (self, pat);
  } else {
    g_warning ("%s: Method not implemented\n", __FUNCTION__);
    *pat = g_ptr_array_new ();
  }
  return TRUE;
}

void media_player_control_implement_get_pat(MediaPlayerControlClass *klass,
    media_player_control_get_pat_impl impl)
{
  klass->get_pat = impl;
}

gboolean media_player_control_get_pmt (MediaPlayerControl *self, guint *program_num, guint *pcr_pid, 
                                             GPtrArray **stream_info)
{
  media_player_control_get_pmt_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_pmt);

  if (impl != NULL) {
    (impl) (self, program_num, pcr_pid, stream_info);
  } else {
    g_warning ("Method not implemented\n");
    *stream_info = g_ptr_array_new ();
  }
  return TRUE;
}

void media_player_control_implement_get_pmt(MediaPlayerControlClass *klass,
    media_player_control_get_pmt_impl impl)
{
  klass->get_pmt = impl;
}

gboolean 
media_player_control_get_associated_data_channel (MediaPlayerControl *self, gchar **ip, gint *port)
{
  media_player_control_get_associated_data_channel_impl impl =
    (MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_associated_data_channel);

  if (impl != NULL) {
    (impl) (self, ip, port);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void media_player_control_implement_get_associated_data_channel (MediaPlayerControlClass *klass,
    media_player_control_get_associated_data_channel_impl impl)
{
  klass->get_associated_data_channel = impl;
}

void
media_player_control_emit_initialized (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Initialized],
                 0);
}
void
media_player_control_emit_eof (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_EOF],
                 0);
}

void
media_player_control_emit_error (gpointer instance,
    guint error_num, gchar *error_des)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Error],
                 0,
                 error_num, error_des);
}

void
media_player_control_emit_buffered (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Buffered],
                 0);
}
void
media_player_control_emit_buffering (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Buffering],
                 0);
}

void
media_player_control_emit_player_state_changed (gpointer instance, gint old_state, gint new_state)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_PlayerStateChanged],
                 0, old_state, new_state);
}

void
media_player_control_emit_target_ready (gpointer instance, GHashTable *infos)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_TargetReady],
                 0, infos);
}
void
media_player_control_emit_seeked (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Seeked],
                 0);
}
void
media_player_control_emit_stopped (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Stopped],
                 0);
}

void
media_player_control_emit_request_window (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RequestWindow],
                 0);
}

void
media_player_control_emit_suspended (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Suspended],
                 0);
}

void
media_player_control_emit_restored (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Restored],
                 0);
}

void
media_player_control_emit_video_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_VideoTagChanged],
                 0, channel);
}

void
media_player_control_emit_audio_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_AudioTagChanged],
                 0, channel);
}

void
media_player_control_emit_text_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_TextTagChanged],
                 0, channel);
}

void
media_player_control_emit_metadata_changed (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_MetadataChanged],
                 0);
}

void
media_player_control_emit_record_start (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RecordStart],
                 0);
}

void
media_player_control_emit_record_stop (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RecordStop],
                 0);
}

static inline void
media_player_control_base_init_once (gpointer klass G_GNUC_UNUSED)
{

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Initialized] =
    g_signal_new ("initialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_EOF] =
    g_signal_new ("eof",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Error] =
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

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Seeked] =
    g_signal_new ("seeked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Stopped] =
    g_signal_new ("stopped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RequestWindow] =
    g_signal_new ("request-window",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Buffering] =
    g_signal_new ("buffering",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Buffered] =
    g_signal_new ("buffered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_PlayerStateChanged] =
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

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_TargetReady] =
    g_signal_new ("target-ready",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE));

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Suspended] =
    g_signal_new ("suspended",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_Restored] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_VideoTagChanged] =
    g_signal_new ("video-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_AudioTagChanged] =
    g_signal_new ("audio-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_TextTagChanged] =
    g_signal_new ("text-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_MetadataChanged] =
    g_signal_new ("metadata-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  
  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RecordStart] =
    g_signal_new ("record-start",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  media_player_control_signals[SIGNAL_MEDIA_PLAYER_CONTROL_RecordStop] =
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
media_player_control_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
    media_player_control_base_init_once (klass);
  }
}
