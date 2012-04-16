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

#include "umms-server.h"
#include "umms-debug.h"
#include "umms-player-backend.h"
#include "umms-audio-manager-backend.h"
#include "umms-video-output-backend.h"
#include "umms-plugin.h"
#include "umms-utils.h"

typedef enum _HintType {
  HintTypeFileName,
  HintTypeProtocol,
  HintTypePluginType,
  HintTypeNum
} HintType;

gchar *
get_player_plugin_filename_by_configure (GKeyFile *umms_conf, const gchar *prot)
{
  gsize len;
  gint i;
  GError *error = NULL;
  gchar *path = NULL;
  gchar *filename = NULL;
  gchar **keys = NULL;
  gchar *v = NULL;

  g_return_val_if_fail (umms_conf, NULL);
  g_return_val_if_fail (prot, NULL);
  g_return_val_if_fail (prot[0] != '\0', NULL);

  /* Parse player plugins preference group */
  keys = g_key_file_get_keys (umms_conf, PLAYER_PLUGIN_GROUP, &len, &error);
  if (!keys) {
    g_warning ("group:%s not found", PLAYER_PLUGIN_GROUP);
    if (error != NULL) {
      g_warning ("error: %s", error->message);
      g_error_free (error);
    } else {
      g_warning ("error: unknown error");
    }
    return NULL;
  }

  for (i=0; i<len; i++) {
    if (g_strrstr (prot, keys[i]) || g_strrstr ("all", keys[i])) {
      if (filename = g_key_file_get_string (umms_conf, PLAYER_PLUGIN_GROUP, keys[i], NULL)) {
        break;
      }
    }
  }

  if (keys)
    g_strfreev (keys);

  return filename;
}

static UmmsPlayerBackend *
make_backend_from_plugin (UmmsPlugin *plugin)
{
  gpointer backend = NULL;

  if (!plugin->backend_new_func) {
    UMMS_WARNING ("backend_new_func is NULL");
    return NULL;
  }

  backend = plugin->backend_new_func ();
  if (backend && UMMS_IS_PLAYER_BACKEND (backend))
    umms_player_backend_set_plugin (UMMS_PLAYER_BACKEND (backend), plugin);

  return backend;
}

static UmmsPlugin *
query_plugin (GList *plugins, gpointer hint, HintType hint_type)
{
  UmmsPlugin *plugin = NULL;
  gboolean done = FALSE;
  const gchar *plugin_type_name = NULL;

  g_return_val_if_fail (plugins, NULL);
  g_return_val_if_fail (hint, NULL);

  while (plugins && !done) {
    plugin = (UmmsPlugin *)plugins->data;

    if (!plugin) {
      plugins = plugins->next;
      continue;
    }

    switch (hint_type) {
    case HintTypeFileName:
      if (!g_strcmp0 (plugin->filename, (const gchar *)hint))
        done = TRUE;
      break;
    case HintTypeProtocol:
      if (umms_plugin_support_protocol (plugin, (const gchar *)hint))
        done = TRUE;
      break;
    case HintTypePluginType:
      if (plugin->type == (UmmsPluginType)hint)
        done = TRUE;
      break;
    default:
      UMMS_WARNING ("Unknow hinttype %d", hint_type);
      break;
    }

    plugins = plugins->next;
  }

  if (done)
    return plugin;
  else
    return NULL;
}

gpointer umms_backend_factory_make (UmmsPluginType type)
{
  UmmsPlugin *plugin = NULL;
  gpointer backend = NULL;

  plugin = query_plugin (umms_ctx->plugins, (gpointer)type, HintTypePluginType);

  if (plugin)
    backend = make_backend_from_plugin (plugin);

  if (backend) {
    UMMS_DEBUG ("created audio manager backend (%p) from plugin (%p)", backend, plugin);
    umms_plugin_info (plugin);
  }
  return backend;
}

UmmsPlayerBackend *
umms_player_backend_make_from_uri (const gchar *uri)
{
  gchar *prot = NULL;
  gchar *base = NULL;
  gchar *filename = NULL;
  UmmsPlugin *plugin = NULL;
  UmmsPlayerBackend *backend = NULL;

  g_return_val_if_fail (uri_is_valid (uri), NULL);

  prot = uri_get_protocol (uri);
  if (!prot) {
    UMMS_WARNING ("failed to get protocol for uri \"%s\"", uri);
    return NULL;
  }

  /* Firstly, check configure for plugins preference */
  if (filename = get_player_plugin_filename_by_configure (umms_ctx->conf, prot)) {
    plugin = query_plugin (umms_ctx->plugins, filename, HintTypeFileName);
  }

  if (!plugin)
    plugin = query_plugin (umms_ctx->plugins, prot, HintTypeProtocol);

  if (plugin)
    backend = make_backend_from_plugin (plugin);

  if (prot)
    g_free (prot);
  if (filename)
    g_free (filename);

  if (backend) {
    UMMS_DEBUG ("created backend (%p) from plugin (%p)", backend, plugin);
    umms_plugin_info (plugin);
  }
  return backend;
}
