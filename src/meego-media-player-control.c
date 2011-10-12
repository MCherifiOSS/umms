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
#include "meego-media-player-control.h"

struct _MeegoMediaPlayerControlClass {
  GTypeInterface parent_class;
  meego_media_player_control_set_uri_impl set_uri;
  meego_media_player_control_set_target_impl set_target;
  meego_media_player_control_play_impl play;
  meego_media_player_control_pause_impl pause;
  meego_media_player_control_stop_impl stop;
  meego_media_player_control_set_position_impl set_position;
  meego_media_player_control_get_position_impl get_position;
  meego_media_player_control_set_playback_rate_impl set_playback_rate;
  meego_media_player_control_get_playback_rate_impl get_playback_rate;
  meego_media_player_control_set_volume_impl set_volume;
  meego_media_player_control_get_volume_impl get_volume;
  meego_media_player_control_set_window_id_impl set_window_id;
  meego_media_player_control_set_video_size_impl set_video_size;
  meego_media_player_control_get_video_size_impl get_video_size;
  meego_media_player_control_get_buffered_time_impl get_buffered_time;
  meego_media_player_control_get_buffered_bytes_impl get_buffered_bytes;
  meego_media_player_control_get_media_size_time_impl get_media_size_time;
  meego_media_player_control_get_media_size_bytes_impl get_media_size_bytes;
  meego_media_player_control_has_video_impl has_video;
  meego_media_player_control_has_audio_impl has_audio;
  meego_media_player_control_is_streaming_impl is_streaming;
  meego_media_player_control_is_seekable_impl is_seekable;
  meego_media_player_control_support_fullscreen_impl support_fullscreen;
  meego_media_player_control_get_player_state_impl get_player_state;
  meego_media_player_control_get_current_video_impl get_current_video;
  meego_media_player_control_get_current_audio_impl get_current_audio;
  meego_media_player_control_set_current_video_impl set_current_video;
  meego_media_player_control_set_current_audio_impl set_current_audio;
  meego_media_player_control_get_video_num_impl get_video_num;
  meego_media_player_control_get_audio_num_impl get_audio_num;
  meego_media_player_control_set_proxy_impl set_proxy;
  meego_media_player_control_set_subtitle_uri_impl set_subtitle_uri;
  meego_media_player_control_get_audio_num_impl get_subtitle_num;
  meego_media_player_control_get_current_subtitle_impl get_current_subtitle;
  meego_media_player_control_set_current_subtitle_impl set_current_subtitle;
  meego_media_player_control_set_buffer_depth_impl set_buffer_depth;
  meego_media_player_control_get_buffer_depth_impl get_buffer_depth;
  meego_media_player_control_set_mute_impl set_mute;
  meego_media_player_control_is_mute_impl is_mute;
  meego_media_player_control_set_scale_mode_impl set_scale_mode;
  meego_media_player_control_get_scale_mode_impl get_scale_mode;
  meego_media_player_control_suspend_impl suspend;
  meego_media_player_control_restore_impl restore;
  meego_media_player_control_get_video_codec_impl get_video_codec;
  meego_media_player_control_get_audio_codec_impl get_audio_codec;
  meego_media_player_control_get_video_bitrate_impl get_video_bitrate;
  meego_media_player_control_get_audio_bitrate_impl get_audio_bitrate;
  meego_media_player_control_get_encapsulation_impl get_encapsulation;
  meego_media_player_control_get_audio_samplerate_impl get_audio_samplerate;
  meego_media_player_control_get_video_framerate_impl get_video_framerate;
  meego_media_player_control_get_video_resolution_impl get_video_resolution;
  meego_media_player_control_get_video_aspect_ratio_impl get_video_aspect_ratio;
  meego_media_player_control_get_protocol_name_impl get_protocol_name;
  meego_media_player_control_get_current_uri_impl get_current_uri;
  meego_media_player_control_get_title_impl get_title;
  meego_media_player_control_get_artist_impl get_artist;
  meego_media_player_control_record_impl record;
  meego_media_player_control_get_pat_impl get_pat;
  meego_media_player_control_get_pmt_impl get_pmt;
  meego_media_player_control_get_associated_data_channel_impl get_associated_data_channel;
};

enum {
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Initialized,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_EOF,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Error,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Seeked,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Stopped,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_RequestWindow,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffering,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffered,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_PlayerStateChanged,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TargetReady,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Suspended,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Restored,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_VideoTagChanged,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_AudioTagChanged,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TextTagChanged,
  SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_MetadataChanged,
  N_MEEGO_MEDIA_PLAYER_CONTROL_SIGNALS
};
static guint meego_media_player_control_signals[N_MEEGO_MEDIA_PLAYER_CONTROL_SIGNALS] = {0};

static void meego_media_player_control_base_init (gpointer klass);

GType
meego_media_player_control_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0)) {
    static const GTypeInfo info = {
      sizeof (MeegoMediaPlayerControlClass),
      meego_media_player_control_base_init, /* base_init */
      NULL, /* base_finalize */
      NULL, /* class_init */
      NULL, /* class_finalize */
      NULL, /* class_data */
      0,
      0, /* n_preallocs */
      NULL /* instance_init */
    };

    type = g_type_register_static (G_TYPE_INTERFACE,
           "MeegoMediaPlayerControl", &info, 0);
  }

  return type;
}

gboolean
meego_media_player_control_set_uri (MeegoMediaPlayerControl *self,
    const gchar *in_uri)
{
  meego_media_player_control_set_uri_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_uri);

  if (impl != NULL) {
    (impl) (self,
            in_uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_uri (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_uri_impl impl)
{
  klass->set_uri = impl;
}

gboolean
meego_media_player_control_set_target (MeegoMediaPlayerControl *self,
    gint type, GHashTable *params)
{
  meego_media_player_control_set_target_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_target);

  if (impl != NULL) {
    (impl) (self, type, params);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_target (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_target_impl impl)
{
  klass->set_target = impl;
}



gboolean
meego_media_player_control_play (MeegoMediaPlayerControl *self)
{
  meego_media_player_control_play_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->play);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_play (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_play_impl impl)
{
  klass->play = impl;
}

gboolean
meego_media_player_control_pause (MeegoMediaPlayerControl *self)
{
  meego_media_player_control_pause_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->pause);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_pause (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_pause_impl impl)
{
  klass->pause = impl;
}

gboolean
meego_media_player_control_stop (MeegoMediaPlayerControl *self)
{
  meego_media_player_control_stop_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->stop);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_stop (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_stop_impl impl)
{
  klass->stop = impl;
}


gboolean
meego_media_player_control_set_position (MeegoMediaPlayerControl *self,
    gint64 in_pos)
{
  meego_media_player_control_set_position_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_position);

  if (impl != NULL) {
    (impl) (self,
            in_pos);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_position (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_position_impl impl)
{
  klass->set_position = impl;
}

gboolean
meego_media_player_control_get_position (MeegoMediaPlayerControl *self, gint64 *cur_time)
{
  gboolean ret = FALSE;
  meego_media_player_control_get_position_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_position);

  if (impl != NULL) {
    ret = (impl) (self, cur_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return ret;
}


void meego_media_player_control_implement_get_position (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_position_impl impl)
{
  klass->get_position = impl;
}


gboolean
meego_media_player_control_set_playback_rate (MeegoMediaPlayerControl *self,
    gdouble in_rate)
{
  meego_media_player_control_set_playback_rate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_playback_rate);

  if (impl != NULL) {
    return (impl) (self,
                   in_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_playback_rate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_playback_rate_impl impl)
{
  klass->set_playback_rate = impl;
}

gboolean
meego_media_player_control_get_playback_rate (MeegoMediaPlayerControl *self, gdouble *out_rate)
{
  meego_media_player_control_get_playback_rate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_playback_rate);

  if (impl != NULL) {
    (impl) (self, out_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_playback_rate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_playback_rate_impl impl)
{
  klass->get_playback_rate = impl;
}


gboolean
meego_media_player_control_set_volume (MeegoMediaPlayerControl *self,
    gint in_volume)
{
  meego_media_player_control_set_volume_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_volume);

  if (impl != NULL) {
    (impl) (self,
            in_volume);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_volume (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_volume_impl impl)
{
  klass->set_volume = impl;
}

gboolean
meego_media_player_control_get_volume (MeegoMediaPlayerControl *self, gint *vol)
{
  meego_media_player_control_get_volume_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_volume);

  if (impl != NULL) {
    (impl) (self, vol);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_volume (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_volume_impl impl)
{
  klass->get_volume = impl;
}


gboolean
meego_media_player_control_set_window_id (MeegoMediaPlayerControl *self,
    gdouble in_win_id)
{
  meego_media_player_control_set_window_id_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_window_id);

  if (impl != NULL) {
    (impl) (self,
            in_win_id);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_window_id (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_window_id_impl impl)
{
  klass->set_window_id = impl;
}





gboolean
meego_media_player_control_set_video_size (MeegoMediaPlayerControl *self,
    guint in_x,
    guint in_y,
    guint in_w,
    guint in_h)
{
  meego_media_player_control_set_video_size_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_video_size);

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


void meego_media_player_control_implement_set_video_size (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_video_size_impl impl)
{
  klass->set_video_size = impl;
}

gboolean
meego_media_player_control_get_video_size (MeegoMediaPlayerControl *self, guint *w, guint *h)
{
  meego_media_player_control_get_video_size_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_size);

  if (impl != NULL) {
    (impl) (self, w, h);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_video_size (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_size_impl impl)
{
  klass->get_video_size = impl;
}

gboolean
meego_media_player_control_get_buffered_time (MeegoMediaPlayerControl *self, gint64 *buffered_time)
{
  meego_media_player_control_get_buffered_time_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffered_time);

  if (impl != NULL) {
    (impl) (self, buffered_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_buffered_time (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_buffered_time_impl impl)
{
  klass->get_buffered_time = impl;
}

gboolean
meego_media_player_control_get_buffered_bytes (MeegoMediaPlayerControl *self, gint64 *buffered_time)
{
  meego_media_player_control_get_buffered_bytes_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffered_bytes);

  if (impl != NULL) {
    (impl) (self, buffered_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_buffered_bytes (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_buffered_bytes_impl impl)
{
  klass->get_buffered_bytes = impl;
}

gboolean
meego_media_player_control_get_media_size_time (MeegoMediaPlayerControl *self, gint64 *media_size_time)
{
  meego_media_player_control_get_media_size_time_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_media_size_time);

  if (impl != NULL) {
    (impl) (self, media_size_time);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_media_size_time (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_media_size_time_impl impl)
{
  klass->get_media_size_time = impl;
}

gboolean
meego_media_player_control_get_media_size_bytes (MeegoMediaPlayerControl *self, gint64 *media_size_bytes)
{
  meego_media_player_control_get_media_size_bytes_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_media_size_bytes);

  if (impl != NULL) {
    (impl) (self, media_size_bytes);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_media_size_bytes (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_media_size_bytes_impl impl)
{
  klass->get_media_size_bytes = impl;
}

gboolean
meego_media_player_control_has_video (MeegoMediaPlayerControl *self, gboolean *has_video)
{
  meego_media_player_control_has_video_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->has_video);

  if (impl != NULL) {
    (impl) (self, has_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_has_video (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_has_video_impl impl)
{
  klass->has_video = impl;
}

gboolean
meego_media_player_control_has_audio (MeegoMediaPlayerControl *self, gboolean *has_audio)
{
  meego_media_player_control_has_audio_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->has_audio);

  if (impl != NULL) {
    (impl) (self, has_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_has_audio (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_has_audio_impl impl)
{
  klass->has_audio = impl;
}

gboolean
meego_media_player_control_is_streaming (MeegoMediaPlayerControl *self, gboolean *is_streaming)
{
  meego_media_player_control_is_streaming_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_streaming);

  if (impl != NULL) {
    (impl) (self, is_streaming);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_is_streaming (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_is_streaming_impl impl)
{
  klass->is_streaming = impl;
}

gboolean
meego_media_player_control_is_seekable (MeegoMediaPlayerControl *self, gboolean *seekable)
{
  meego_media_player_control_is_seekable_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_seekable);

  if (impl != NULL) {
    (impl) (self, seekable);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_is_seekable (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_is_seekable_impl impl)
{
  klass->is_seekable = impl;
}

gboolean
meego_media_player_control_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
  meego_media_player_control_support_fullscreen_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->support_fullscreen);

  if (impl != NULL) {
    (impl) (self, support_fullscreen);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_support_fullscreen (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_support_fullscreen_impl impl)
{
  klass->support_fullscreen = impl;
}

gboolean
meego_media_player_control_get_player_state (MeegoMediaPlayerControl *self, gint *state)
{
  meego_media_player_control_get_player_state_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_player_state);

  if (impl != NULL) {
    (impl) (self, state);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_player_state (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_player_state_impl impl)
{
  klass->get_player_state = impl;
}


gboolean
meego_media_player_control_get_current_video (MeegoMediaPlayerControl *self, gint *cur_video)
{
  meego_media_player_control_get_current_video_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_video);

  if (impl != NULL) {
    (impl) (self, cur_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_current_video (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_current_video_impl impl)
{
  klass->get_current_video = impl;
}


gboolean
meego_media_player_control_get_current_audio (MeegoMediaPlayerControl *self, gint *cur_audio)
{
  meego_media_player_control_get_current_audio_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_audio);

  if (impl != NULL) {
    (impl) (self, cur_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_current_audio (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_current_audio_impl impl)
{
  klass->get_current_audio = impl;
}


gboolean
meego_media_player_control_set_current_video (MeegoMediaPlayerControl *self, gint cur_video)
{
  meego_media_player_control_set_current_video_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_video);

  if (impl != NULL) {
    (impl) (self, cur_video);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_current_video (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_current_video_impl impl)
{
  klass->set_current_video = impl;
}


gboolean
meego_media_player_control_set_current_audio (MeegoMediaPlayerControl *self, gint cur_audio)
{
  meego_media_player_control_set_current_audio_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_audio);

  if (impl != NULL) {
    (impl) (self, cur_audio);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_set_current_audio (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_current_audio_impl impl)
{
  klass->set_current_audio = impl;
}


gboolean
meego_media_player_control_get_video_num (MeegoMediaPlayerControl *self, gint *video_num)
{
  meego_media_player_control_get_video_num_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_num);

  if (impl != NULL) {
    (impl) (self, video_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_video_num (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_num_impl impl)
{
  klass->get_video_num = impl;
}


gboolean
meego_media_player_control_get_audio_num (MeegoMediaPlayerControl *self, gint *audio_num)
{
  meego_media_player_control_get_audio_num_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_num);

  if (impl != NULL) {
    (impl) (self, audio_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}


void meego_media_player_control_implement_get_audio_num (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_audio_num_impl impl)
{
  klass->get_audio_num = impl;
}

gboolean
meego_media_player_control_set_proxy (MeegoMediaPlayerControl *self,
    GHashTable *params)
{
  meego_media_player_control_set_proxy_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_proxy);

  if (impl != NULL) {
    (impl) (self, params);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_proxy (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_proxy_impl impl)
{
  klass->set_proxy = impl;
}

gboolean
meego_media_player_control_set_subtitle_uri (MeegoMediaPlayerControl *self, gchar *sub_uri)
{
  meego_media_player_control_set_subtitle_uri_impl impl = (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_subtitle_uri);

  if (impl != NULL) {
    (impl) (self, sub_uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_subtitle_uri (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_subtitle_uri_impl impl)
{
  klass->set_subtitle_uri = impl;
}

gboolean
meego_media_player_control_get_subtitle_num (MeegoMediaPlayerControl *self, gint *sub_num)
{
  meego_media_player_control_get_subtitle_num_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_subtitle_num);

  if (impl != NULL) {
    (impl) (self, sub_num);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_subtitle_num (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_subtitle_num_impl impl)
{
  klass->get_subtitle_num = impl;
}

gboolean
meego_media_player_control_get_current_subtitle (MeegoMediaPlayerControl *self, gint *cur_sub)
{
  meego_media_player_control_get_current_subtitle_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_subtitle);

  if (impl != NULL) {
    (impl) (self, cur_sub);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_current_subtitle (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_current_subtitle_impl impl)
{
  klass->get_current_subtitle = impl;
}

gboolean
meego_media_player_control_set_current_subtitle (MeegoMediaPlayerControl *self, gint cur_sub)
{
  meego_media_player_control_set_current_subtitle_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_current_subtitle);

  if (impl != NULL) {
    (impl) (self, cur_sub);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_current_subtitle (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_current_subtitle_impl impl)
{
  klass->set_current_subtitle = impl;
}

gboolean
meego_media_player_control_set_buffer_depth (MeegoMediaPlayerControl *self, gint format, gint64 buf_val)
{
  meego_media_player_control_set_buffer_depth_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_buffer_depth);

  if (impl != NULL) {
    (impl) (self, format, buf_val);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_buffer_depth (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_buffer_depth_impl impl)
{
  klass->set_buffer_depth = impl;
}

gboolean
meego_media_player_control_get_buffer_depth (MeegoMediaPlayerControl *self, gint format, gint64 *buf_val)
{
  meego_media_player_control_get_buffer_depth_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_buffer_depth);

  if (impl != NULL) {
    (impl) (self, format, buf_val);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_buffer_depth (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_buffer_depth_impl impl)
{
  klass->get_buffer_depth = impl;
}

gboolean
meego_media_player_control_set_mute (MeegoMediaPlayerControl *self, gint mute)
{
  meego_media_player_control_set_mute_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_mute);

  if (impl != NULL) {
    (impl) (self, mute);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_mute (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_mute_impl impl)
{
  klass->set_mute = impl;
}

gboolean
meego_media_player_control_is_mute (MeegoMediaPlayerControl *self, gint *mute)
{
  meego_media_player_control_is_mute_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->is_mute);

  if (impl != NULL) {
    (impl) (self, mute);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_is_mute (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_is_mute_impl impl)
{
  klass->is_mute = impl;
}

gboolean meego_media_player_control_set_scale_mode (MeegoMediaPlayerControl *self, gint scale_mode)
{
  meego_media_player_control_set_scale_mode_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->set_scale_mode);

  if (impl != NULL) {
    (impl) (self, scale_mode);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_set_scale_mode (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_set_scale_mode_impl impl)
{
  klass->set_scale_mode = impl;
}

gboolean meego_media_player_control_get_scale_mode (MeegoMediaPlayerControl *self, gint *scale_mode)
{
  meego_media_player_control_get_scale_mode_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_scale_mode);

  if (impl != NULL) {
    (impl) (self, scale_mode);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_scale_mode (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_scale_mode_impl impl)
{
  klass->get_scale_mode = impl;
}

gboolean meego_media_player_control_suspend (MeegoMediaPlayerControl *self)
{
  meego_media_player_control_suspend_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->suspend);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_suspend (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_suspend_impl impl)
{
  klass->suspend = impl;
}

gboolean meego_media_player_control_restore (MeegoMediaPlayerControl *self)
{
  meego_media_player_control_restore_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->restore);

  if (impl != NULL) {
    (impl) (self);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_restore (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_restore_impl impl)
{
  klass->restore = impl;
}

gboolean meego_media_player_control_get_video_codec (MeegoMediaPlayerControl *self,
    gint channel,
    gchar **video_codec)
{
  meego_media_player_control_get_video_codec_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_codec);

  if (impl != NULL) {
    (impl) (self, channel, video_codec);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_video_codec (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_codec_impl impl)
{
  klass->get_video_codec = impl;
}

gboolean meego_media_player_control_get_audio_codec (MeegoMediaPlayerControl *self,
    gint channel,
    gchar **audio_codec)
{
  meego_media_player_control_get_audio_codec_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_codec);

  if (impl != NULL) {
    (impl) (self, channel, audio_codec);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_audio_codec (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_audio_codec_impl impl)
{
  klass->get_audio_codec = impl;
}

gboolean meego_media_player_control_get_video_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate)
{
  meego_media_player_control_get_video_bitrate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_bitrate);

  if (impl != NULL) {
    (impl) (self, channel, bit_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_video_bitrate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_bitrate_impl impl)
{
  klass->get_video_bitrate = impl;
}

gboolean meego_media_player_control_get_audio_bitrate (MeegoMediaPlayerControl *self, gint channel, gint *bit_rate)
{
  meego_media_player_control_get_audio_bitrate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_bitrate);

  if (impl != NULL) {
    (impl) (self, channel, bit_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_audio_bitrate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_audio_bitrate_impl impl)
{
  klass->get_audio_bitrate = impl;
}

gboolean meego_media_player_control_get_encapsulation(MeegoMediaPlayerControl *self, gchar ** encapsulation)
{
  meego_media_player_control_get_encapsulation_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_encapsulation);

  if (impl != NULL) {
    (impl) (self, encapsulation);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_encapsulation (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_encapsulation_impl impl)
{
  klass->get_encapsulation = impl;
}

gboolean meego_media_player_control_get_audio_samplerate(MeegoMediaPlayerControl *self, gint channel,
    gint * sample_rate)
{
  meego_media_player_control_get_audio_samplerate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_audio_samplerate);

  if (impl != NULL) {
    (impl) (self, channel, sample_rate);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_audio_samplerate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_audio_samplerate_impl impl)
{
  klass->get_audio_samplerate = impl;
}

gboolean meego_media_player_control_get_video_framerate(MeegoMediaPlayerControl *self, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom)
{
  meego_media_player_control_get_video_framerate_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_framerate);

  if (impl != NULL) {
    (impl) (self, channel, frame_rate_num, frame_rate_denom);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_video_framerate (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_framerate_impl impl)
{
  klass->get_video_framerate = impl;
}

gboolean meego_media_player_control_get_video_resolution(MeegoMediaPlayerControl *self, gint channel,
    gint * width, gint * height)
{
  meego_media_player_control_get_video_resolution_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_resolution);

  if (impl != NULL) {
    (impl) (self, channel, width, height);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_video_resolution(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_resolution_impl impl)
{
  klass->get_video_resolution = impl;
}


gboolean meego_media_player_control_get_video_aspect_ratio(MeegoMediaPlayerControl *self,
    gint channel, gint * ratio_num, gint * ratio_denom)
{
  meego_media_player_control_get_video_aspect_ratio_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_video_aspect_ratio);

  if (impl != NULL) {
    (impl) (self, channel, ratio_num, ratio_denom);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_video_aspect_ratio(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_video_aspect_ratio_impl impl)
{
  klass->get_video_aspect_ratio = impl;
}

gboolean meego_media_player_control_get_protocol_name(MeegoMediaPlayerControl *self, gchar ** prot_name)
{
  meego_media_player_control_get_protocol_name_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_protocol_name);

  if (impl != NULL) {
    (impl) (self, prot_name);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_protocol_name(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_protocol_name_impl impl)
{
  klass->get_protocol_name = impl;
}

gboolean meego_media_player_control_get_current_uri(MeegoMediaPlayerControl *self, gchar ** uri)
{
  meego_media_player_control_get_current_uri_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_current_uri);

  if (impl != NULL) {
    (impl) (self, uri);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_current_uri(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_current_uri_impl impl)
{
  klass->get_current_uri = impl;
}

gboolean meego_media_player_control_get_title (MeegoMediaPlayerControl *self, gchar ** title)
{
  meego_media_player_control_get_title_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_title);

  if (impl != NULL) {
    (impl) (self, title);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_title(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_title_impl impl)
{
  klass->get_title = impl;
}

gboolean meego_media_player_control_get_artist(MeegoMediaPlayerControl *self, gchar ** artist)
{
  meego_media_player_control_get_artist_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_artist);

  if (impl != NULL) {
    (impl) (self, artist);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_artist(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_artist_impl impl)
{
  klass->get_artist = impl;
}

gboolean meego_media_player_control_record (MeegoMediaPlayerControl *self, gboolean to_record, gchar *location)
{
  gboolean ret = FALSE;
  meego_media_player_control_record_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->record);

  if (impl != NULL) {
    ret = (impl) (self, to_record, location);
  } else {
    g_warning ("Method not implemented\n");
  }
  return ret;
}

void meego_media_player_control_implement_record  (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_record_impl impl)
{
  klass->record = impl;
}

gboolean meego_media_player_control_get_pat (MeegoMediaPlayerControl *self, GPtrArray **pat)
{
  meego_media_player_control_get_pat_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_pat);

  if (impl != NULL) {
    (impl) (self, pat);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_pat(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_pat_impl impl)
{
  klass->get_pat = impl;
}

gboolean meego_media_player_control_get_pmt (MeegoMediaPlayerControl *self, guint *program_num, guint *pcr_pid, 
                                             GPtrArray **stream_info)
{
  meego_media_player_control_get_pmt_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_pmt);

  if (impl != NULL) {
    (impl) (self, program_num, pcr_pid, stream_info);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_pmt(MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_pmt_impl impl)
{
  klass->get_pmt = impl;
}

gboolean 
meego_media_player_control_get_associated_data_channel (MeegoMediaPlayerControl *self, gchar **ip, gint *port)
{
  meego_media_player_control_get_associated_data_channel_impl impl =
    (MEEGO_MEDIA_PLAYER_CONTROL_GET_CLASS (self)->get_associated_data_channel);

  if (impl != NULL) {
    (impl) (self, ip, port);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void meego_media_player_control_implement_get_associated_data_channel (MeegoMediaPlayerControlClass *klass,
    meego_media_player_control_get_associated_data_channel_impl impl)
{
  klass->get_associated_data_channel = impl;
}

void
meego_media_player_control_emit_initialized (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Initialized],
                 0);
}
void
meego_media_player_control_emit_eof (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_EOF],
                 0);
}

void
meego_media_player_control_emit_error (gpointer instance,
    guint error_num, gchar *error_des)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Error],
                 0,
                 error_num, error_des);
}

void
meego_media_player_control_emit_buffered (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffered],
                 0);
}
void
meego_media_player_control_emit_buffering (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffering],
                 0);
}

void
meego_media_player_control_emit_player_state_changed (gpointer instance, gint old_state, gint new_state)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_PlayerStateChanged],
                 0, old_state, new_state);
}

void
meego_media_player_control_emit_target_ready (gpointer instance, GHashTable *infos)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TargetReady],
                 0, infos);
}
void
meego_media_player_control_emit_seeked (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Seeked],
                 0);
}
void
meego_media_player_control_emit_stopped (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Stopped],
                 0);
}

void
meego_media_player_control_emit_request_window (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_RequestWindow],
                 0);
}

void
meego_media_player_control_emit_suspended (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Suspended],
                 0);
}

void
meego_media_player_control_emit_restored (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Restored],
                 0);
}

void
meego_media_player_control_emit_video_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_VideoTagChanged],
                 0, channel);
}

void
meego_media_player_control_emit_audio_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_AudioTagChanged],
                 0, channel);
}

void
meego_media_player_control_emit_text_tag_changed (gpointer instance, gint channel)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TextTagChanged],
                 0, channel);
}

void
meego_media_player_control_emit_metadata_changed (gpointer instance)
{
  g_assert (instance != NULL);
  g_assert (G_TYPE_CHECK_INSTANCE_TYPE (instance, MEEGO_TYPE_MEDIA_PLAYER_CONTROL));
  g_signal_emit (instance,
                 meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_MetadataChanged],
                 0);
}

static inline void
meego_media_player_control_base_init_once (gpointer klass G_GNUC_UNUSED)
{

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Initialized] =
    g_signal_new ("initialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_EOF] =
    g_signal_new ("eof",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Error] =
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

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Seeked] =
    g_signal_new ("seeked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Stopped] =
    g_signal_new ("stopped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_RequestWindow] =
    g_signal_new ("request-window",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffering] =
    g_signal_new ("buffering",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Buffered] =
    g_signal_new ("buffered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_PlayerStateChanged] =
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

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TargetReady] =
    g_signal_new ("target-ready",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE));

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Suspended] =
    g_signal_new ("suspended",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_Restored] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_VideoTagChanged] =
    g_signal_new ("video-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_AudioTagChanged] =
    g_signal_new ("audio-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_TextTagChanged] =
    g_signal_new ("text-tag-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  meego_media_player_control_signals[SIGNAL_MEEGO_MEDIA_PLAYER_CONTROL_MetadataChanged] =
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
meego_media_player_control_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
    meego_media_player_control_base_init_once (klass);
  }
}
