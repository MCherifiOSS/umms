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

#ifndef __AUDIO_MANAGER_INTERFACE_H__
#define __AUDIO_MANAGER_INTERFACE_H__
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * AudioManagerInterface:
 *
 * Dummy typedef representing any implementation of this interface.
 */
typedef struct _AudioManagerInterface AudioManagerInterface;

/**
 * AudioManagerInterfaceClass:
 *
 * The class of AudioManagerInterface.
 */
typedef struct _AudioManagerInterfaceClass AudioManagerInterfaceClass;

GType audio_manager_interface_get_type (void);
#define AUDIO_TYPE_MANAGER_INTERFACE audio_manager_interface_get_type ()

#define AUDIO_MANAGER_INTERFACE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), AUDIO_TYPE_MANAGER_INTERFACE, AudioManagerInterface))
#define AUDIO_IS_MANAGER_INTERFACE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), AUDIO_TYPE_MANAGER_INTERFACE))
#define AUDIO_MANAGER_INTERFACE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE((obj), AUDIO_TYPE_MANAGER_INTERFACE, AudioManagerInterfaceClass))

typedef gboolean (*audio_manager_interface_set_volume_impl) (AudioManagerInterface *self, gint type, gint volume);
void audio_manager_interface_implement_set_volume (AudioManagerInterfaceClass *klass, 
                                                    audio_manager_interface_set_volume_impl impl);

typedef gboolean (*audio_manager_interface_get_volume_impl) (AudioManagerInterface *self, gint type, gint *volume);
void audio_manager_interface_implement_get_volume (AudioManagerInterfaceClass *klass, 
                                                     audio_manager_interface_get_volume_impl impl);

typedef gboolean (*audio_manager_interface_set_state_impl) (AudioManagerInterface *self, gint type, gint state);
void audio_manager_interface_implement_set_state (AudioManagerInterfaceClass *klass, 
                                                    audio_manager_interface_set_state_impl impl);

typedef gboolean (*audio_manager_interface_get_state_impl) (AudioManagerInterface *self, gint type, gint *state);
void audio_manager_interface_implement_get_state (AudioManagerInterfaceClass *klass, 
                                                     audio_manager_interface_get_state_impl impl);

/*virtual function wrappers*/
gboolean audio_manager_interface_set_volume (AudioManagerInterface *self, gint type, gint volume);
gboolean audio_manager_interface_get_volume (AudioManagerInterface *self, gint type, gint *volume);
gboolean audio_manager_interface_set_state (AudioManagerInterface *self, gint type, gint state);
gboolean audio_manager_interface_get_state (AudioManagerInterface *self, gint type, gint *state);

G_END_DECLS
#endif
