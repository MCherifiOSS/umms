#include <glib.h>
#include <string.h>
#include <stdarg.h>
#include <gobject/gvaluecollector.h>
#include "umms-utils.h"


static const gchar *mesg[MSG_NUM] = {
  "Method not implemented",
  "Backend not loaded",
};

const gchar *
get_mesg_str (Mesg msg_id)
{
  return mesg[msg_id];
}

static void
uri_protocol_check_internal (const gchar * uri, gchar ** endptr)
{
  gchar *check = (gchar *) uri;

  g_return_if_fail (uri != NULL);
  g_return_if_fail (endptr != NULL);

  if (g_ascii_isalpha (*check)) {
    check++;
    while (g_ascii_isalnum (*check) || *check == '+'
           || *check == '-' || *check == '.')
      check++;
  }

  *endptr = check;
}

gboolean
uri_is_valid (const gchar * uri)
{
  gchar *endptr;

  g_return_val_if_fail (uri != NULL, FALSE);

  uri_protocol_check_internal (uri, &endptr);

  return *endptr == ':';
}


gchar *
uri_get_protocol (const gchar * uri)
{
  gchar *colon;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (uri_is_valid (uri), NULL);

  colon = strstr (uri, ":");

  return g_ascii_strdown (uri, colon - uri);
}

static void
val_release (gpointer data)
{
  GValue *val = (GValue *)data;
  g_value_unset(val);
  g_free (val);
}

//call g_hash_table_unref after usage.
GHashTable *
param_table_create (const gchar* key1, ...)
{
  GHashTable  *params;
  const gchar *key;
  GValue      *val;
  GType       valtype;
  const char  *collect_err;
  va_list     args;

  params = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, val_release);

  va_start (args, key1);

  key = key1;
  while (key != NULL) {
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
