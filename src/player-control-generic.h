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

#ifndef _PLAYER_CONTROL_GENERIC_H
#define _PLAYER_CONTROL_GENERIC_H

#include <glib-object.h>
#include "player-control-base.h"

G_BEGIN_DECLS

#define PLAYER_CONTROL_TYPE_GENERIC player_control_generic_get_type()

#define PLAYER_CONTROL_GENERIC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  PLAYER_CONTROL_TYPE_GENERIC, PlayerControlGeneric))

#define PLAYER_CONTROL_GENERIC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  PLAYER_CONTROL_TYPE_GENERIC, PlayerControlGenericClass))

#define PLAYER_CONTROL_IS_GENERIC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  PLAYER_CONTROL_TYPE_GENERIC))

#define PLAYER_CONTROL_IS_GENERIC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  PLAYER_CONTROL_TYPE_GENERIC))

#define PLAYER_CONTROL_GENERIC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  PLAYER_CONTROL_TYPE_GENERIC, PlayerControlGenericClass))

typedef struct _PlayerControlGeneric PlayerControlGeneric;
typedef struct _PlayerControlGenericClass PlayerControlGenericClass;
//typedef struct _PlayerControlGenericPrivate PlayerControlGenericPrivate;

struct _PlayerControlGeneric
{
  /*GObject parent;*/
  PlayerControlBase parent;

  /*PlayerControlGenericPrivate *priv;*/
};


struct _PlayerControlGenericClass
{
  /*GObjectClass parent_class;*/
  PlayerControlBaseClass parent_class;

};

GType player_control_generic_get_type (void) G_GNUC_CONST;

PlayerControlGeneric *player_control_generic_new (void);

G_END_DECLS

#endif /* _PLAYER_CONTROL_GENERIC_H */
