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

#ifndef _UMMS_CLIENT_OBJECT_H
#define _UMMS_CLIENT_OBJECT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_CLIENT_OBJECT umms_client_object_get_type()

#define UMMS_CLIENT_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_CLIENT_OBJECT, UmmsClientObject))

#define UMMS_CLIENT_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_CLIENT_OBJECT, UmmsClientObjectClass))

#define UMMS_IS_CLIENT_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_CLIENT_OBJECT))

#define UMMS_IS_CLIENT_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_CLIENT_OBJECT))

#define UMMS_CLIENT_OBJECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_CLIENT_OBJECT, UmmsClientObjectClass))

typedef struct _UmmsClientObject UmmsClientObject;
typedef struct _UmmsClientObjectClass UmmsClientObjectClass;
typedef struct _UmmsClientObjectPrivate UmmsClientObjectPrivate;

struct _UmmsClientObject {
  GObject parent;

  UmmsClientObjectPrivate *priv;
};


struct _UmmsClientObjectClass {
  GObjectClass parent_class;
};

GType umms_client_object_get_type (void) G_GNUC_CONST;

UmmsClientObject *umms_client_object_new (void);
DBusGProxy *umms_client_object_request_player (UmmsClientObject *self, gboolean attended, gdouble time_to_execution, gchar **name);
void umms_client_object_remove_player (UmmsClientObject *self, DBusGProxy *player);

G_END_DECLS

#endif /* _UMMS_CLIENT_OBJECT_H */
