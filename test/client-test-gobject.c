#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>

#include "test-common.h"
#include "../src/umms-marshals.h"
#include "../libummsclient/umms-client-object.h"

extern gboolean to_continue;
static UmmsClientObject *umms_client_obj = NULL;
static DBusGProxy *player = NULL;
GMainLoop *loop;

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
      if (!dbus_g_proxy_call (player, "SetUri", &error,
            G_TYPE_STRING, uri, G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SetUri failed", error);
      }
      break;
    case Play:
      if (!dbus_g_proxy_call (player, "Play", &error,
            G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("Play failed", error);
      }
      break;
    case Pause:
      if (!dbus_g_proxy_call (player, "Pause", &error,
            G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("Pause failed", error);
      }
      break;
    case Stop:
      if (!dbus_g_proxy_call (player, "Stop", &error,
            G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("Stop failed", error);
      }
      break;
    case SetPosition:
      sscanf (args[1], "%lld", &pos);
      pos = 1000 * pos;
      g_print ("SetPosition to '%lld' ms\n", pos);
      if (!dbus_g_proxy_call (player, "SetPosition", &error,
            G_TYPE_INT64, pos, G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SetPosition failed", error);
      }
      break;
    case GetPosition:
      if (!dbus_g_proxy_call (player, "GetPosition", &error,
            G_TYPE_INVALID,
            G_TYPE_INT64, &pos, G_TYPE_INVALID)) {
        TESTER_ERROR ("GetPosition failed", error);
      } else {
        TESTER_DEBUG ("Position = %lld ms", pos);
      }
      break;
    case SetPlaybackRate:
      sscanf (args[1], "%lf", &rate);
      g_print ("Set rate to '%f'\n", rate);
      if (!dbus_g_proxy_call (player, "SetPlaybackRate", &error,
            G_TYPE_DOUBLE, rate, G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SetPlaybackRate failed", error);
      }
      break;
    case GetPlaybackRate:
      if (!dbus_g_proxy_call (player, "GetPlaybackRate", &error,
            G_TYPE_INVALID,
            G_TYPE_DOUBLE, &rate, G_TYPE_INVALID)){
        TESTER_ERROR ("GetPlaybackRate failed", error);
      } else {
        TESTER_DEBUG ("rate = %f", rate);
      }
      break;
    case SetVolume:
      sscanf (args[1], "%d", &volume);
      g_print ("SetVolume to '%d'\n", volume);
      if (!dbus_g_proxy_call (player, "SetVolume", &error,
            G_TYPE_INT, volume, G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SetVolume failed", error);
      }
      break;
    case GetVolume:
      if (!dbus_g_proxy_call (player, "GetVolume", &error,
            G_TYPE_INVALID,
            G_TYPE_INT, &volume, G_TYPE_INVALID)){
        TESTER_ERROR ("GetVolume failed", error);
      } else {
        TESTER_DEBUG ("volume = %d", volume);
      }
      break;
    case SetWindowId:
      //TODO:This API is for rendering video data to a X window. For this test progrom, do nothing.
      TESTER_DEBUG ("SetWindowId, do nothing in this test");
      break;
    case SetVideoSize:
      sscanf (args[1], "%u,%u,%u,%u", &x, &y, &w, &h);
      g_print ("SetVideoSize (%u,%u,%u,%u)\n", x, y, w, h);
      if (!dbus_g_proxy_call (player, "SetVideoSize", &error,
            G_TYPE_UINT, x,
            G_TYPE_UINT, y,
            G_TYPE_UINT, w,
            G_TYPE_UINT, h,
            G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SetVideoSize failed", error);
      }
      break;
    case GetVideoSize:
      if (!dbus_g_proxy_call (player, "GetVideoSize", &error,
            G_TYPE_INVALID,
            G_TYPE_UINT, &w, G_TYPE_UINT, &h, G_TYPE_INVALID)){
        TESTER_ERROR ("GetVideoSize failed", error);
      } else {
        TESTER_DEBUG ("width=%u, height=%u", w, h);
      }
      break;
    case GetBufferedTime:
      if (!dbus_g_proxy_call (player, "GetBufferedTime", &error,
            G_TYPE_INVALID,
            G_TYPE_INT64, &size, G_TYPE_INVALID)) {
        TESTER_ERROR ("GetBufferedTime failed", error);
      } else {
        TESTER_DEBUG ("buffered time = %lld", size);
      }
      break;
    case GetBufferedBytes:
      if (!dbus_g_proxy_call (player, "GetBufferedBytes", &error,
            G_TYPE_INVALID,
            G_TYPE_INT64, &size,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("GetBufferedBytes failed", error);
      } else {
        TESTER_DEBUG ("buffered bytes = %lld", size);
      }
      break;
    case GetMediaSizeTime:
      if (!dbus_g_proxy_call (player, "GetMediaSizeTime", &error,
            G_TYPE_INVALID,
            G_TYPE_INT64, &size,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("GetMediaSizeTime failed", error);
      } else {
        TESTER_DEBUG ("size time = %lld", size);
      }
      break;
    case GetMediaSizeBytes:
      if (!dbus_g_proxy_call (player, "GetMediaSizeBytes", &error,
            G_TYPE_INVALID,
            G_TYPE_INT64, &size,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("GetMediaSizeBytes failed", error);
      } else {
        TESTER_DEBUG ("size bytes = %lld", size);
      }
      break;
    case HasVideo:
      if (!dbus_g_proxy_call (player, "HasVideo", &error,
            G_TYPE_INVALID,
            G_TYPE_BOOLEAN, &has_video, G_TYPE_INVALID)) {
        TESTER_ERROR ("HasVideo failed", error);
      } else {
        TESTER_DEBUG ("has_video = %d", has_video);
      }
      break;
    case HasAudio:
      if (!dbus_g_proxy_call (player, "HasAudio", &error,
            G_TYPE_INVALID,
            G_TYPE_BOOLEAN, &has_audio, G_TYPE_INVALID)) {
        TESTER_ERROR ("HasAudio failed", error);
      } else {
        TESTER_DEBUG ("has_audio = %d", has_audio);
      }
      break;
    case IsStreaming:
      if (!dbus_g_proxy_call (player, "IsStreaming", &error,
            G_TYPE_INVALID,
            G_TYPE_BOOLEAN, &is_streaming, G_TYPE_INVALID)) {
        TESTER_ERROR ("IsStreaming failed", error);
      } else {
        TESTER_DEBUG ("is_streaming = %d", is_streaming);
      }
      break;
    case IsSeekable:
      if (!dbus_g_proxy_call (player, "IsSeekable", &error,
            G_TYPE_INVALID,
            G_TYPE_BOOLEAN, &is_seekable,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("IsSeekable failed", error);
      } else {
        TESTER_DEBUG ("is_seekable = %d", is_seekable);
      }
      break;
    case SupportFullscreen:
      if (!dbus_g_proxy_call (player, "SupportFullscreen", &error,
            G_TYPE_INVALID,
            G_TYPE_BOOLEAN, &support_fullscreen,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("SupportFullscreen failed", error);
      } else {
        TESTER_DEBUG ("SupportFullscreen = %d", support_fullscreen);
      }
      break;
    case GetPlayerState:
      if (!dbus_g_proxy_call (player, "GetPlayerState", &error,
            G_TYPE_INVALID,
            G_TYPE_INT, &state,
            G_TYPE_INVALID)) {
        TESTER_ERROR ("GetPlayerState failed", error);
      } else {
        TESTER_DEBUG ("state = '%s'", state_name[state]);
      }
      break;
    case SetTarget:
      TESTER_DEBUG ("SetTarget succeed");
      break;
    default:
      TESTER_DEBUG ("Unknown method id: %d\n", method_id);
      break;
  }
  return FALSE;
}

void initialized_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG ("MeidaPlayer initialized");
}


void player_state_changed_cb(DBusGProxy *player, gint state, gpointer user_data)
{
  TESTER_DEBUG("State changed to '%s'", state_name[state]);
}

void eof_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "EOF....");
}

void begin_buffering_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "Begin buffering");
}

void buffered_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "Buffering completed");
}

void seeked_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "Seeking completed");
}

void stopped_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "Player stopped");
}

void error_cb(DBusGProxy *player, guint err_id, gchar *msg, gpointer user_data)
{
  TESTER_DEBUG( "Error: Domain='Engine', Type='%s', msg='%s'", engine_error_str[err_id], msg);
}

void request_window_cb(DBusGProxy *player, gpointer user_data)
{
  TESTER_DEBUG( "Player engine request a X window");
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
  TESTER_DEBUG ("called");
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



main (int argc, char **argv)
{
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
