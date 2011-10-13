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

#ifndef _ENGINE_GENERIC_H
#define _ENGINE_GENERIC_H

#include <glib-object.h>

G_BEGIN_DECLS

#define ENGINE_TYPE_GENERIC engine_generic_get_type()

#define ENGINE_GENERIC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  ENGINE_TYPE_GENERIC, EngineGeneric))

#define ENGINE_GENERIC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  ENGINE_TYPE_GENERIC, EngineGenericClass))

#define ENGINE_IS_GENERIC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  ENGINE_TYPE_GENERIC))

#define ENGINE_IS_GENERIC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  ENGINE_TYPE_GENERIC))

#define ENGINE_GENERIC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  ENGINE_TYPE_GENERIC, EngineGenericClass))

typedef struct _EngineGeneric EngineGeneric;
typedef struct _EngineGenericClass EngineGenericClass;
typedef struct _EngineGenericPrivate EngineGenericPrivate;

struct _EngineGeneric
{
  GObject parent;

  EngineGenericPrivate *priv;
};


struct _EngineGenericClass
{
  GObjectClass parent_class;

};

GType engine_generic_get_type (void) G_GNUC_CONST;

EngineGeneric *engine_generic_new (void);

G_END_DECLS

#endif /* _ENGINE_GENERIC_H */
