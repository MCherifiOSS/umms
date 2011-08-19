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
