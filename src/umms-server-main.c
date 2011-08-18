
#include <string.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdio.h>

#include "umms-common.h"
#include "umms-debug.h"
#include "umms-object-manager.h"
#include "umms-playing-content-metadata-viewer.h"
#include "./glue/umms-object-manager-glue.h"
#include "./glue/umms-playing-content-metadata-viewer-glue.h"

static GMainLoop *loop = NULL;

static void remove_mngr (UmmsObjectManager *mngr);

static gboolean
request_name (void)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  guint32 request_status;
  GError *error = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
          DBUS_SERVICE_DBUS,
          DBUS_PATH_DBUS,
          DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (proxy,
       UMMS_SERVICE_NAME,
       DBUS_NAME_FLAG_DO_NOT_QUEUE, &request_status,
       &error)) {
    g_printerr ("Failed to request name: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }

  UMMS_DEBUG ("request_status = %d",  request_status);
  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

//#define FAKE_UMMS_SIGNAL
#ifdef FAKE_UMMS_SIGNAL
#include <glib/giochannel.h>
enum {
  SIGNAL_MEDIA_PLAYER_IFACE_Initialized,
  SIGNAL_MEDIA_PLAYER_IFACE_EOF,
  SIGNAL_MEDIA_PLAYER_IFACE_Error,
  SIGNAL_MEDIA_PLAYER_IFACE_Seeked,
  SIGNAL_MEDIA_PLAYER_IFACE_Stopped,
  SIGNAL_MEDIA_PLAYER_IFACE_RequestWindow,
  SIGNAL_MEDIA_PLAYER_IFACE_Buffering,
  SIGNAL_MEDIA_PLAYER_IFACE_Buffered,
  SIGNAL_MEDIA_PLAYER_IFACE_PlayerStateChanged,
  N_MEDIA_PLAYER_IFACE_SIGNALS
};


char *sig_name[N_MEDIA_PLAYER_IFACE_SIGNALS] = {
  "SIGNAL_MEDIA_PLAYER_IFACE_Initialized",
  "SIGNAL_MEDIA_PLAYER_IFACE_EOF",
  "SIGNAL_MEDIA_PLAYER_IFACE_Error",
  "SIGNAL_MEDIA_PLAYER_IFACE_Seeked",
  "SIGNAL_MEDIA_PLAYER_IFACE_Stopped",
  "SIGNAL_MEDIA_PLAYER_IFACE_RequestWindow",
  "SIGNAL_MEDIA_PLAYER_IFACE_Buffering",
  "SIGNAL_MEDIA_PLAYER_IFACE_Buffered",
  "SIGNAL_MEDIA_PLAYER_IFACE_PlayerStateChanged"
};




static void shell_help ()
{
  int i;
  for (i = 0; i < N_MEDIA_PLAYER_IFACE_SIGNALS; i++) {
    printf ("'%d': to trige '%s'\n", i, sig_name[i]);
  }
  printf ("'?': to display this help info\n");
  printf ("'h': to display this help info\n");
  printf ("'q': to quit\n");
}

#if 0
static void emit_signal (MeegoMediaPlayer *obj, int sig_id)
{
  switch (sig_id) {
    case SIGNAL_MEDIA_PLAYER_IFACE_Initialized:
      umms_media_player_iface_emit_initialized (obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_EOF:
      umms_media_player_iface_emit_eof (obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_Error:
      umms_media_player_iface_emit_error (obj, 1, "Hello Error");
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_Seeked:
      umms_media_player_iface_emit_seeked (obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_Stopped:
      umms_media_player_iface_emit_stopped (obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_RequestWindow:
      umms_media_player_iface_emit_request_window (obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_Buffering:
      umms_media_player_iface_emit_buffering(obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_Buffered:
      umms_media_player_iface_emit_buffered(obj);
      break;
    case SIGNAL_MEDIA_PLAYER_IFACE_PlayerStateChanged:
      umms_media_player_iface_emit_player_state_changed(obj, PlayerStateNull);
      break;

    default:
      printf ("Unknown sig_id:%d\n", sig_id);
      break;
  }

}
#endif
static gboolean channel_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
  int rc;
  char buf[64];
  UmmsObjectManager *mngr = (UmmsObjectManager *)data;

  if (condition != G_IO_IN) {
    return TRUE;
  }

  /* we've received something on stdin.    */
  rc = fscanf(stdin, "%s", buf);
  if (rc <= 0) {
    printf("NULL\n");
    return TRUE;
  }

  if (!strcmp(buf, "h")) {
    shell_help();
  } else if (!strcmp(buf, "?")) {
    shell_help();
  } else if (!strcmp(buf, "q")) {
    printf("quit umms service\n");
    remove_mngr (mngr);
    g_main_loop_quit(loop);
  } else if ('0' <= buf[0] && buf[0] <= (N_MEDIA_PLAYER_IFACE_SIGNALS + 48)) {
    //emit_signal(obj, buf[0] - 48);
  } else {
    printf("Unknown command `%s'\n", buf);
  }
  return TRUE;
}
#endif


int
main (int    argc,
      char **argv)
{
  DBusGConnection *connection;
  GError *error = NULL;
#ifdef FAKE_UMMS_SIGNAL
  GIOChannel *chan;
#endif

  g_thread_init (NULL);
  gst_init (&argc, &argv);


  if (!request_name ()) {
    UMMS_DEBUG("UMMS service already running");
    exit (1);
  }

  UmmsObjectManager *umms_object_manager = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_OBJECT_MANAGER, &dbus_glib_umms_object_manager_object_info);
  umms_object_manager = umms_object_manager_new ();

  UmmsPlayingContentMetadataViewer *metadata_viewer = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, &dbus_glib_umms_playing_content_metadata_viewer_object_info);
  metadata_viewer = umms_playing_content_metadata_viewer_new (umms_object_manager);

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL) {
    g_printerr ("yyFailed to open connection to DBus: %s\n", error->message);
    g_error_free (error);

    exit (1);
  }

  dbus_g_connection_register_g_object (connection, UMMS_OBJECT_MANAGER_OBJECT_PATH, G_OBJECT (umms_object_manager));
  dbus_g_connection_register_g_object (connection, UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH, G_OBJECT (metadata_viewer));

  loop = g_main_loop_new (NULL, TRUE);
#ifdef FAKE_UMMS_SIGNAL
  chan = g_io_channel_unix_new(0);
  g_io_add_watch(chan, G_IO_IN, channel_cb, umms_object_manager);
#endif
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  printf("exit successful\n");
  return EXIT_SUCCESS;
}

static gboolean
unregister_object (gpointer obj)
{
  DBusGConnection *connection;
  GError *err = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", err->message);
    g_error_free (err);
    return FALSE;
  }
  dbus_g_connection_unregister_g_object (connection,
      G_OBJECT (obj));
  return TRUE;
}

static void
remove_mngr (UmmsObjectManager *mngr)
{
  printf("remove object manager\n");
  unregister_object (mngr);
  g_object_unref (mngr);

  return;
}
