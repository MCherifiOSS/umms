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

#ifndef __UMMS_AUDIO_MANAGER_BACKEND_H__
#define __UMMS_AUDIO_MANAGER_BACKEND_H__
#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_AUDIO_MANAGER_BACKEND umms_audio_manager_backend_get_type ()

#define UMMS_AUDIO_MANAGER_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_AUDIO_MANAGER_BACKEND, UmmsAudioManagerBackend))

#define UMMS_AUDIO_MANAGER_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_AUDIO_MANAGER_BACKEND, UmmsAudioManagerBackendClass))

#define UMMS_IS_AUDIO_MANAGER_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_AUDIO_MANAGER_BACKEND))

#define UMMS_IS_AUDIO_MANAGER_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_AUDIO_MANAGER_BACKEND))

#define UMMS_AUDIO_MANAGER_BACKEND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_AUDIO_MANAGER_BACKEND, UmmsAudioManagerBackendClass))


typedef struct _UmmsAudioManagerBackend UmmsAudioManagerBackend;
typedef struct _UmmsAudioManagerBackendClass UmmsAudioManagerBackendClass;
typedef struct _UmmsAudioManagerBackendPrivate UmmsAudioManagerBackendPrivate;

struct _UmmsAudioManagerBackend {
  GObject parent;
  UmmsAudioManagerBackendPrivate *priv;
};

struct _UmmsAudioManagerBackendClass {
  GObjectClass parent_class;

  gboolean (*set_volume) (UmmsAudioManagerBackend *self, gint type, gint volume, GError **err);
  gboolean (*get_volume) (UmmsAudioManagerBackend *self, gint type, gint *volume, GError **err);
  gboolean (*set_state) (UmmsAudioManagerBackend *self, gint type, gint state, GError **err);
  gboolean (*get_state) (UmmsAudioManagerBackend *self, gint type, gint *state, GError **err);
};

GType umms_audio_manager_backend_get_type (void);
/*virtual function wrappers*/
gboolean umms_audio_manager_backend_set_volume (UmmsAudioManagerBackend *self, gint type, gint volume, GError **err);
gboolean umms_audio_manager_backend_get_volume (UmmsAudioManagerBackend *self, gint type, gint *volume, GError **err);
gboolean umms_audio_manager_backend_set_state (UmmsAudioManagerBackend *self, gint type, gint state, GError **err);
gboolean umms_audio_manager_backend_get_state (UmmsAudioManagerBackend *self, gint type, gint *state, GError **err);

G_END_DECLS
#endif
