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

#ifndef _MEDIA_PLAYER_FACTORY_H
#define _MEDIA_PLAYER_FACTORY_H

#include <glib-object.h>
//#include <media-player.h>
#include "media-player-control.h"

G_BEGIN_DECLS

#define TYPE_MEDIA_PLAYER_FACTORY media_player_factory_get_type()

#define MEDIA_PLAYER_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_MEDIA_PLAYER_FACTORY, MediaPlayerFactory))

#define MEDIA_PLAYER_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_MEDIA_PLAYER_FACTORY, MediaPlayerFactoryClass))

#define IS_MEDIA_PLAYER_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_MEDIA_PLAYER_FACTORY))

#define IS_MEDIA_PLAYER_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_MEDIA_PLAYER_FACTORY))

#define MEDIA_PLAYER_FACTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_MEDIA_PLAYER_FACTORY, MediaPlayerFactoryClass))

typedef struct _MediaPlayerFactory MediaPlayerFactory;
typedef struct _MediaPlayerFactoryClass MediaPlayerFactoryClass;
typedef struct _MediaPlayerFactoryPrivate MediaPlayerFactoryPrivate;

struct _MediaPlayerFactory
{
 // MediaPlayer parent;
  GObject parent;

  MediaPlayerFactoryPrivate *priv;
};


struct _MediaPlayerFactoryClass
{
 // MediaPlayerClass parent_class;
  GObjectClass parent_class;

  /*
     *
     * self:            A MediaPlayer
     * uri:             Uri to play
     * new_engine[out]: TRUE if loaded a new engine and destroyed old engine
     *                  FALSE if not
     *
     * Returns:         TRUE if successful
     *                  FALSE if not
     *
     * Load engine according to uri prefix which indicates the source type, and store it in MediaPlayer::player_control.
     * Subclass should implement this vmethod to customize the procedure of backend engine loading.
     *          
     */
  MediaPlayerControl* (*load_engine) (MediaPlayerFactory *self, const char *uri, gboolean *new_engine);

};

GType media_player_factory_get_type (void) G_GNUC_CONST;

MediaPlayerFactory *media_player_factory_new (void);

G_END_DECLS

#endif /* _MEDIA_PLAYER_FACTORY_H */
