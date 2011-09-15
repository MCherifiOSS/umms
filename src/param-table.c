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
  GHashTable  *params;
  const gchar *key;
  GValue      *val;
  GType       valtype;
  const char  *collect_err;
  va_list     args;

  params = g_hash_table_new_full (g_str_hash, NULL, NULL, val_release);

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
