#ifndef _UMMS_OBJECT_MANAGER_H
#define _UMMS_OBJECT_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_OBJECT_MANAGER umms_object_manager_get_type()

#define UMMS_OBJECT_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_OBJECT_MANAGER, UmmsObjectManager))

#define UMMS_OBJECT_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_OBJECT_MANAGER, UmmsObjectManagerClass))

#define UMMS_IS_OBJECT_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_OBJECT_MANAGER))

#define UMMS_IS_OBJECT_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_OBJECT_MANAGER))

#define UMMS_OBJECT_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_OBJECT_MANAGER, UmmsObjectManagerClass))

typedef struct _UmmsObjectManager UmmsObjectManager;
typedef struct _UmmsObjectManagerClass UmmsObjectManagerClass;
typedef struct _UmmsObjectManagerPrivate UmmsObjectManagerPrivate;

struct _UmmsObjectManager
{
  GObject parent;
  UmmsObjectManagerPrivate *priv;
};


struct _UmmsObjectManagerClass
{
  GObjectClass parent_class;
};


GType umms_object_manager_get_type (void) G_GNUC_CONST;
UmmsObjectManager *umms_object_manager_new (void);
gboolean umms_object_manager_request_media_player(UmmsObjectManager *self, gchar **object_path, GError **error);
gboolean umms_object_manager_request_media_player_unattended(UmmsObjectManager *self, gdouble time_to_execution, 
        gchar **token, gchar **object_path, GError **error);
gboolean umms_object_manager_remove_media_player(UmmsObjectManager *self, gchar *object_path, GError **error);

G_END_DECLS

#endif /* _UMMS_OBJECT_MANAGER_H */

