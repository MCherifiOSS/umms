#ifndef _UMMS_AUDIO_MANAGER_H
#define _UMMS_AUDIO_MANAGER_H

#include <glib-object.h>
#include "audio-manager-interface.h"

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


struct _UmmsAudioManager
{
  GObject parent;
  UmmsAudioManagerPrivate *priv;
};


struct _UmmsAudioManagerClass
{
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
