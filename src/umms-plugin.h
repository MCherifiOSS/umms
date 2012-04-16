#ifndef __UMMS_PLUGIN_H__
#define __UMMS_PLUGIN_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum _UmmsPluginType UmmsPluginType;
typedef struct _UmmsPlugin UmmsPlugin;

enum _UmmsPluginType {
  UMMS_PLUGIN_TYPE_PLAYER_BACKEND,
  UMMS_PLUGIN_TYPE_VIDEO_OUTPUT_BACKEND,
  UMMS_PLUGIN_TYPE_AUDIO_MANAGER_BACKEND,
  UMMS_PLUGIN_TYPE_NUM
};

struct _UmmsPlugin {
  gint major_version;//the major version number of umms core that plugin was compiled for
  gint minor_version;//the minor version number of core that plugin was compiled for
  UmmsPluginType type;
  const gchar *filename;//set by "plugin loader" when this plugin loaded.
  const gchar *name;
  const gchar *description;

  /*
   * Used for plugins of UMMS_PLUGIN_TYPE_PLAYER_BACKEND type.
   * If supported_uri_protocols is empty, unsupported_uri_protocols is respected
   * as a "black list" of uri. If supported_uri_protocols is not empty, ignore
   * unsupported_uri_protocols.
   * e.g. The implementor can create a black list for its plugin like this:
   * supported_uri_protocols = {NULL};
   * unsupported_uri_protocols = {"dvb"};
   * that means the plugin can handle all uri protocols unless "dvb".
   */
  const gchar **supported_uri_protocols;
  const gchar **unsupported_uri_protocols;

  /*vmethod to create the instance of backend.*/
  gpointer (*backend_new_func) (void);
};

gboolean umms_plugin_support_protocol (UmmsPlugin *plugin, const gchar *prot);
void umms_plugin_info (UmmsPlugin *plugin);
gpointer umms_plugin_create_backend (UmmsPlugin *plugin);
const gchar *umms_plugin_get_type_name (UmmsPlugin *plugin);

G_END_DECLS
#endif
