#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>

#include "umms-common.h"
#include "umms-debug.h"
#include "meego-media-player-control.h"
#include "engine-gst.h"

const gchar *error_type[] = {
  "ErrorTypeEngine",
  "NumOfErrorType"
};

const gchar *state_name[] = {
  "PlayerStateNull",
  "PlayerStatePaused",
  "PlayerStatePlaying",
  "PlayerStateStopped"
};

#define N_UMMS_METHOD 23

GMainLoop *loop = NULL;
static MeegoMediaPlayerControl *player = NULL;
static gchar args[2][256];

char *method_name[N_UMMS_METHOD] = {
  "SetUri",
  "Play",
  "Pause",
  "Stop",
  "SetPosition",
  "GetPosition",//5
  "SetPlaybackRate",
  "GetPlaybackRate",
  "SetVolume",
  "GetVolume",
  "SetWindowId",//10
  "SetVideoSize",
  "GetVideoSize",
  "GetBufferedTime",
  "GetBufferedBytes",
  "GetMediaSizeTime",//15
  "GetMediaSizeBytes",
  "HasVideo",
  "HasAudio",
  "IsStreaming",
  "IsSeekable",//20
  "SupportFullscreen",
  "GetPlayerState"
};

static void shell_help ()
{
  int i;
  for (i = 0; i < N_UMMS_METHOD; i++) {
    printf ("'%d':  '%s'\n", i, method_name[i]);
  }
  printf ("'?': to display this help info\n");
  printf ("'h': to display this help info\n");
  printf ("'q': to quit\n");
}

#define DEFAULT_URI "http://sh1sskweb001.ccr.corp.intel.com/html5_workloads/workload_mytube/workload/videos/720p.m4v"

void get_param (gchar *method_str, gchar *param)
{
  gboolean need_input = TRUE;
  gint method_id;

  UMMS_DEBUG ("called");

  method_id = atoi(method_str);
  switch (method_id) {
    case 0:
      g_print("Input uri:\n");
      break;
    case 4:
      g_print("Input pos to seek: (seconds)\n");
      break;
    case 6:
      g_print("Input playback rate: \n");
      break;
    case 8:
      g_print("Input volume to set: [0,100]\n");
      break;
    case 11:
      g_print("Input dest reactangle, Example: 0,0,352,288\n");
      break;
    default:
      need_input = FALSE;
      break;
  }
  if (need_input) {
    fscanf (stdin, "%s", param);
    g_print("param is '%s'\n", param);
  }
  return;
}

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

  UMMS_DEBUG ("called");
  method_id = atoi(args[0]);
  switch (method_id) {
    case 0:
      if (args[1][0] == 'd')
        uri = DEFAULT_URI;
      else
        uri = args[1];
      UMMS_DEBUG ("uri = '%s'", uri);
      meego_media_player_control_set_uri (player, uri);
      break;
    case 1:
      meego_media_player_control_play (player);
      break;
    case 2:
      meego_media_player_control_pause (player);
      break;
    case 3:
      meego_media_player_control_stop (player);
      break;
    case 4:
      sscanf (args[1], "%lld", &pos);
      pos = 1000 * pos;
      g_print ("SetPosition to '%lld' ms\n", pos);
      break;
    case 5:
      UMMS_DEBUG ("Position = %lld ms", pos);
      break;
    case 6:
      sscanf (args[1], "%lf", &rate);
      g_print ("Set rate to '%f'\n", rate);
      break;
    case 7:
      UMMS_DEBUG ("rate = %f", rate);
      break;
    case 8:
      sscanf (args[1], "%d", &volume);
      g_print ("SetVolume to '%d'\n", volume);
      break;
    case 9:
      UMMS_DEBUG ("volume = %d", volume);
      break;
    case 10:
      //TODO:This API is for rendering video data to a X window. For this test progrom, do nothing.
      UMMS_DEBUG ("SetWindowId, do nothing in this test");
      break;
    case 11:
      sscanf (args[1], "%u,%u,%u,%u", &x, &y, &w, &h);
      g_print ("SetVideoSize (%u,%u,%u,%u)\n", x, y, w, h);
      break;
    case 12:
      UMMS_DEBUG ("width=%u, height=%u", w, h);
      break;
    case 13:
      UMMS_DEBUG ("buffered time = %lld", size);
      break;
    case 14:
      UMMS_DEBUG ("buffered bytes = %lld", size);
      break;
    case 15:
      UMMS_DEBUG ("size time = %lld", size);
      break;
    case 16:
      UMMS_DEBUG ("size bytes = %lld", size);
      break;
    case 17:
      UMMS_DEBUG ("has_video = %d", has_video);
      break;
    case 18:
      UMMS_DEBUG ("has_audio = %d", has_audio);
      break;
    case 19:
      UMMS_DEBUG ("is_streaming = %d", is_streaming);
      break;
    case 20:
      UMMS_DEBUG ("is_seekable = %d", is_seekable);
      break;
    case 21:
      UMMS_DEBUG ("support functions = %d", support_fullscreen);
      break;
    case 22:
      UMMS_DEBUG ("state = '%s'", state_name[state]);
      break;
    default:
      UMMS_DEBUG ("Unknown method id: %d\n", method_id);
      break;
  }
  return FALSE;
}

void player_state_changed_cb(MeegoMediaPlayerControl *player, gint state, gpointer user_data)
{
  UMMS_DEBUG("State changed to '%s'", state_name[state]);
}

void eof_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "EOF....");
}

void begin_buffering_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "Begin buffering");
}

void buffered_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "Buffering completed");
}

void seeked_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "Seeking completed");
}

void stopped_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "Player stopped");
}

void error_cb(MeegoMediaPlayerControl *player, guint err_id, gchar *msg, gpointer user_data)
{
  UMMS_DEBUG( "Error Domain:'%s', msg='%s'", error_type[err_id], msg);
}

void request_window_cb(MeegoMediaPlayerControl *player, gpointer user_data)
{
  UMMS_DEBUG( "Player engine request a X window");
}

static void
connect_sigs(MeegoMediaPlayerControl *player)
{
    UMMS_DEBUG ("called");
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



static gpointer cmd_thread_func (gpointer data)
{
  gchar buf[32];
  gchar param[256];
  gint  rc;
  static gint mid;
  static to_continue = TRUE;
  gchar *path;


  while (to_continue) {
    g_print ("Input cmd:\n");
    rc = fscanf(stdin, "%s", args[0]);
    if (rc <= 0) {
      g_print("NULL\n");
    }

    if (!strcmp(args[0], "h")) {
      shell_help();
    } else if (!strcmp(args[0], "?")) {
      shell_help();
    } else if (!strcmp(args[0], "q")) {
      g_main_loop_quit (loop);
      printf("quit engine test program\n");
      break;
    } else {
      get_param(args[0], args[1]);
      g_print ("args[0]=%s, args[1]=%s\n", args[0], args[1]);
      g_idle_add (method_call, NULL);
    }
  }
  return NULL;
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
