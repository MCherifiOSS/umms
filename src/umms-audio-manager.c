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
#include "umms-error.h"
#include "umms-marshals.h"
#include "umms-plugin.h"
#include "umms-audio-manager.h"
#include "umms-audio-manager-backend.h"

G_DEFINE_TYPE (UmmsAudioManager, umms_audio_manager, G_TYPE_OBJECT)

#define UMMS_AUDIO_MANAGER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManagerPrivate))
#define GET_PRIVATE(o) ((UmmsAudioManager *)o)->priv

struct _UmmsAudioManagerPrivate {
  UmmsAudioManagerBackend *backend;
};

/*property for audio manager*/
enum PROPTYPE {
  PROP_0,
  PROP_LAST
};

static void
umms_audio_manager_get_property (GObject    *object,
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
umms_audio_manager_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  gint tmp;
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_audio_manager_dispose (GObject *object)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (object);
  g_object_unref (priv->backend);

  G_OBJECT_CLASS (umms_audio_manager_parent_class)->dispose (object);
}

static void
umms_audio_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_audio_manager_parent_class)->finalize (object);
}

static void
umms_audio_manager_class_init (UmmsAudioManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsAudioManagerPrivate));

  object_class->get_property = umms_audio_manager_get_property;
  object_class->set_property = umms_audio_manager_set_property;
  object_class->dispose = umms_audio_manager_dispose;
  object_class->finalize = umms_audio_manager_finalize;
}

static void
umms_audio_manager_init (UmmsAudioManager *self)
{
  self->priv = UMMS_AUDIO_MANAGER_GET_PRIVATE (self);

  self->priv->backend = (UmmsAudioManagerBackend *)umms_backend_factory_make (UMMS_PLUGIN_TYPE_AUDIO_MANAGER_BACKEND);
  if (self->priv->backend == NULL)
    UMMS_WARNING ("failed to load backend");
}

UmmsAudioManager *
umms_audio_manager_new (void)
{
  return g_object_new (UMMS_TYPE_AUDIO_MANAGER, NULL);
}

gboolean umms_audio_manager_set_volume(UmmsAudioManager *self, gint type, gint vol, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  return umms_audio_manager_backend_set_volume (priv->backend, type, vol, err);
}

gboolean umms_audio_manager_get_volume(UmmsAudioManager *self, gint type, gint *vol, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  return umms_audio_manager_backend_get_volume (priv->backend, type, vol, err);
}

gboolean umms_audio_manager_set_state(UmmsAudioManager *self, gint type, gint state, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  return umms_audio_manager_backend_set_state (priv->backend, type, state, err);
}

gboolean umms_audio_manager_get_state(UmmsAudioManager *self, gint type, gint *state, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  return umms_audio_manager_backend_get_state (priv->backend, type, state, err);
}
