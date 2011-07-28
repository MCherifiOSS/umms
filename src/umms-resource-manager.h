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
  ResourceTypeLast
};

GType umms_resource_manager_get_type (void) G_GNUC_CONST;
UmmsResourceManager *umms_resource_manager_new (void);
gboolean umms_resource_manager_request_resource (UmmsResourceManager *self, ResourceType type);
gboolean umms_resource_manager_release_resource (UmmsResourceManager *self, ResourceType type);

G_END_DECLS

#endif /* _UMMS_RESOURCE_MANAGER_H */

