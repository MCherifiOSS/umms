#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>

#include "test-common.h"
#include "../src/umms-debug.h"
#include "../src/meego-media-player-control.h"
#include "../src/engine-gst.h"

extern to_continue;
GMainLoop *loop = NULL;
static MeegoMediaPlayerControl *player = NULL;
gboolean method_call (gpointer data)
{
  gchar *obj_path, *token;
  gchar *uri;
  GError *error = NULL;
  static gint i = 0;
  gint64 pos, size;
  gint   volume, state;
  gdouble rate = 1.0;
  guint   x = 0, y = 0, w = 352, h = 288;
  gboolean has_video, has_audio, support_fullscreen, is_seekable, is_streaming;
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
      meego_media_player_control_set_uri (player, uri);
      break;
    case Play:
      meego_media_player_control_play (player);
      break;
    case Pause:
      meego_media_player_control_pause (player);
      break;
    case Stop:
      meego_media_player_control_stop (player);
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
      //meego_media_player_control_set_target (player, ReservedType0, 0);
      break;
    default:
      TESTER_DEBUG ("Unknown method id: %d\n", method_id);
      break;
  }
  return FALSE;
}

void player_state_changed_cb(MeegoMediaPlayerControl *player, gint state, gpointer user_data)
{
  TESTER_DEBUG("State changed to '%s'", state_name[state]);
}

void eof_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "EOF....");
}

void begin_buffering_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Begin buffering");
}

void buffered_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Buffering completed");
}

void seeked_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Seeking completed");
}

void stopped_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Player stopped");
}

void error_cb(MeegoMediaPlayerControl *player, guint err_id, gchar *msg, gpointer user_data)
{
  TESTER_DEBUG( "Error Domain:'%s', msg='%s'", error_type[err_id], msg);
}

void request_window_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  TESTER_DEBUG( "Player engine request a X window");
}

static void
connect_sigs(MeegoMediaPlayerControl *player)
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

main (int argc, char **argv)
{
  GError *error = NULL;
  guint i;
  gchar *obj_path;
  GThread *cmd_thread;

  g_type_init ();
  gst_init (&argc, &argv);

  player = (MeegoMediaPlayerControl*)engine_gst_new();
  connect_sigs (player);

  loop = g_main_loop_new (NULL, TRUE);

  cmd_thread = g_thread_create (cmd_thread_func, NULL, FALSE, NULL);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  g_object_unref (G_OBJECT (player));


  exit(0);
}
