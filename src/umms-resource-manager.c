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
#include "umms-types.h"
#include "umms-resource-manager.h"

G_DEFINE_TYPE (UmmsResourceManager, umms_resource_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerPrivate))

#define GET_PRIVATE(o) ((UmmsResourceManager *)o)->priv

struct _UmmsResourceManagerPrivate {
  GMutex *lock;
  guint type_num;
  Resource **res_collection;
  guint *limit;
};

static void
umms_resource_manager_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_resource_manager_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_resource_manager_dispose (GObject *object)
{
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (object);

  g_mutex_free(priv->lock);

  G_OBJECT_CLASS (umms_resource_manager_parent_class)->dispose (object);
}

static void
umms_resource_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_resource_manager_parent_class)->finalize (object);
}

static void
umms_resource_manager_class_init (UmmsResourceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsResourceManagerPrivate));

  object_class->get_property = umms_resource_manager_get_property;
  object_class->set_property = umms_resource_manager_set_property;
  object_class->dispose = umms_resource_manager_dispose;
  object_class->finalize = umms_resource_manager_finalize;

}

static gboolean
parse_resource_def (UmmsResourceManager *self, const gchar *desc, gint index)
{
  gboolean ret;
  gboolean id_done = FALSE;
  gchar **strv = NULL;
  gchar **ids = NULL;
  gint i;
  gint res_num;
  UmmsResourceManagerPrivate *priv = self->priv;

  g_return_val_if_fail (desc, FALSE);

  strv = g_strsplit (desc, ":", 0);
  if (!strv) {
    UMMS_WARNING ("failed to load resource (%d) definition", index);
    return FALSE;
  }

  priv->limit[index] = atoi (strv[0]);
  if (!(priv->res_collection[index] = g_malloc0 (priv->limit[index] * sizeof (Resource)))) {
    ret = FALSE;
    goto out;
  }


  if (strv[1]) {
    ids = g_strsplit (strv[1], ",", 0);
    if (ids && g_strv_length(ids) == priv->limit[index]) {
      for (i=0; i<priv->limit[index]; i++) {
        priv->res_collection[index][i].id = atoi (ids[i]);
        priv->res_collection[index][i].type = index;
      }
      id_done = TRUE;
      ret = TRUE;
    }
  }

  if (!id_done) {
    //no resource id specified, let's assign them from 0 to limit-1
    for (i=0; i<priv->limit[index]; i++) {
      priv->res_collection[index][i].id = i;
      priv->res_collection[index][i].type = index;
    }
    ret = TRUE;
  }

out:
  if (strv)
    g_strfreev (strv);
  if (ids)
    g_strfreev (ids);
  return ret;
}

static void
print_resource (UmmsResourceManager *self)
{
  UmmsResourceManagerPrivate *priv = self->priv;
  gint i, j;

  UMMS_DEBUG ("we have %u resource types", priv->type_num);
  if (priv->res_collection) {
    for (i=0; i<priv->type_num; i++) {
      UMMS_DEBUG ("type %u: limit (%d)", i, priv->limit[i]);
      for (j=0; j<priv->limit[i]; j++) {
        g_print ("\tid=%d used=%d,\t", priv->res_collection[i][j].id, priv->res_collection[i][j].used);
      }
      g_print ("\n");
    }
  }
}

static void
umms_resource_manager_init (UmmsResourceManager *self)
{
  UmmsResourceManagerPrivate *priv;
  gboolean ret;
  gsize type_num;
  gint i;
  GError *err = NULL;
  gchar **keys = NULL;
  gchar *resource_desc = NULL;
  gchar *strv = NULL;

  self->priv = MANAGER_PRIVATE (self);
  priv = self->priv;
  priv->lock = g_mutex_new ();

  /* get the resource configuration info */
  g_assert (umms_ctx);
  if (!umms_ctx->resource_conf)
    return;

  keys = g_key_file_get_keys (umms_ctx->resource_conf, RESOURCE_GROUP, &type_num, &err);
  if (!keys) {
    g_warning ("group:%s not found", RESOURCE_GROUP);
    if (err != NULL) {
      g_warning ("error: %s", err->message);
      g_error_free (err);
    } else {
      g_warning ("error: unknown error");
    }
    return;
  }

  /*
   * type = priv->limit:resource_id
   * 0 = 3:1,2,3
   * 1 = 5
   */
  priv->res_collection = g_malloc0 (type_num * sizeof (Resource*));
  priv->limit = g_malloc0 (type_num * sizeof (guint));
  if (!priv->res_collection || !priv->limit) {
    UMMS_WARNING ("Failed to allocate memory");
    goto out;
  }

  for (i=0; i<type_num; i++) {
    if (resource_desc = g_key_file_get_string (umms_ctx->resource_conf, RESOURCE_GROUP, keys[i], NULL)) {
      ret = parse_resource_def (self, resource_desc, i);
      g_free (resource_desc);
      if (!ret)
        goto failed;
    }
  }
  priv->type_num = type_num;

  print_resource (self);
out:
  if (keys)
    g_strfreev (keys);
  return;

failed:
  if (priv->res_collection) {
    for (i=0; priv->res_collection[i] != NULL; i++)
      g_free (priv->res_collection[i]);
    g_free (priv->res_collection);
    priv->res_collection = NULL;
  }

  if (priv->limit) {
    g_free (priv->limit);
    priv->limit = NULL;
  }

  goto out;
}

static UmmsResourceManager *mngr_global = NULL;

UmmsResourceManager *
umms_resource_manager_new (void)
{
  if (!mngr_global) {
    mngr_global = g_object_new (UMMS_TYPE_RESOURCE_MANAGER, NULL);
  }
  return mngr_global;
}

Resource *
umms_resource_manager_request_resource (UmmsResourceManager *self, ResourceRequest *req)
{
  UmmsResourceManagerPrivate *priv;
  gint i;
  guint res_num;
  Resource *res = NULL;
  Resource *res_array = NULL;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (req, NULL);

  priv = GET_PRIVATE (self);

  if (!priv->res_collection) {
    UMMS_WARNING ("no resource defined");
    return NULL;
  }

  g_mutex_lock (priv->lock);

  res_num = priv->limit[req->type];
  res_array = priv->res_collection[req->type];

  //Respect the preference given by client.
  if (req->preference != NO_PREFERENCE) {
    for (i = 0; i < res_num; i++) {
      if ((res_array[i].id == req->preference) && (res_array[i].used == FALSE)) {
        res_array[i].used = TRUE;
        res = &res_array[i];
        goto out;
      }
    }
    UMMS_DEBUG ("Preference (%d) is not available", req->preference);
  }

  //Find the first available item.
  for (i = 0; i < res_num; i++) {
    if (!res_array[i].used) {
      res_array[i].used = TRUE;
      res = &res_array[i];
      break;
    }
  }

out:
  if (res)
    UMMS_DEBUG ("resource (type:%d, id:%d) available", res->type, res->id);
  else
    UMMS_DEBUG ("resource (type:%d, id:%d) unavailable", req->type, req->preference);

  g_mutex_unlock (priv->lock);
  return res;
}

void
umms_resource_manager_release_resource (UmmsResourceManager *self, Resource *res)
{
  gint i;
  guint res_num;
  Resource *res_array = NULL;
  UmmsResourceManagerPrivate *priv;

  g_return_if_fail (self && res);

  priv = GET_PRIVATE (self);
  g_mutex_lock (priv->lock);
  res->used = FALSE;
  UMMS_DEBUG ("resouce (type:%d, id:%d) released", res->type, res->id);
  g_mutex_unlock (priv->lock);
  return;
}
