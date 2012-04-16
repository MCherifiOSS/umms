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

#ifndef _UMMS_AUDIO_MANAGER_H
#define _UMMS_AUDIO_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UMMS_TYPE_AUDIO_MANAGER umms_audio_manager_get_type()

#define UMMS_AUDIO_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManager))

#define UMMS_AUDIO_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManagerClass))

#define UMMS_IS_AUDIO_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  UMMS_TYPE_AUDIO_MANAGER))

#define UMMS_IS_AUDIO_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  UMMS_TYPE_AUDIO_MANAGER))

#define UMMS_AUDIO_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManagerClass))

typedef struct _UmmsAudioManager UmmsAudioManager;
typedef struct _UmmsAudioManagerClass UmmsAudioManagerClass;
typedef struct _UmmsAudioManagerPrivate UmmsAudioManagerPrivate;


struct _UmmsAudioManager {
  GObject parent;
  UmmsAudioManagerPrivate *priv;
};


struct _UmmsAudioManagerClass {
  GObjectClass parent_class;
};

GType umms_audio_manager_get_type (void) G_GNUC_CONST;

UmmsAudioManager *umms_audio_manager_new (void);
gboolean umms_audio_manager_set_volume(UmmsAudioManager *self, gint type, gint vol, GError **error);
gboolean umms_audio_manager_get_volume(UmmsAudioManager *self, gint type, gint *vol, GError **error);
gboolean umms_audio_manager_set_state(UmmsAudioManager *self, gint type, gint state, GError **error);
gboolean umms_audio_manager_get_state(UmmsAudioManager *self, gint type, gint *state, GError **error);

G_END_DECLS

#endif /* _UMMS_AUDIO_MANAGER_H */
