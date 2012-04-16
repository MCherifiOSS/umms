/*
 * UMMS (Unified Multi Media Service) provides a set of DBus APIs to support
 * playing UmmsAudio and Video as well as DVB playback.
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

#ifndef __UMMS_VIDEO_OUTPUT_BACKEND_H__
#define __UMMS_VIDEO_OUTPUT_BACKEND_H__
#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_VIDEO_OUTPUT_BACKEND umms_video_output_backend_get_type ()

#define UMMS_VIDEO_OUTPUT_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT_BACKEND, UmmsVideoOutputBackend))

#define UMMS_VIDEO_OUTPUT_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_VIDEO_OUTPUT_BACKEND, UmmsVideoOutputBackendClass))

#define UMMS_IS_VIDEO_OUTPUT_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT_BACKEND))

#define UMMS_IS_VIDEO_OUTPUT_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_VIDEO_OUTPUT_BACKEND))

#define UMMS_VIDEO_OUTPUT_BACKEND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_VIDEO_OUTPUT_BACKEND, UmmsVideoOutputBackendClass))


typedef struct _UmmsVideoOutputBackend UmmsVideoOutputBackend;
typedef struct _UmmsVideoOutputBackendClass UmmsVideoOutputBackendClass;
typedef struct _UmmsVideoOutputBackendPrivate UmmsVideoOutputBackendPrivate;

struct _UmmsVideoOutputBackend {
  GObject parent;
  UmmsVideoOutputBackendPrivate *priv;
};

struct _UmmsVideoOutputBackendClass {
  GObjectClass parent_class;

  gboolean (*get_valid_video_output) (UmmsVideoOutputBackend *self, gchar ***outputs, GError **err);
  gboolean (*get_valid_mode) (UmmsVideoOutputBackend *self, const gchar *output_name, gchar ***modes, GError **err);
  gboolean (*set_mode) (UmmsVideoOutputBackend *self, const gchar *output_name, const gchar *mode, GError **err);
  gboolean (*get_mode) (UmmsVideoOutputBackend *self, const gchar *output_name, gchar **mode, GError **err);
};

GType umms_video_output_backend_get_type (void);
gboolean umms_video_output_backend_get_valid_video_output(UmmsVideoOutputBackend *self, gchar ***outputs, GError **err);
gboolean umms_video_output_backend_get_valid_mode(UmmsVideoOutputBackend *self, const gchar *output_name, gchar ***modes, GError **err);
gboolean umms_video_output_backend_set_mode (UmmsVideoOutputBackend *self, const gchar *output_name, const gchar *mode, GError **error);
gboolean umms_video_output_backend_get_mode (UmmsVideoOutputBackend *self, const gchar *output_name, gchar **mode, GError **error);

G_END_DECLS
#endif
