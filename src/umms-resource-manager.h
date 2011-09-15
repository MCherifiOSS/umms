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

#ifndef _UMMS_RESOURCE_MANAGER_H
#define _UMMS_RESOURCE_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_RESOURCE_MANAGER umms_resource_manager_get_type()

#define UMMS_RESOURCE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManager))

#define UMMS_RESOURCE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerClass))

#define UMMS_IS_RESOURCE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_RESOURCE_MANAGER))

#define UMMS_IS_RESOURCE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_RESOURCE_MANAGER))

#define UMMS_RESOURCE_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerClass))

typedef struct _UmmsResourceManager UmmsResourceManager;
typedef struct _UmmsResourceManagerClass UmmsResourceManagerClass;
typedef struct _UmmsResourceManagerPrivate UmmsResourceManagerPrivate;
typedef struct _Resource Resource;
typedef struct _ResourceRequest ResourceRequest;
typedef enum    _ResourceType ResourceType;
struct _UmmsResourceManager
{
  GObject parent;
  UmmsResourceManagerPrivate *priv;
};


struct _UmmsResourceManagerClass
{
  GObjectClass parent_class;
};

enum _ResourceType
{
  ResourceTypePlane,
  ResourceTypeHwViddec,
  ResourceTypeHwClock,
  ResourceTypeTuner,
  ResourceTypeNum
};

#define INVALID_RES_HANDLE -1
//Actual resource returned by resource manager.
struct _Resource {
  ResourceType type;
  gint         handle;
};

//Resource requested by user.
struct _ResourceRequest {
  ResourceType type;
  gint         preference;//Expected resource by client (e.g. for ResourceTypePlane, it may be UPP_A). 
};


GType umms_resource_manager_get_type (void) G_GNUC_CONST;
UmmsResourceManager *umms_resource_manager_new (void);
Resource *umms_resource_manager_request_resource (UmmsResourceManager *self, ResourceRequest *req);
void umms_resource_manager_release_resource (UmmsResourceManager *self, Resource *res);

G_END_DECLS

#endif /* _UMMS_RESOURCE_MANAGER_H */

