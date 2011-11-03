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

#ifndef _PLAYER_CONTROL_FACTORY_H
#define _PLAYER_CONTROL_FACTORY_H

#include <glib-object.h>
//#include <media-player.h>
#include "media-player-control.h"

G_BEGIN_DECLS

#define PLAYER_CONTROL_TYPE_FACTORY player_control_factory_get_type()

#define PLAYER_CONTROL_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  PLAYER_CONTROL_TYPE_FACTORY, PlayerControlFactory))

#define PLAYER_CONTROL_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  PLAYER_CONTROL_TYPE_FACTORY, PlayerControlFactoryClass))

#define IS_PLAYER_CONTROL_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  PLAYER_CONTROL_TYPE_FACTORY))

#define IS_PLAYER_CONTROL_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  PLAYER_CONTROL_TYPE_FACTORY))

#define PLAYER_CONTROL_FACTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  PLAYER_CONTROL_TYPE_FACTORY, PlayerControlFactoryClass))

typedef struct _PlayerControlFactory PlayerControlFactory;
typedef struct _PlayerControlFactoryClass PlayerControlFactoryClass;
typedef struct _PlayerControlFactoryPrivate PlayerControlFactoryPrivate;

struct _PlayerControlFactory
{
 // PlayerControl parent;
  GObject parent;

  PlayerControlFactoryPrivate *priv;
};


struct _PlayerControlFactoryClass
{
 // PlayerControlClass parent_class;
  GObjectClass parent_class;

  /*
     *
     * self:            A PlayerControl
     * uri:             Uri to play
     * new_engine[out]: TRUE if loaded a new engine and destroyed old engine
     *                  FALSE if not
     *
     * Returns:         TRUE if successful
     *                  FALSE if not
     *
     * Load engine according to uri prefix which indicates the source type, and store it in PlayerControl::player_control.
     * Subclass should implement this vmethod to customize the procedure of backend engine loading.
     *          
     */
  MediaPlayerControl* (*load_engine) (PlayerControlFactory *self, const char *uri, gboolean *new_engine);

};

GType player_control_factory_get_type (void) G_GNUC_CONST;

PlayerControlFactory *player_control_factory_new (void);

G_END_DECLS

#endif /* _PLAYER_CONTROL_FACTORY_H */
