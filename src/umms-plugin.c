#include <glib.h>
#include "umms-plugin.h"
#include "umms-debug.h"

/* Keep order with enum UmmsPluginType */
static const gchar *UmmsPluginTypeName[] = {
  "UMMS_PLUGIN_TYPE_PLAYER_BACKEND",
  "UMMS_PLUGIN_TYPE_VIDEO_OUTPUT_BACKEND",
  "UMMS_PLUGIN_TYPE_AUDIO_MANAGER_BACKEND",
  "UMMS_PLUGIN_TYPE_UNKNOWN"
};

static const gchar *
type_name (UmmsPluginType type)
{
  if (type < UMMS_PLUGIN_TYPE_PLAYER_BACKEND || type >= UMMS_PLUGIN_TYPE_NUM)
    type = UMMS_PLUGIN_TYPE_NUM;
  return UmmsPluginTypeName[type];
}

const gchar *
umms_plugin_get_type_name (UmmsPlugin *plugin)
{
  return type_name (plugin->type);
}

gboolean
umms_plugin_support_protocol (UmmsPlugin *plugin, const gchar *prot)
{
  gint i;
  gboolean supported;

  g_return_val_if_fail (plugin, FALSE);

  if (plugin->type != UMMS_PLUGIN_TYPE_PLAYER_BACKEND)
    return FALSE;

  g_return_val_if_fail (plugin->supported_uri_protocols != NULL, FALSE);

  if (!plugin->supported_uri_protocols[0]) {
    i = 0;
    supported = TRUE;
    //check the black list
    g_return_val_if_fail (plugin->unsupported_uri_protocols != NULL, FALSE);

    while (plugin->unsupported_uri_protocols[i]) {
      if (!g_strcmp0(prot, plugin->unsupported_uri_protocols[i++])) {
        supported = FALSE;
        break;
      }
    }
  } else if (!g_strcmp0 (plugin->supported_uri_protocols[0], "all")) {
    return TRUE;
  } else {
    i = 0;
    supported = FALSE;
    while (plugin->supported_uri_protocols[i]) {
      if (!g_strcmp0(prot, plugin->supported_uri_protocols[i++])) {
        supported = TRUE;
        break;
      }
    }
  }

  return supported;
}

void
umms_plugin_info (UmmsPlugin *plugin)
{
  gint i = 0;

  g_print ("===== plugins info =====\n");
  g_print ("Name:        %s\n", plugin->name);
  g_print ("Description: %s\n", plugin->description);
  g_print ("Version:     %d.%d\n", plugin->major_version, plugin->minor_version);
  g_print ("Plugin Type: %s\n", type_name(plugin->type));

  if (plugin->type == UMMS_PLUGIN_TYPE_PLAYER_BACKEND) {
    g_return_if_fail (plugin->supported_uri_protocols);
    g_print ("Supported protcols:\n");
    while (plugin->supported_uri_protocols[i]) {
      g_print ("  %s\n", plugin->supported_uri_protocols[i++]);
    }

    if (plugin->unsupported_uri_protocols) {
      i = 0;
      g_print ("Unsupported protcols:\n");
      while (plugin->unsupported_uri_protocols[i]) {
        g_print ("  %s\n", plugin->unsupported_uri_protocols[i++]);
      }
    }
  }
  g_print ("========================\n");
}

gpointer
umms_plugin_create_backend (UmmsPlugin *plugin)
{
  gpointer backend = NULL;

  g_return_val_if_fail (plugin, NULL);

  if (plugin->backend_new_func) {
    backend = plugin->backend_new_func ();
  } else {
    UMMS_WARNING ("backend_new_func is NULL");
  }

  return backend;
}
