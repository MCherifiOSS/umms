#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>

#include "../src/umms-common.h"
#include "../src/umms-debug.h"
#include "../src/umms-marshals.h"
#include "../libummsclient/umms-client-object.h"

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

static UmmsClientObject *umms_client_obj = NULL;
static DBusGProxy *player = NULL;
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
  printf ("'r': request media player\n");
  printf ("'u': request unattended media player\n");
  printf ("'d': delete media player\n");
  printf ("'q': to quit\n");
}

#define DEFAULT_URI "file:///root/720p.m4v"

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
  DBusGProxy *player_control;
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
      if (args[1][0] == '\0')
        uri = DEFAULT_URI;
      else
        uri = args[1];
      UMMS_DEBUG ("uri = '%s'", uri);
      if (!dbus_g_proxy_call (player, "SetUri", &error,
           G_TYPE_STRING, uri, G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to SetUri", error);
      break;
    case 1:
      if (!dbus_g_proxy_call (player, "Play", &error,
           G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to Play", error);
      break;
    case 2:
      if (!dbus_g_proxy_call (player, "Pause", &error,
           G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to Pause", error);
      break;
    case 3:
      if (!dbus_g_proxy_call (player, "Stop", &error,
           G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to Stop", error);
      break;
    case 4:
      sscanf (args[1], "%lld", &pos);
      pos = 1000 * pos;
      g_print ("SetPosition to '%lld' ms\n", pos);
      if (!dbus_g_proxy_call (player, "SetPosition", &error,
           G_TYPE_INT64, pos, G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to SetPosition", error);
      break;
    case 5:
      if (!dbus_g_proxy_call (player, "GetPosition", &error,
           G_TYPE_INVALID,
           G_TYPE_INT64, &pos, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetPosition", error);
      UMMS_DEBUG ("Position = %lld ms", pos);
      break;
    case 6:
      sscanf (args[1], "%lf", &rate);
      g_print ("Set rate to '%f'\n", rate);
      if (!dbus_g_proxy_call (player, "SetPlaybackRate", &error,
           G_TYPE_DOUBLE, rate, G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to SetPlaybackRate", error);
      break;
    case 7:
      if (!dbus_g_proxy_call (player, "GetPlaybackRate", &error,
           G_TYPE_INVALID,
           G_TYPE_DOUBLE, &rate, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetPlaybackRate", error);
      UMMS_DEBUG ("rate = %f", rate);
      break;
    case 8:
      sscanf (args[1], "%d", &volume);
      g_print ("SetVolume to '%d'\n", volume);
      if (!dbus_g_proxy_call (player, "SetVolume", &error,
           G_TYPE_INT, volume, G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to SetVolume", error);
      break;
    case 9:
      if (!dbus_g_proxy_call (player, "GetVolume", &error,
           G_TYPE_INVALID,
           G_TYPE_INT, &volume, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetVolume", error);
      UMMS_DEBUG ("volume = %d", volume);
      break;
    case 10:
      //TODO:This API is for rendering video data to a X window. For this test progrom, do nothing.
      UMMS_DEBUG ("SetWindowId, do nothing in this test");
      break;
    case 11:
      sscanf (args[1], "%u,%u,%u,%u", &x, &y, &w, &h);
      g_print ("SetVideoSize (%u,%u,%u,%u)\n", x, y, w, h);
      if (!dbus_g_proxy_call (player, "SetVideoSize", &error,
           G_TYPE_UINT, x,
           G_TYPE_UINT, y,
           G_TYPE_UINT, w,
           G_TYPE_UINT, h,
           G_TYPE_INVALID,
           G_TYPE_INVALID))
        UMMS_GERROR ("SetVideoSize failed", error);
      break;
    case 12:
      if (!dbus_g_proxy_call (player, "GetVideoSize", &error,
           G_TYPE_INVALID,
           G_TYPE_UINT, &w, G_TYPE_UINT, &h, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetVideoSize", error);
      UMMS_DEBUG ("width=%u, height=%u", w, h);
      break;
    case 13:
      if (!dbus_g_proxy_call (player, "GetBufferedTime", &error,
           G_TYPE_INVALID,
           G_TYPE_INT64, &size, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetBufferedTime", error);
      UMMS_DEBUG ("buffered time = %lld", size);
      break;
    case 14:
      if (!dbus_g_proxy_call (player, "GetBufferedBytes", &error,
           G_TYPE_INVALID,
           G_TYPE_INT64, &size,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetBufferedBytes", error);
      UMMS_DEBUG ("buffered bytes = %lld", size);
      break;
    case 15:
      if (!dbus_g_proxy_call (player, "GetMediaSizeTime", &error,
           G_TYPE_INVALID,
           G_TYPE_INT64, &size,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetMediaSizeTime", error);
      UMMS_DEBUG ("size time = %lld", size);
      break;
    case 16:
      if (!dbus_g_proxy_call (player, "GetMediaSizeBytes", &error,
           G_TYPE_INVALID,
           G_TYPE_INT64, &size,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetMediaSizeBytes", error);
      UMMS_DEBUG ("size bytes = %lld", size);
      break;
    case 17:
      if (!dbus_g_proxy_call (player, "HasVideo", &error,
           G_TYPE_INVALID,
           G_TYPE_BOOLEAN, &has_video, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to HasVideo", error);
      UMMS_DEBUG ("has_video = %d", has_video);
      break;
    case 18:
      if (!dbus_g_proxy_call (player, "HasAudio", &error,
           G_TYPE_INVALID,
           G_TYPE_BOOLEAN, &has_audio, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to HasAudio", error);
      UMMS_DEBUG ("has_audio = %d", has_audio);
      break;
    case 19:
      if (!dbus_g_proxy_call (player, "IsStreaming", &error,
           G_TYPE_INVALID,
           G_TYPE_BOOLEAN, &is_streaming, G_TYPE_INVALID))
        UMMS_GERROR ("Failed to IsStreaming", error);
      UMMS_DEBUG ("is_streaming = %d", is_streaming);
      break;
    case 20:
      if (!dbus_g_proxy_call (player, "IsSeekable", &error,
           G_TYPE_INVALID,
           G_TYPE_BOOLEAN, &is_seekable,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to IsSeekable", error);
      UMMS_DEBUG ("is_seekable = %d", is_seekable);
      break;
    case 21:
      if (!dbus_g_proxy_call (player, "SupportFullscreen", &error,
           G_TYPE_INVALID,
           G_TYPE_BOOLEAN, &support_fullscreen,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to SupportFullscreen", error);
      UMMS_DEBUG ("support functions = %d", support_fullscreen);
      break;
    case 22:
      if (!dbus_g_proxy_call (player, "GetPlayerState", &error,
           G_TYPE_INVALID,
           G_TYPE_INT, &state,
           G_TYPE_INVALID))
        UMMS_GERROR ("Failed to GetPlayerState", error);
      UMMS_DEBUG ("state = '%s'", state_name[state]);
      break;
    default:
      UMMS_DEBUG ("Unknown method id: %d\n", method_id);
      break;
  }
  return FALSE;
}

void initialized_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG ("MeidaPlayer initialized");
}


void player_state_changed_cb(DBusGProxy *player, gint state, gpointer user_data)
{
  UMMS_DEBUG("State changed to '%s'", state_name[state]);
}

void eof_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "EOF....");
}

void begin_buffering_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "Begin buffering");
}

void buffered_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "Buffering completed");
}

void seeked_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "Seeking completed");
}

void stopped_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "Player stopped");
}

void error_cb(DBusGProxy *player, guint err_id, gchar *msg, gpointer user_data)
{
  UMMS_DEBUG( "Error Domain:'%s', msg='%s'", error_type[err_id], msg);
}

void request_window_cb(DBusGProxy *player, gpointer user_data)
{
  UMMS_DEBUG( "Player engine request a X window");
}

static void
add_sigs(DBusGProxy *player)
{
  dbus_g_proxy_add_signal (player, "Initialized", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Eof", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Buffering", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Buffered", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "RequestWindow", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Seeked", G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Stopped", G_TYPE_INVALID);

  dbus_g_object_register_marshaller (umms_marshal_VOID__UINT_STRING, G_TYPE_NONE, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "Error", G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (player, "PlayerStateChanged", G_TYPE_INT, G_TYPE_INVALID);
}

static void
connect_sigs(DBusGProxy *player)
{
  UMMS_DEBUG ("called");
  dbus_g_proxy_connect_signal (player,
      "Initialized",
      G_CALLBACK(initialized_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Eof",
      G_CALLBACK(eof_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Buffering",
      G_CALLBACK(begin_buffering_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Buffered",
      G_CALLBACK(buffered_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "RequestWindow",
      G_CALLBACK(request_window_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Seeked",
      G_CALLBACK(seeked_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Stopped",
      G_CALLBACK(stopped_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "Error",
      G_CALLBACK(error_cb),
      NULL, NULL);

  dbus_g_proxy_connect_signal (player,
      "PlayerStateChanged",
      G_CALLBACK(player_state_changed_cb),
      NULL, NULL);
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
      printf("quit client program\n");
      exit(0);
    } else if (!strcmp(args[0], "u")) {
      UMMS_DEBUG ("request unattended media player");
      player = umms_client_object_request_player (umms_client_obj, FALSE, 5, &path);
      UMMS_DEBUG ("path = '%s'", path);
      g_free(path);
    } else if (!strcmp(args[0], "r")) {
      UMMS_DEBUG ("request media player");
      player = umms_client_object_request_player (umms_client_obj, TRUE, 0, &path);
      UMMS_DEBUG ("path = '%s'", path);
      g_free(path);
    } else if (!strcmp(args[0], "d")) {
      UMMS_DEBUG ("remove media player");
      umms_client_object_remove_player (umms_client_obj, player);
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
  GMainLoop *loop;
  GError *error = NULL;
  guint i;
  gchar *obj_path;
  GThread *cmd_thread;

  g_type_init ();

  umms_client_obj = umms_client_object_new();
  player = umms_client_object_request_player (umms_client_obj, TRUE, 0, &obj_path);
  add_sigs (player);
  connect_sigs (player);

  loop = g_main_loop_new (NULL, TRUE);

  cmd_thread = g_thread_create (cmd_thread_func, NULL, FALSE, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (G_OBJECT (player));

  exit(0);
}
