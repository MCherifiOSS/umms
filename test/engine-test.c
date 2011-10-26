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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-object.h>
#include <gst/gst.h>

#include "test-common.h"
#include "../src/umms-debug.h"
#include "../src/media-player-control.h"
#include "../src/player-control-tv.h"

extern gboolean to_continue;
GMainLoop *loop = NULL;
static MediaPlayerControl *player = NULL;
gboolean method_call (gpointer data)
{
  gchar *uri;
  gint64 pos = 0;
  gint64 size = 0;
  gint   volume, state;
  gdouble rate = 1.0;
  guint   x = 0, y = 0, w = 352, h = 288;
  gboolean has_video = FALSE;
  gboolean has_audio = FALSE;
  gboolean support_fullscreen = FALSE;
  gboolean is_seekable = FALSE;
  gboolean is_streaming = FALSE;
  gint method_id;

  TESTER_DEBUG ("called");

  if (!strcmp(args[0], "q")) {
    TESTER_DEBUG ("Quit testing");
    to_continue = FALSE;//stop cmd thead
    g_main_loop_quit (loop);
    return FALSE;
  }

  method_id = atoi(args[0]);
  switch (method_id) {
    case SetUri:
      if (args[1][0] == 'd')
        uri = DEFAULT_URI;
      else
        uri = args[1];
      TESTER_DEBUG ("uri = '%s'", uri);
      media_player_control_set_uri (player, uri);
      break;
    case Play:
      media_player_control_play (player);
      break;
    case Pause:
      media_player_control_pause (player);
      break;
    case Stop:
      media_player_control_stop (player);
      break;
    case SetPosition:
      sscanf (args[1], "%lld", &pos);
      pos = 1000 * pos;
      g_print ("SetPosition to '%lld' ms\n", pos);
      break;
    case GetPosition:
      TESTER_DEBUG ("Position = %lld ms", pos);
      break;
    case SetPlaybackRate:
      sscanf (args[1], "%lf", &rate);
      g_print ("Set rate to '%f'\n", rate);
      break;
    case GetPlaybackRate:
      TESTER_DEBUG ("rate = %f", rate);
      break;
    case SetVolume:
      sscanf (args[1], "%d", &volume);
      g_print ("SetVolume to '%d'\n", volume);
      break;
    case GetVolume:
      TESTER_DEBUG ("volume = %d", volume);
      break;
    case SetWindowId:
      //TODO:This API is for rendering video data to a X window. For this test progrom, do nothing.
      TESTER_DEBUG ("SetWindowId, do nothing in this test");
      break;
    case SetVideoSize:
      sscanf (args[1], "%u,%u,%u,%u", &x, &y, &w, &h);
      g_print ("SetVideoSize (%u,%u,%u,%u)\n", x, y, w, h);
      break;
    case GetVideoSize:
      TESTER_DEBUG ("width=%u, height=%u", w, h);
      break;
    case GetBufferedTime:
      TESTER_DEBUG ("buffered time = %lld", size);
      break;
    case GetBufferedBytes:
      TESTER_DEBUG ("buffered bytes = %lld", size);
      break;
    case GetMediaSizeTime:
      TESTER_DEBUG ("size time = %lld", size);
      break;
    case GetMediaSizeBytes:
      TESTER_DEBUG ("size bytes = %lld", size);
      break;
    case HasVideo:
      TESTER_DEBUG ("has_video = %d", has_video);
      break;
    case HasAudio:
      TESTER_DEBUG ("has_audio = %d", has_audio);
      break;
    case IsStreaming:
      TESTER_DEBUG ("is_streaming = %d", is_streaming);
      break;
    case IsSeekable:
      TESTER_DEBUG ("is_seekable = %d", is_seekable);
      break;
    case SupportFullscreen:
      TESTER_DEBUG ("support fullscreen = %d", support_fullscreen);
      break;
    case GetPlayerState:
      TESTER_DEBUG ("state = '%s'", state_name[state]);
      break;
    case SetTarget:
      //TESTER_DEBUG ("target_type=%d", ReservedType0);
      //media_player_control_set_target (player, ReservedType0, 0);
      break;
    default:
      TESTER_DEBUG ("Unknown method id: %d\n", method_id);
      break;
  }
  return FALSE;
}

void player_state_changed_cb(MediaPlayerControl *player, gint state, gpointer user_data)
{
  TESTER_DEBUG("State changed to '%s'", state_name[state]);
}

void eof_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "EOF....");
}

void begin_buffering_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Begin buffering");
}

void buffered_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Buffering completed");
}

void seeked_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Seeking completed");
}

void stopped_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Player stopped");
}

void error_cb(MediaPlayerControl *player, guint err_id, gchar *msg, gpointer user_data)
{
  TESTER_DEBUG( "Error: Domain='Engine', Type='%s', msg='%s'", engine_error_str[err_id], msg);
}

void request_window_cb(MediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Player engine request a X window");
}

static void
connect_sigs(MediaPlayerControl *player)
{
    TESTER_DEBUG ("called");
    g_signal_connect_object (player, "request-window",
            G_CALLBACK (request_window_cb),
            player,
            0);
    g_signal_connect_object (player, "eof",
            G_CALLBACK (eof_cb),
            player,
            0);
    g_signal_connect_object (player, "error",
            G_CALLBACK (error_cb),
            player,
            0);

    g_signal_connect_object (player, "seeked",
            G_CALLBACK (seeked_cb),
            player,
            0);

    g_signal_connect_object (player, "stopped",
            G_CALLBACK (stopped_cb),
            player,
            0);

    g_signal_connect_object (player, "buffering",
            G_CALLBACK (begin_buffering_cb),
            player,
            0);

    g_signal_connect_object (player, "buffered",
            G_CALLBACK (buffered_cb),
            player,
            0);
    g_signal_connect_object (player, "player-state-changed",
            G_CALLBACK (player_state_changed_cb),
            player,
            0);
}

int main (int argc, char **argv)
{
  GThread *cmd_thread;

  g_type_init ();
  gst_init (&argc, &argv);

  player = (MediaPlayerControl*)player_control_tv_new();
  connect_sigs (player);

  loop = g_main_loop_new (NULL, TRUE);

  cmd_thread = g_thread_create (cmd_thread_func, NULL, FALSE, NULL);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  g_object_unref (G_OBJECT (player));


  exit(0);
}
