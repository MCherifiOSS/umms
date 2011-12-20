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

#ifndef _UMMS_VIDEO_OUTPUT_H
#define _UMMS_VIDEO_OUTPUT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_VIDEO_OUTPUT umms_video_output_get_type()

#define UMMS_VIDEO_OUTPUT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT, UmmsVideoOutput))

#define UMMS_VIDEO_OUTPUT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_VIDEO_OUTPUT, UmmsVideoOutputClass))

#define UMMS_IS_VIDEO_OUTPUT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT))

#define UMMS_IS_VIDEO_OUTPUT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_VIDEO_OUTPUT))

#define UMMS_VIDEO_OUTPUT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT, UmmsVideoOutputClass))

typedef struct _UmmsVideoOutput UmmsVideoOutput;
typedef struct _UmmsVideoOutputClass UmmsVideoOutputClass;
typedef struct _UmmsVideoOutputPrivate UmmsVideoOutputPrivate;
typedef struct _UmmsVideoOutputInterface UmmsVideoOutputInterface;

struct _UmmsVideoOutputInterface
{
  gboolean (*init) (UmmsVideoOutput *video_output);
  gboolean (*get_valid_video_output) (UmmsVideoOutput *video_output, gchar ***outputs);
  gboolean (*get_valid_mode) (UmmsVideoOutput *video_output, const gchar *output_name, gchar ***modes);
  gboolean (*set_mode) (UmmsVideoOutput *video_output, const gchar *output_name, const gchar *mode);
  gboolean (*get_mode) (UmmsVideoOutput *video_output, const gchar *output_name, gchar **mode);
};

struct _UmmsVideoOutput
{
  GObject parent;
  UmmsVideoOutputPrivate *priv;
  UmmsVideoOutputInterface video_output_if;
};

struct _UmmsVideoOutputClass
{
    GObjectClass parent_class;
};

GType umms_video_output_get_type (void) G_GNUC_CONST;

UmmsVideoOutput *umms_video_output_new ();
gboolean umms_video_output_get_valid_video_output(UmmsVideoOutput *self, gchar ***outputs, GError **err);
gboolean umms_video_output_get_valid_mode(UmmsVideoOutput *self, const gchar *output_name, gchar ***modes, GError **err);
gboolean umms_video_output_set_mode (UmmsVideoOutput *self, const gchar *output_name, const gchar *mode, GError **error);
gboolean umms_video_output_get_mode (UmmsVideoOutput *self, const gchar *output_name, gchar **mode, GError **error);

G_END_DECLS

#endif /* _UMMS_VIDEO_OUTPUT_H */
