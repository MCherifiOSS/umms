#include <gst/gst.h>

#include "umms-debug.h"
#include "umms-common.h"
#include "umms-error.h"
#include "umms-marshals.h"
#include "umms-audio-manager.h"
#include "audio-manager-interface.h"

G_DEFINE_TYPE (UmmsAudioManager, umms_audio_manager, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManagerPrivate))
#define GET_PRIVATE(o) ((UmmsAudioManager *)o)->priv

struct _UmmsAudioManagerPrivate {
  AudioManagerInterface *audio_mngr_if;
};

static void
umms_audio_manager_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (object);

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
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (object);
  const gchar *tmp;

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_audio_manager_dispose (GObject *object)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (object);
  g_object_unref (priv->audio_mngr_if);

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
umms_audio_manager_init (UmmsAudioManager *player)
{
  player->priv = PLAYER_PRIVATE (player);
  player->priv->audio_mngr_if = AUDIO_MANAGER_INTERFACE(audio_manager_engine_new ());//TODO:create audio manager interface instance here.
}

UmmsAudioManager *
umms_audio_manager_new (void)
{
  return g_object_new (UMMS_TYPE_AUDIO_MANAGER, NULL);
}

gboolean umms_audio_manager_set_volume(UmmsAudioManager *self, gint type, gint vol, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  if (!audio_manager_interface_set_volume (priv->audio_mngr_if, type, vol)) {
    g_assert (err == NULL || *err == NULL);
    if (err) {
      g_set_error (err, UMMS_AUDIO_ERROR, UMMS_AUDIO_ERROR_FAILED, "Setting volume failed, output type = %d", type);
    }
    return FALSE;
  }
  return TRUE;
}

gboolean umms_audio_manager_get_volume(UmmsAudioManager *self, gint type, gint *vol, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  if (!audio_manager_interface_get_volume (priv->audio_mngr_if, type, vol)) {
    g_assert (err == NULL || *err == NULL);
    if (err) {
      g_set_error (err, UMMS_AUDIO_ERROR, UMMS_AUDIO_ERROR_FAILED, "Getting volume failed, output type = %d", type);
    }
    return FALSE;
  }
  return TRUE;
}

gboolean umms_audio_manager_set_state(UmmsAudioManager *self, gint type, gint state, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  if (!audio_manager_interface_set_state (priv->audio_mngr_if, type, state)) {
    g_assert (err == NULL || *err == NULL);
    if (err) {
      g_set_error (err, UMMS_AUDIO_ERROR, UMMS_AUDIO_ERROR_FAILED, "Setting audio output state failed, output type = %d", type);
    }
    return FALSE;
  }
  return TRUE;
}

gboolean umms_audio_manager_get_state(UmmsAudioManager *self, gint type, gint *state, GError **err)
{
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (self);

  if (!audio_manager_interface_get_state (priv->audio_mngr_if, type, state)) {
    g_assert (err == NULL || *err == NULL);
    if (err) {
      g_set_error (err, UMMS_AUDIO_ERROR, UMMS_AUDIO_ERROR_FAILED, "Getting audio output state failed, output type = %d", type);
    }
    return FALSE;
  }
  return TRUE;
}
