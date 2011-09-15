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

#ifndef _UMMS_PLAYING_CONTENT_METADATA_VIEWER_H
#define _UMMS_PLAYING_CONTENT_METADATA_VIEWER_H

#include <glib-object.h>
#include "umms-object-manager.h"

G_BEGIN_DECLS

#define UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER umms_playing_content_metadata_viewer_get_type()

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, UmmsPlayingContentMetadataViewer))

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, UmmsPlayingContentMetadataViewerClass))

#define UMMS_IS_PLAYING_CONTENT_METADATA_VIEWER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER))

#define UMMS_IS_PLAYING_CONTENT_METADATA_VIEWER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER))

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, UmmsPlayingContentMetadataViewerClass))

typedef struct _UmmsPlayingContentMetadataViewer UmmsPlayingContentMetadataViewer;
typedef struct _UmmsPlayingContentMetadataViewerClass UmmsPlayingContentMetadataViewerClass;
typedef struct _UmmsPlayingContentMetadataViewerPrivate UmmsPlayingContentMetadataViewerPrivate;

struct _UmmsPlayingContentMetadataViewer
{
  GObject parent;

  UmmsPlayingContentMetadataViewerPrivate *priv;
};


struct _UmmsPlayingContentMetadataViewerClass
{
  GObjectClass parent_class;

};

GType umms_playing_content_metadata_viewer_get_type (void) G_GNUC_CONST;

UmmsPlayingContentMetadataViewer *umms_playing_content_metadata_viewer_new (UmmsObjectManager *obj_mngr);

gboolean 
umms_playing_content_metadata_viewer_get_playing_content_metadata (UmmsPlayingContentMetadataViewer *self, GPtrArray **metadata, GError **err);

G_END_DECLS

#endif /* _UMMS_PLAYING_CONTENT_METADATA_VIEWER_H */
