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
UmmsObjectManager *umms_object_manager_new (gint platform);
gboolean umms_object_manager_request_media_player(UmmsObjectManager *self, gchar **object_path, GError **error);
gboolean umms_object_manager_request_media_player_unattended(UmmsObjectManager *self, gdouble time_to_execution, 
        gchar **token, gchar **object_path, GError **error);
gboolean umms_object_manager_remove_media_player(UmmsObjectManager *self, gchar *object_path, GError **error);
GList *umms_object_manager_get_player_list (UmmsObjectManager *self);

G_END_DECLS

#endif /* _UMMS_OBJECT_MANAGER_H */

