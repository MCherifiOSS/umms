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

#ifndef _MEEGO_MEDIA_PLAYER_GSTREAMER_H
#define _MEEGO_MEDIA_PLAYER_GSTREAMER_H

#include <glib-object.h>
#include <meego-media-player.h>

G_BEGIN_DECLS

#define MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER meego_media_player_gstreamer_get_type()

#define MEEGO_MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamer))

#define MEEGO_MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamerClass))

#define MEEGO_IS_MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER))

#define MEEGO_IS_MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER))

#define MEEGO_MEDIA_PLAYER_GSTREAMER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamerClass))

typedef struct _MeegoMediaPlayerGstreamer MeegoMediaPlayerGstreamer;
typedef struct _MeegoMediaPlayerGstreamerClass MeegoMediaPlayerGstreamerClass;
typedef struct _MeegoMediaPlayerGstreamerPrivate MeegoMediaPlayerGstreamerPrivate;

struct _MeegoMediaPlayerGstreamer
{
  MeegoMediaPlayer parent;

  MeegoMediaPlayerGstreamerPrivate *priv;
};


struct _MeegoMediaPlayerGstreamerClass
{
  MeegoMediaPlayerClass parent_class;

};

GType meego_media_player_gstreamer_get_type (void) G_GNUC_CONST;

MeegoMediaPlayerGstreamer *meego_media_player_gstreamer_new (void);

G_END_DECLS

#endif /* _MEEGO_MEDIA_PLAYER_GSTREAMER_H */
