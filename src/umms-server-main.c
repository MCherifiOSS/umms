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
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "umms-server.h"
#include "umms-types.h"
#include "umms-debug.h"
#include "umms-plugin.h"
#include "umms-object-manager.h"
#include "umms-playing-content-metadata-viewer.h"
#include "umms-audio-manager.h"
#include "umms-video-output.h"
#include "./glue/umms-object-manager-glue.h"
#include "./glue/umms-audio-manager-glue.h"
#include "./glue/umms-video-output-glue.h"
#include "./glue/umms-playing-content-metadata-viewer-glue.h"

UmmsCtx *umms_ctx = NULL;
static GMainLoop *loop = NULL;

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

  return request_status == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
}

static GKeyFile *
load_conf (const gchar *path)
{
  GKeyFile *keyfile = NULL;
  GError *error = NULL;

  if (!(keyfile = g_key_file_new ())) {
    g_warning ("failed to create key file");
    return NULL;
  }

  if (!g_key_file_load_from_file (keyfile, path, 0, &error)) {
    g_warning ("failed to load file:\"%s\"", path);
    if (error != NULL) {
      g_warning ("error: %s", error->message);
      g_error_free (error);
    } else {
      g_warning ("error: unknown error");
    }
    return NULL;
  }

  return keyfile;
}


static gboolean
is_valid_module_name (const gchar *name)
{
  g_return_val_if_fail (name, FALSE);

  return g_str_has_prefix (name, "lib") &&
         g_str_has_suffix (name, ".so");
}

static gboolean
load_plugin (UmmsCtx *ctx, const gchar *base, const gchar *name)
{
  GModule *module = NULL;
  UmmsPlugin *plugin = NULL;
  gchar *path = NULL;
  gboolean ret = FALSE;

  g_return_val_if_fail (base, FALSE);
  g_return_val_if_fail (name, FALSE);

  path = g_build_filename (base, name, NULL);
  module = g_module_open (path, G_MODULE_BIND_LAZY);
  if (!module) {
    UMMS_DEBUG ("%s", g_module_error ());
    goto out;
  }

  if (!g_module_symbol (module, "umms_plugin", (gpointer *)&plugin)) {
    UMMS_DEBUG ("%s: %s", path, g_module_error ());
    goto close_module;
  }

  if (plugin == NULL) {
    UMMS_DEBUG ("symbol umms_plugin is NULL");
    goto close_module;
  }

  plugin->filename = name;
  ctx->plugins = g_list_prepend (ctx->plugins, plugin);
  ret = TRUE;

out:
  if (path)
    g_free (path);
  return ret;

close_module:
  if (!g_module_close (module)) {
    UMMS_DEBUG ("%s", g_module_error ());
  }
  goto out;
}

static gboolean
load_plugins (UmmsCtx *ctx)
{
  gchar *base = NULL;
  const gchar *filename = NULL;
  GDir *dir = NULL;
  GError *err = NULL;

  /* FIXME: Should not hardcode plugin dir */
  base = UMMS_PLUGINS_PATH_DEFAULT;
  if (!(dir = g_dir_open (base, 0, &err))) {
    UMMS_WARNING ("can't open dir \"%s\", %s", base, err->message);
    return FALSE;
  }

  while (filename = g_dir_read_name (dir)) {
    if (!is_valid_module_name (filename))
      continue;
    load_plugin (ctx, base, filename);
  }

  return TRUE;
}

static void
get_proxy_from_conf (UmmsCtx *ctx, GKeyFile *conf)
{
  gchar **keys = NULL;
  gchar **value_holder = NULL;
  gsize len;
  gint i;
  GError *err = NULL;

  if (!conf)
    return;

  keys = g_key_file_get_keys (conf, PROXY_GROUP, &len, &err);
  if (!keys) {
    g_warning ("group:%s not found", PROXY_GROUP);
    if (err != NULL) {
      g_warning ("error: %s", err->message);
      g_error_free (err);
    } else {
      g_warning ("error: unknown error");
    }
    return;
  }

  for (i = 0; i < len; i++) {
     if (g_strcmp0(keys[i], "uri") == 0)
        value_holder = &ctx->proxy_uri;
     else if (g_strcmp0(keys[i], "user") == 0)
        value_holder = &ctx->proxy_id;
     else if (g_strcmp0(keys[i], "password") == 0)
        value_holder = &ctx->proxy_pw;
     else {
        value_holder = NULL;
        UMMS_WARNING ("Invalid key %s", keys[i]);
     }

     if (value_holder)
        *value_holder = g_key_file_get_string (conf, PROXY_GROUP, keys[i], NULL);
  }

  g_strfreev (keys);
  return;
}

int
main (int    argc,
      char **argv)
{
  DBusGConnection *connection;
  const gchar *conf_path;
  const gchar *resource_conf_path;
  GError *error = NULL;

  g_type_init ();
  g_thread_init (NULL);

  umms_ctx = g_malloc0 (sizeof (UmmsCtx));

  /* load conf */
  //FIXME: not hardcode path
  conf_path = "/etc/umms.conf";
  resource_conf_path = "/etc/umms-resource.conf";
  umms_ctx->conf = load_conf (conf_path);
  umms_ctx->resource_conf = load_conf (resource_conf_path);

  /* http proxy */
  umms_ctx->proxy_uri = g_strdup (g_getenv ("http_proxy"));
  if (!umms_ctx->proxy_uri)
    get_proxy_from_conf (umms_ctx, umms_ctx->conf);

  /* plugins */
  load_plugins (umms_ctx);

  if (!request_name ()) {
    UMMS_DEBUG("UMMS service already running");
    exit (1);
  }

  /* Global object */
  UmmsObjectManager *umms_object_manager = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_OBJECT_MANAGER, &dbus_glib_umms_object_manager_object_info);
  umms_object_manager = umms_object_manager_new ();

  UmmsPlayingContentMetadataViewer *metadata_viewer = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, &dbus_glib_umms_playing_content_metadata_viewer_object_info);
  metadata_viewer = umms_playing_content_metadata_viewer_new (umms_object_manager);

  UmmsAudioManager *audio_manager = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_AUDIO_MANAGER, &dbus_glib_umms_audio_manager_object_info);
  audio_manager = umms_audio_manager_new();


  UmmsVideoOutput *video_output = NULL;
  dbus_g_object_type_install_info (UMMS_TYPE_VIDEO_OUTPUT, &dbus_glib_umms_video_output_object_info);
  video_output = umms_video_output_new();

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", error->message);
    g_error_free (error);
    exit (1);
  }

  dbus_g_connection_register_g_object (connection, UMMS_OBJECT_MANAGER_OBJECT_PATH, G_OBJECT (umms_object_manager));
  dbus_g_connection_register_g_object (connection, UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH, G_OBJECT (metadata_viewer));
  dbus_g_connection_register_g_object (connection, UMMS_AUDIO_MANAGER_OBJECT_PATH, G_OBJECT (audio_manager));
  dbus_g_connection_register_g_object (connection, UMMS_VIDEO_OUTPUT_OBJECT_PATH, G_OBJECT (video_output));

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_print ("exit successful\n");
  return EXIT_SUCCESS;
}
