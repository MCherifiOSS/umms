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

#include "umms-server.h"
#include "umms-debug.h"
#include "umms-types.h"
#include "umms-video-output.h"
#include "umms-video-output-backend.h"
#include "umms-backend-factory.h"

G_DEFINE_TYPE (UmmsVideoOutput, umms_video_output, G_TYPE_OBJECT)

#define UMMS_VIDEO_OUTPUT_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_VIDEO_OUTPUT, UmmsVideoOutputPrivate))

#define GET_PRIVATE(o) ((UmmsVideoOutput *)o)->priv

struct _UmmsVideoOutputPrivate {
  UmmsVideoOutputBackend *backend;
};

static void
umms_video_output_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_video_output_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (object);
  gint tmp;
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_video_output_dispose (GObject *object)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (umms_video_output_parent_class)->dispose (object);
}

static void
umms_video_output_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_video_output_parent_class)->finalize (object);
}

static void
umms_video_output_class_init (UmmsVideoOutputClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsVideoOutputPrivate));

  object_class->get_property = umms_video_output_get_property;
  object_class->set_property = umms_video_output_set_property;
  object_class->dispose = umms_video_output_dispose;
  object_class->finalize = umms_video_output_finalize;
}

static void
umms_video_output_init (UmmsVideoOutput *self)
{
  self->priv = UMMS_VIDEO_OUTPUT_GET_PRIVATE (self);
  self->priv->backend = (UmmsVideoOutputBackend *)umms_backend_factory_make (UMMS_PLUGIN_TYPE_VIDEO_OUTPUT_BACKEND);
  if (self->priv->backend == NULL)
    UMMS_WARNING ("failed to load backend");
  return;
}

UmmsVideoOutput *
umms_video_output_new ()
{
  return g_object_new (UMMS_TYPE_VIDEO_OUTPUT, NULL);
}

gboolean umms_video_output_get_valid_video_output(UmmsVideoOutput *self, gchar ***outputs, GError **err)
{

  UmmsVideoOutputPrivate *priv = GET_PRIVATE (self);

  return umms_video_output_backend_get_valid_video_output (priv->backend, outputs, err);
}

gboolean umms_video_output_get_valid_mode (UmmsVideoOutput *self, const char* output_name, gchar ***modes, GError **err)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (self);

  return umms_video_output_backend_get_valid_mode (priv->backend, output_name, modes, err);
}

gboolean umms_video_output_set_mode(UmmsVideoOutput *self, const char *output_name, const char *mode, GError **err)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (self);

  return umms_video_output_backend_set_mode (priv->backend, output_name, mode, err);
}

gboolean umms_video_output_get_mode(UmmsVideoOutput *self, const char *output_name, char **mode, GError **err)
{

  UmmsVideoOutputPrivate *priv = GET_PRIVATE (self);

  return umms_video_output_backend_get_mode (priv->backend, output_name, mode, err);
}
