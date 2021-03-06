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


#ifndef PLAYER_CORE_H_
#define PLAYER_CORE_H_
typedef enum _PlyMainState {
  PLY_MAIN_STATE_IDLE,
  PLY_MAIN_STATE_READY,
  PLY_MAIN_STATE_RUN,
  PLY_MAIN_STATE_PAUSE,
} PlyMainState;

typedef struct _PlyMainData {
  GMutex *state_lock;
  PlyMainState state;
  GString filename;
  gint64 duration_nanosecond;
  gint play_speed;
} PlyMainData;

/***
 * @brief Play Core module init
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_init(void);

/***
 * @brief Get player main state
 *
 * @return PlyMainState
 */
PlyMainState ply_get_state(void);

/***
 * @brief Reload file and play it, this function returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_reload_file(gchar *file_name);

gint ply_set_subtitle(gchar *filename);

/***
 * @brief Play the stream from the very beginning, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_play_stream(void);

/***
 * @brief Stop the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_stop_stream(void);

/***
 * @brief Pause the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_pause_stream(void);

/***
 * @brief Resume the paused stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_resume_stream(void);

/***
 * @brief Seek the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @param ts if negative means rewind, if positive means forward, the unit is second
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_seek_stream_from_beginging(gint64 nanosecond);


gint ply_forward_rewind(gint speed);

gint ply_get_play_speed(void);

gint ply_get_video_num(void);

gint ply_get_cur_video(void);

gint ply_set_cur_video(gint cur_video);

gchar* ply_get_video_codec(gint video_num);

gint ply_get_audio_num(void);

gint ply_get_cur_audio(void);

gint ply_set_cur_audio(gint cur_audio);

gchar* ply_get_audio_codec(gint audio_num);

gchar* ply_get_container_name(void);

gint ply_get_video_bitrate(gint video_num);

gdouble ply_get_video_framerate(gint video_num);

gint ply_get_video_resolution(gint video_num, gint *width, gint *height);

gint ply_get_video_aspectratio(gint video_num, gint *width, gint *height);

gint ply_get_audio_bitrate(gint audio_num);

gint ply_get_audio_samplerate(gint audio_num);

gchar* ply_get_current_uri(void);

gchar* ply_get_protocol_name(void);

gint ply_set_volume(gint volume);

gint ply_get_volume(void);

gint ply_get_rawdata_address(gchar ** ip_addr, gint * port);

GArray *ply_get_pat(void);

gint ply_get_pmt(guint *program_num, guint *pcr_pid, GArray **array);

#endif /* PLAYER_CORE_H_ */
