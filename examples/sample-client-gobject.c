#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>

#include "../src/umms-marshals.h"
#include "../src/umms-debug.h"
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

GMainLoop *loop;
static UmmsClientObject *umms_client_obj = NULL;
static DBusGProxy *player = NULL;

#define DEFAULT_URI "file:///root/720p.m4v"

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

static gboolean quit_play (gpointer data)
{
  GError *error = NULL;
  if (!dbus_g_proxy_call (player, "Stop", &error,
       G_TYPE_INVALID,
       G_TYPE_INVALID)) {
    UMMS_GERROR ("Failed to Stop", error);
  }

  umms_client_object_remove_player (umms_client_obj, player);
  g_main_loop_quit (loop);
  return FALSE;
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

  if (!dbus_g_proxy_call (player, "SetUri", &error,
       G_TYPE_STRING, DEFAULT_URI, G_TYPE_INVALID,
       G_TYPE_INVALID)) {
    UMMS_GERROR ("Failed to SetUri", error);
  }

  if (!dbus_g_proxy_call (player, "Play", &error,
       G_TYPE_INVALID,
       G_TYPE_INVALID)) {
    UMMS_GERROR ("Failed to Play", error);
  }

  g_timeout_add (10000, quit_play, NULL);

  loop = g_main_loop_new (NULL, TRUE);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (G_OBJECT (player));

  exit(0);
}
