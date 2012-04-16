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

#include "umms-audio-manager-backend.h"
#include "umms-utils.h"

G_DEFINE_TYPE (UmmsAudioManagerBackend, umms_audio_manager_backend, G_TYPE_OBJECT);

#define UMMS_AUDIO_MANAGER_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), UMMS_TYPE_AUDIO_MANAGER_BACKEND, UmmsAudioManagerBackendPrivate))

struct _UmmsAudioManagerBackendPrivate {
  gint dummy;
};

gboolean
umms_audio_manager_backend_set_volume (UmmsAudioManagerBackend *self,
                                       gint type, gint volume, GError **err)
{
  TYPE_VMETHOD_CALL (AUDIO_MANAGER_BACKEND, set_volume, type, volume, err);
}

gboolean
umms_audio_manager_backend_get_volume (UmmsAudioManagerBackend *self,
                                       gint type, gint *volume, GError **err)
{
  TYPE_VMETHOD_CALL (AUDIO_MANAGER_BACKEND, get_volume, type, volume, err);
}

gboolean
umms_audio_manager_backend_set_state (UmmsAudioManagerBackend *self,
                                      gint type, gint state, GError **err)
{
  TYPE_VMETHOD_CALL (AUDIO_MANAGER_BACKEND, set_state, type, state, err);
}

gboolean
umms_audio_manager_backend_get_state (UmmsAudioManagerBackend *self,
                                      gint type, gint *state, GError **err)
{
  TYPE_VMETHOD_CALL (AUDIO_MANAGER_BACKEND, get_state, type, state, err);
}

static void
umms_audio_manager_backend_dispose (GObject *object)
{
  G_OBJECT_CLASS (umms_audio_manager_backend_parent_class)->dispose (object);
}

static void
umms_audio_manager_backend_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_audio_manager_backend_parent_class)->finalize (object);
}

static void
umms_audio_manager_backend_class_init (UmmsAudioManagerBackendClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (UmmsAudioManagerBackendPrivate));
  gobject_class->dispose = umms_audio_manager_backend_dispose;
  gobject_class->finalize = umms_audio_manager_backend_finalize;
}

static void
umms_audio_manager_backend_init (UmmsAudioManagerBackend *self)
{

}
