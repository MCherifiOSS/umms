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

#include "umms-debug.h"
#include "umms-common.h"
#include "umms-resource-manager.h"

G_DEFINE_TYPE (UmmsResourceManager, umms_resource_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerPrivate))

#define GET_PRIVATE(o) ((UmmsResourceManager *)o)->priv

//UPP_A and UPP_B
#define MAX_PLANE 2
#define MAX_HW_VIDDEC 2
#define MAX_HW_CLOCK 5
#define MAX_HW_TUNER 1

typedef struct _ResourceItem {
  gint     id;
  gboolean used;
} ResourceItem;

static ResourceItem planes[MAX_PLANE] = {{UPP_A, FALSE}, {UPP_B, FALSE}};
static ResourceItem hw_viddec[MAX_HW_VIDDEC] = {{1, FALSE}, {2, FALSE}};
static ResourceItem hw_clock[MAX_HW_CLOCK] = {{1, FALSE}, {2, FALSE}, {3, FALSE}, {4, FALSE}, {5, FALSE}};
static ResourceItem hw_tuner[MAX_HW_TUNER] = {{1, FALSE}};

//The sequence must follow the enum _ResourceType definition.
guint limit[ResourceTypeNum] = {MAX_PLANE, MAX_HW_VIDDEC, MAX_HW_CLOCK, MAX_HW_TUNER};
ResourceItem *res_collection[ResourceTypeNum] = {planes, hw_viddec, hw_clock, hw_tuner};

struct _UmmsResourceManagerPrivate {
  GMutex *lock;
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

static void
umms_resource_manager_init (UmmsResourceManager *self)
{
  UmmsResourceManagerPrivate *priv;

  self->priv = MANAGER_PRIVATE (self);
  priv = self->priv;

  priv->lock = g_mutex_new ();
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
  ResourceItem *res_array = NULL;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (req, NULL);

  priv = GET_PRIVATE (self);
  g_mutex_lock (priv->lock);

  res_num = limit[req->type];
  res_array = res_collection[req->type];

  //Respect the preference given by client.
  if (req->preference != INVALID_RES_HANDLE) {
    for (i = 0; i < res_num; i++) {
      if ((res_array[i].id == req->preference) && (res_array[i].used == FALSE)) {
        res_array[i].used = TRUE;
        goto found;
      }
    }
    UMMS_DEBUG ("Preference (%d) is not available", req->preference);
  }

  //Find the first available item.
  for (i = 0; i < res_num; i++) {
    if (!res_array[i].used) {
      res_array[i].used = TRUE;
      goto found;
    }
  }

  UMMS_DEBUG ("No available resource");
  goto func_exit;

found:
  res = (Resource *)g_malloc (sizeof(Resource));
  if (!res) {
    UMMS_DEBUG ("malloc failed");
    goto func_exit;
  }
  res->type = req->type;
  res->handle = res_array[i].id;
  UMMS_DEBUG ("resource available, type:%d, handle:%d", res->type, res->handle);

func_exit:
  g_mutex_unlock (priv->lock);
  return res;
}

void
umms_resource_manager_release_resource (UmmsResourceManager *self, Resource *res)
{
  gint i;
  guint res_num;
  ResourceItem *res_array = NULL;
  UmmsResourceManagerPrivate *priv;

  g_return_if_fail (self && res);

  priv = GET_PRIVATE (self);
  g_mutex_lock (priv->lock);
  res_num = limit[res->type];
  res_array = res_collection[res->type];

  for (i = 0; i < res_num; i++) {
    if (res_array[i].id == res->handle) {
      res_array[i].used = FALSE;
      UMMS_DEBUG ("resouce released, type:%d, handle:%d", res->type, res_array[i].id);
      g_free (res);
      goto func_exit;
    }
  }

  UMMS_DEBUG ("Invalid resource handle '%d'", res->handle);

func_exit:
  g_mutex_unlock (priv->lock);
  return;
}
