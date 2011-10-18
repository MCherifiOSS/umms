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

#ifndef _MEDIA_PLAYER_GSTREAMER_H
#define _MEDIA_PLAYER_GSTREAMER_H

#include <glib-object.h>
#include <media-player.h>

G_BEGIN_DECLS

#define TYPE_MEDIA_PLAYER_GSTREAMER media_player_gstreamer_get_type()

#define MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TYPE_MEDIA_PLAYER_GSTREAMER, MediaPlayerGstreamer))

#define MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TYPE_MEDIA_PLAYER_GSTREAMER, MediaPlayerGstreamerClass))

#define IS_MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TYPE_MEDIA_PLAYER_GSTREAMER))

#define IS_MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TYPE_MEDIA_PLAYER_GSTREAMER))

#define MEDIA_PLAYER_GSTREAMER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TYPE_MEDIA_PLAYER_GSTREAMER, MediaPlayerGstreamerClass))

typedef struct _MediaPlayerGstreamer MediaPlayerGstreamer;
typedef struct _MediaPlayerGstreamerClass MediaPlayerGstreamerClass;
typedef struct _MediaPlayerGstreamerPrivate MediaPlayerGstreamerPrivate;

struct _MediaPlayerGstreamer
{
  MediaPlayer parent;

  MediaPlayerGstreamerPrivate *priv;
};


struct _MediaPlayerGstreamerClass
{
  MediaPlayerClass parent_class;

};

GType media_player_gstreamer_get_type (void) G_GNUC_CONST;

MediaPlayerGstreamer *media_player_gstreamer_new (void);

G_END_DECLS

#endif /* _MEDIA_PLAYER_GSTREAMER_H */
