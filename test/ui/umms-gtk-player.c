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

#include <glib.h>
#include <gtk/gtk.h>
#include "umms-gtk-player.h"
#include "gtk-backend.h"

gchar dbg_state_name[][30] = {
  "PLY_MAIN_STATE_IDLE",
  "PLY_MAIN_STATE_READY",
  "PLY_MAIN_STATE_RUN",
  "PLY_MAIN_STATE_PAUSE",
};

static PlyMainData ply_main_data;

gint ply_init(void)
{
  ply_main_data.state_lock = g_mutex_new ();
  ply_main_data.state = PLY_MAIN_STATE_IDLE;
  ply_main_data.play_speed = 1;
  avdec_init();
  return 0;
}

static void ply_update_duration(void * dummy)
{
  gint64 pos, len, volume;

  if (ply_get_state() == PLY_MAIN_STATE_RUN) {
    avdec_get_duration(&len);
    avdec_get_position(&pos);
    ui_update_progressbar(pos, len);
  }
}


PlyMainState ply_get_state(void)
{
  g_mutex_lock (ply_main_data.state_lock);
  PlyMainState state = ply_main_data.state;
  g_mutex_unlock (ply_main_data.state_lock);
  return state;
}


void ply_set_state(PlyMainState state)
{
  static guint dur_timeout_id = 0;

  g_mutex_lock (ply_main_data.state_lock);
  ply_main_data.state = state;
  g_print("%s\n", dbg_state_name[ply_main_data.state]);

  if (state == PLY_MAIN_STATE_RUN) {
    if (dur_timeout_id == 0) {
      dur_timeout_id = g_timeout_add(1000, (GSourceFunc)ply_update_duration, NULL);
    }
  } else {
    if (dur_timeout_id != 0) {
      g_source_remove (dur_timeout_id);
      dur_timeout_id = 0;
    }
  }

  g_mutex_unlock (ply_main_data.state_lock);
}


gint ply_get_speed(void)
{
  g_mutex_lock (ply_main_data.state_lock);
  gint speed = ply_main_data.play_speed;
  g_mutex_unlock (ply_main_data.state_lock);
  return speed;
}


void ply_set_speed(gint speed)
{
  g_mutex_lock (ply_main_data.state_lock);
  ply_main_data.play_speed = speed;
  g_mutex_unlock (ply_main_data.state_lock);
}


gint64 ply_get_duration(void)
{
  g_mutex_lock (ply_main_data.state_lock);
  gint64 len = ply_main_data.duration_nanosecond;
  g_mutex_unlock (ply_main_data.state_lock);
  return len;
}


void ply_set_duration(gint64 len)
{
  g_mutex_lock (ply_main_data.state_lock);
  ply_main_data.duration_nanosecond = len;
  g_mutex_unlock (ply_main_data.state_lock);
}


gint ply_reload_file(gchar *filename)
{
  if (ply_get_state() != PLY_MAIN_STATE_IDLE &&
      ply_get_state() != PLY_MAIN_STATE_READY) {
    ply_stop_stream();
  }

  //g_string_assign(&ply_main_data.filename, filename);
  avdec_set_source(filename);
  ply_main_data.state = PLY_MAIN_STATE_READY;

  g_print("%s\n", dbg_state_name[ply_main_data.state]);
  return 0;
}

gint ply_set_subtitle(gchar *filename)
{
  if (ply_get_state() == PLY_MAIN_STATE_IDLE ||
      ply_get_state() == PLY_MAIN_STATE_READY) {
    avdec_set_subtitle (filename);
  }

  return 0;
}

gint ply_play_stream(void)
{
  avdec_start();

  return 0;
}

gint ply_stop_stream(void)
{
  avdec_stop();
  return 0;
}

gint ply_pause_stream(void)
{
  avdec_pause();
  return 0;
}

gint ply_resume_stream(void)
{
  avdec_resume();
  return 0;
}

gint ply_seek_stream_from_beginging(gint64 nanosecond)
{
  avdec_seek_from_beginning(nanosecond);
  return 0;
}


gint ply_forward_rewind(gint speed)
{
  avdec_set_speed(speed);
  return 0;
}


gint ply_get_play_speed(void)
{
  return avdec_get_speed();
}

gint ply_get_video_num(void)
{
  gint video_num = 0;
  avdec_get_video_num(&video_num);
  return video_num;
}

gint ply_get_cur_video(void)
{
  gint cur_video = 0;
  avdec_get_cur_video(&cur_video);
  return cur_video;
}

gint ply_set_cur_video(gint cur_video)
{
  return avdec_set_cur_video(cur_video);
}

gchar* ply_get_video_codec(gint video_num)
{
  gchar * codec = 0;
  avdec_get_video_codec(video_num, &codec);
  return codec;
}

gint ply_get_audio_num(void)
{
  gint audio_num = 0;
  avdec_get_audio_num(&audio_num);
  return audio_num;
}

gint ply_get_cur_audio(void)
{
  gint cur_audio = 0;
  avdec_get_cur_audio(&cur_audio);
  return cur_audio;
}

gint ply_set_cur_audio(gint cur_audio)
{
  return avdec_set_cur_audio(cur_audio);
}

gchar* ply_get_audio_codec(gint audio_num)
{
  gchar * codec = 0;
  avdec_get_audio_codec(audio_num, &codec);
  return codec;
}

gchar* ply_get_container_name(void)
{
  gchar * name = 0;
  avdec_get_encapsulation(&name);
  return name;
}

gint ply_get_video_bitrate(gint video_num)
{
  int bitrate = 0;
  avdec_get_video_bitrate(video_num, &bitrate);
  return bitrate;
}

gdouble ply_get_video_framerate(gint video_num)
{
  gint rate_num, rate_denom;
  avdec_get_video_framerate(video_num, &rate_num, &rate_denom);
  return ((gdouble)rate_num) / ((gdouble)rate_denom);
}

gint ply_get_video_resolution(gint video_num, gint *width, gint *height)
{
  return avdec_get_video_resolution(video_num, width, height);
}

gint ply_get_video_aspectratio(gint video_num, gint *width, gint *height)
{
  return avdec_get_video_aspectratio(video_num, width, height);
}

gint ply_get_audio_bitrate(gint audio_num)
{
  int bitrate = 0;
  avdec_get_audio_bitrate(audio_num, &bitrate);
  return bitrate;
}

gint ply_get_audio_samplerate(gint audio_num)
{
  int sample_rate = 0;
  avdec_get_audio_samplerate(audio_num, &sample_rate);
  return sample_rate;
}

gchar* ply_get_current_uri(void)
{
  gchar * uri = NULL;
  avdec_get_current_uri(&uri);
  return uri;
}

gchar* ply_get_protocol_name(void)
{
  gchar * name = NULL;
  avdec_get_protocol_name(&name);
  return name;
}

gint ply_set_volume(gint volume)
{
  return avdec_set_volume(volume);
}

gint ply_get_volume(void)
{
  gint volume;
  avdec_get_volume(&volume);
  return volume;
}

gint ply_get_rawdata_address(gchar ** ip_addr, gint * port)
{
  avdec_get_rawdata_address(ip_addr, port);
  return 0;
}

GArray *ply_get_pat(void)
{
  GArray * pat;
  avdec_get_pat(&pat);
  return pat;
}

gint ply_get_pmt(guint *program_num, guint *pcr_pid, GArray **array)
{
  return avdec_get_pmt(program_num, pcr_pid, array);
}

