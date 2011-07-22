#include <string.h>
#include <stdarg.h>
#include <gobject/gvaluecollector.h>
#include <glib.h>

static void
val_release (gpointer data)
{
    GValue *val = (GValue *)data;
    g_value_unset(val);
    g_free (val);
}

void param_table_release (GHashTable *param)
{
  g_return_if_fail (param);
  g_hash_table_destroy (param);
}
  
GHashTable * 
param_table_create (const gchar* key1, ...)
{
  GPtrArray   *key_array;
  GValueArray *val_array;
  GHashTable  *params;
  const gchar *key;
  GValue      *val;
  GType       valtype;
  const char  *collect_err; 
  va_list     args;
  gint        i;

  params = g_hash_table_new_full (g_str_hash, NULL, NULL, val_release);

  va_start (args, key1);

  key = key1;
  while (key != NULL) 
  { 
    valtype = va_arg (args, GType); 
    val = g_new0(GValue, 1);
    g_value_init (val, valtype); 
    collect_err = NULL; 
    G_VALUE_COLLECT (val, args, G_VALUE_NOCOPY_CONTENTS, &collect_err); 
    g_hash_table_insert(params, (gpointer)key, val);
    key = va_arg (args, gpointer); 
  } 

  va_end (args);

  return params;
}
