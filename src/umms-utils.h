#ifndef __UMMS_UTILS_H__
#define __UMMS_UTILS_H__

#include <glib.h>
#include "umms-error.h"
#include "umms-debug.h"
#include "umms-types.h"

G_BEGIN_DECLS
#define RESET_STR(str) \
     if (str) {       \
       g_free(str);   \
       str = NULL;    \
     }

#define TYPE_VMETHOD_CALL(type, func, ...) \
  UMMS_DEBUG ("calling");                                                  \
  if (!self) {\
    g_set_error (err, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_NOT_LOADED, get_mesg_str (MSG_BACKEND_NOT_LOADED));\
    return FALSE;\
  }\
  if (UMMS_##type##_GET_CLASS (self)->func) {                        \
    return UMMS_##type##_GET_CLASS (self)->func (self, ##__VA_ARGS__); \
  } else {                                                                 \
    g_warning ("%s: %s\n", __FUNCTION__, get_mesg_str (MSG_NOT_IMPLEMENTED));               \
    g_set_error (err, UMMS_BACKEND_ERROR, UMMS_BACKEND_ERROR_METHOD_NOT_IMPLEMENTED, get_mesg_str (MSG_NOT_IMPLEMENTED));\
    return FALSE;                                                          \
  }

const gchar *get_mesg_str (Mesg mesg_id);
gboolean uri_is_valid (const gchar * uri);
gchar *uri_get_protocol (const gchar * uri);
GHashTable *param_table_create (const gchar* key1, ...);

G_END_DECLS
#endif
