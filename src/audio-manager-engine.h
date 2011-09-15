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

#ifndef _AUDIO_MANAGER_ENGINE_H
#define _AUDIO_MANAGER_ENGINE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define AUDIO_TYPE_MANAGER_ENGINE audio_manager_engine_get_type()

#define AUDIO_MANAGER_ENGINE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  AUDIO_TYPE_MANAGER_ENGINE, AudioManagerEngine))

#define AUDIO_MANAGER_ENGINE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  AUDIO_TYPE_MANAGER_ENGINE, AudioManagerEngineClass))

#define AUDIO_IS_MANAGER_ENGINE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  AUDIO_TYPE_MANAGER_ENGINE))

#define AUDIO_IS_MANAGER_ENGINE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  AUDIO_TYPE_MANAGER_ENGINE))

#define AUDIO_MANAGER_ENGINE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  AUDIO_TYPE_MANAGER_ENGINE, AudioManagerEngineClass))

typedef struct _AudioManagerEngine AudioManagerEngine;
typedef struct _AudioManagerEngineClass AudioManagerEngineClass;
typedef struct _AudioManagerEnginePrivate AudioManagerEnginePrivate;

struct _AudioManagerEngine
{
  GObject parent;

  AudioManagerEnginePrivate *priv;
};


struct _AudioManagerEngineClass
{
  GObjectClass parent_class;

};

GType audio_manager_engine_get_type (void) G_GNUC_CONST;

AudioManagerEngine *audio_manager_engine_new (void);

G_END_DECLS

#endif /* _AUDIO_MANAGER_ENGINE_H */
