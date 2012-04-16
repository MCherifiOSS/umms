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

#include "umms-video-output-backend.h"
#include "umms-utils.h"

G_DEFINE_TYPE (UmmsVideoOutputBackend, umms_video_output_backend, G_TYPE_OBJECT);

#define UMMS_VIDEO_OUTPUT_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), UMMS_TYPE_VIDEO_OUTPUT_BACKEND, UmmsAudioManagerBackendPrivate))

struct _UmmsVideoOutputBackendPrivate {
  gint dummy;
};

gboolean umms_video_output_backend_get_valid_video_output(UmmsVideoOutputBackend *self, gchar ***outputs, GError **err)
{
  TYPE_VMETHOD_CALL (VIDEO_OUTPUT_BACKEND, get_valid_video_output, outputs, err);
}

gboolean umms_video_output_backend_get_valid_mode(UmmsVideoOutputBackend *self, const gchar *output_name, gchar ***modes, GError **err)
{
  TYPE_VMETHOD_CALL (VIDEO_OUTPUT_BACKEND, get_valid_mode, output_name, modes, err);
}

gboolean umms_video_output_backend_set_mode (UmmsVideoOutputBackend *self, const gchar *output_name, const gchar *mode, GError **err)
{
  TYPE_VMETHOD_CALL (VIDEO_OUTPUT_BACKEND, set_mode, output_name, mode, err);
}

gboolean umms_video_output_backend_get_mode (UmmsVideoOutputBackend *self, const gchar *output_name, gchar **mode, GError **err)
{
  TYPE_VMETHOD_CALL (VIDEO_OUTPUT_BACKEND, get_mode, output_name, mode, err);
}

static void
umms_video_output_backend_dispose (GObject *object)
{
  G_OBJECT_CLASS (umms_video_output_backend_parent_class)->dispose (object);
}

static void
umms_video_output_backend_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_video_output_backend_parent_class)->finalize (object);
}

static void
umms_video_output_backend_class_init (UmmsVideoOutputBackendClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (UmmsVideoOutputBackendPrivate));
  gobject_class->dispose = umms_video_output_backend_dispose;
  gobject_class->finalize = umms_video_output_backend_finalize;
}

static void
umms_video_output_backend_init (UmmsVideoOutputBackend *self)
{

}
