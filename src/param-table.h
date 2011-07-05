#ifndef _PARAM_TABLE_H
#define _PARAM_TABLE_H
#include <glib.h>

void param_table_release (GHashTable *param);
GHashTable *param_table_create (const gchar* key1, ...);

#endif /* _PARAM_TABLE_H */
