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
#include "umms-audio-manager.h"
#include "umms-video-output.h"
#include "./glue/umms-object-manager-glue.h"
#include "./glue/umms-playing-content-metadata-viewer-glue.h"
#include "./glue/umms-audio-manager-glue.h"
#include "./glue/umms-video-output-glue.h"

static GMainLoop *loop = NULL;

#define PLATFORM_CONFIG "/etc/system-release"

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

const gchar* platform_release[] = {
  "intel-cetv",
  "netbook-ia32",
  "NULL"
};

/*
 * probe system platform information to check
 * if specific gst capability are supported.
 */
gint system_probe(void)
{
  gchar *filename = PLATFORM_CONFIG;
  FILE* fd = 0;
  gchar *contents;
  gsize len;
  GError *error = NULL;
  gint ret = CETV;
  
  /*if the file is exist*/ 
  if(g_file_test(filename, G_FILE_TEST_EXISTS )){

    /*reading data here*/
    g_assert (g_file_test (filename, G_FILE_TEST_EXISTS));
    //g_assert (g_file_test (filename, G_FILE_TEST_IS_REGULAR));

    if (! g_file_get_contents (filename, &contents, &len, &error))
      g_error ("g_file_get_contents() failed: %s", error->message);

    if(strstr(contents, platform_release[CETV])){
      UMMS_DEBUG("is %s", platform_release[CETV]);
      ret = CETV;
    }else if(strstr(contents, platform_release[NETBOOK])){
      UMMS_DEBUG("is %s", platform_release[NETBOOK]);
      ret = NETBOOK;
    }else {
      g_warning ("unknown platform, try generic config\n");
      ret = NETBOOK;
    }

    g_free (contents);
    
  }else{
    /**/
    UMMS_DEBUG("file %s is NOT exist\n", filename); 
    g_warning ("unknown platform, try generic config\n");
    ret = NETBOOK;
  }

  return ret;

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
static void emit_signal (MediaPlayer *obj, int sig_id)
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
  gint platform_type = CETV;

  g_thread_init (NULL);
  gst_init (&argc, &argv);

  if (!request_name ()) {
    UMMS_DEBUG("UMMS service already running");
    exit (1);
  }

  /*system probe*/
  platform_type = system_probe();

  UmmsObjectManager *umms_object_manager = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_OBJECT_MANAGER, &dbus_glib_umms_object_manager_object_info);
  umms_object_manager = umms_object_manager_new (platform_type);

  UmmsPlayingContentMetadataViewer *metadata_viewer = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, &dbus_glib_umms_playing_content_metadata_viewer_object_info);
  metadata_viewer = umms_playing_content_metadata_viewer_new (umms_object_manager);

  UmmsAudioManager *audio_manager = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_AUDIO_MANAGER, &dbus_glib_umms_audio_manager_object_info);
  audio_manager = umms_audio_manager_new(platform_type);

  UmmsVideoOutput *video_output = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_VIDEO_OUTPUT, &dbus_glib_umms_video_output_object_info);
  video_output = umms_video_output_new();

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL) {
    g_printerr ("yyFailed to open connection to DBus: %s\n", error->message);
    g_error_free (error);

    exit (1);
  }

  dbus_g_connection_register_g_object (connection, UMMS_OBJECT_MANAGER_OBJECT_PATH, G_OBJECT (umms_object_manager));
  dbus_g_connection_register_g_object (connection, UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH, G_OBJECT (metadata_viewer));
  dbus_g_connection_register_g_object (connection, UMMS_AUDIO_MANAGER_OBJECT_PATH, G_OBJECT (audio_manager));
  dbus_g_connection_register_g_object (connection, UMMS_VIDEO_OUTPUT_OBJECT_PATH, G_OBJECT (video_output));

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


