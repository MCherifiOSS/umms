#include <string.h>
#include <stdio.h>
#include <math.h>
#include <gst/gst.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "audio-manager-engine.h"
#include "audio-manager-interface.h"

static void audio_manager_interface_init (AudioManagerInterface* iface);

G_DEFINE_TYPE_WITH_CODE (AudioManagerEngine, audio_manager_engine, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (AUDIO_TYPE_MANAGER_INTERFACE, audio_manager_interface_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AUDIO_TYPE_MANAGER_ENGINE, AudioManagerEnginePrivate))

#define GET_PRIVATE(o) ((AudioManagerEngine *)o)->priv

#define RESET_STR(str) \
     if (str) {       \
       g_free(str);   \
       str = NULL;    \
     }

#define TEARDOWN_ELEMENT(ele)                      \
    if (ele) {                                     \
      gst_element_set_state (ele, GST_STATE_NULL); \
      g_object_unref (ele);                        \
      ele = NULL;                                  \
    }

#define VOLUME_STEP ((gdouble)100/8)
#define TO_ISMD_VOLUME(vol, ismd_volume)  \
  do {                                    \
    vol = CLAMP (vol, 0, 100);            \
    ismd_volume = vol/VOLUME_STEP;        \
  }while (0)

#define TO_APP_VOLUME(ismd_volume, vol)   \
  do {                                    \
    vol = round(ismd_volume*VOLUME_STEP);        \
  }while(0)

struct _AudioManagerEnginePrivate {
  GstElement *asink;
};

static gboolean
audio_manager_engine_set_volume (AudioManagerInterface *self, gint type, gint vol)
{
  GstElement *asink;
  AudioManagerEnginePrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (AUDIO_IS_MANAGER_INTERFACE(self), FALSE);

  priv = GET_PRIVATE (self);
  asink = priv->asink;
  g_return_val_if_fail (GST_IS_ELEMENT (asink), FALSE);

  UMMS_DEBUG ("invoked, in vol = %d", vol);
  TO_ISMD_VOLUME(vol, volume);
  UMMS_DEBUG ("ismd vol = %f", volume);
  g_object_set (asink, "volume", volume, NULL);
  return TRUE;
}

audio_manager_engine_get_volume (AudioManagerInterface *self, gint type, gint *vol)
{
  GstElement *asink;
  AudioManagerEnginePrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (AUDIO_IS_MANAGER_INTERFACE(self), FALSE);

  priv = GET_PRIVATE (self);
  asink = priv->asink;
  g_return_val_if_fail (GST_IS_ELEMENT (asink), FALSE);

  g_object_get (asink, "volume", &volume, NULL);
  UMMS_DEBUG ("current volume = %f", volume);
  TO_APP_VOLUME(volume, *vol);
  UMMS_DEBUG ("application volume = %d", *vol);
  return TRUE;
}

static gboolean
audio_manager_engine_set_state (AudioManagerInterface *self, gint type, gint state)
{
  GstElement *asink;
  AudioManagerEnginePrivate *priv;
  gdouble volume;
  const gchar *output_type = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (AUDIO_IS_MANAGER_INTERFACE(self), FALSE);

  priv = GET_PRIVATE (self);
  asink = priv->asink;
  g_return_val_if_fail (GST_IS_ELEMENT (asink), FALSE);

  UMMS_DEBUG ("invoked");
  switch (type) {
    case AUDIO_OUTPUT_TYPE_HDMI:
      output_type = "audio-output-hdmi";
      break;
    case AUDIO_OUTPUT_TYPE_SPDIF:
      output_type = "audio-output-spdif";
      break;
    case AUDIO_OUTPUT_TYPE_I2S0:
      output_type = "audio-output-i2s0";
      break;
    case AUDIO_OUTPUT_TYPE_I2S1:
      output_type = "audio-output-i2s1";
      break;
    default:
      break;
  }
  g_object_set (asink, output_type, state, NULL);
  return TRUE;
}

static gboolean
audio_manager_engine_get_state (AudioManagerInterface *self, gint type, gint *state)
{
  GstElement *asink;
  AudioManagerEnginePrivate *priv;
  gdouble vol;
  gint state_tmp;
  gchar *output_type;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (AUDIO_IS_MANAGER_INTERFACE(self), FALSE);

  priv = GET_PRIVATE (self);
  asink = priv->asink;
  g_return_val_if_fail (GST_IS_ELEMENT (asink), FALSE);

  UMMS_DEBUG ("invoked");
  switch (type) {
    case AUDIO_OUTPUT_TYPE_HDMI:
      output_type = "audio-output-hdmi";
      break;
    case AUDIO_OUTPUT_TYPE_SPDIF:
      output_type = "audio-output-spdif";
      break;
    case AUDIO_OUTPUT_TYPE_I2S0:
      output_type = "audio-output-i2s0";
      break;
    case AUDIO_OUTPUT_TYPE_I2S1:
      output_type = "audio-output-i2s1";
      break;
    default:
      break;
  }
  g_object_get (asink, output_type, &state_tmp, NULL);
  *state = (state_tmp == 0) ? (AUDIO_OUTPUT_STATE_OFF) : (AUDIO_OUTPUT_STATE_ON);
  UMMS_DEBUG (" type = %d, mode = %d", type, state_tmp);
  return TRUE;
}

static void
audio_manager_interface_init (AudioManagerInterface *iface)
{
  AudioManagerInterfaceClass *klass = (AudioManagerInterfaceClass *)iface;

  audio_manager_interface_implement_set_volume (klass,
      audio_manager_engine_set_volume);
  audio_manager_interface_implement_get_volume (klass,
      audio_manager_engine_get_volume);
  audio_manager_interface_implement_set_state (klass,
      audio_manager_engine_set_state);
  audio_manager_interface_implement_get_state (klass,
      audio_manager_engine_get_state);
}

static void
audio_manager_engine_get_property (GObject    *object,
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
audio_manager_engine_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
audio_manager_engine_dispose (GObject *object)
{
  AudioManagerEnginePrivate *priv = GET_PRIVATE (object);
  TEARDOWN_ELEMENT(priv->asink);

  G_OBJECT_CLASS (audio_manager_engine_parent_class)->dispose (object);
}

static void
audio_manager_engine_finalize (GObject *object)
{
  AudioManagerEnginePrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (audio_manager_engine_parent_class)->finalize (object);
}

static void
audio_manager_engine_class_init (AudioManagerEngineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AudioManagerEnginePrivate));

  object_class->get_property = audio_manager_engine_get_property;
  object_class->set_property = audio_manager_engine_set_property;
  object_class->dispose = audio_manager_engine_dispose;
  object_class->finalize = audio_manager_engine_finalize;
}

static void
audio_manager_engine_init (AudioManagerEngine *self)
{
  AudioManagerEnginePrivate *priv;
  GstStateChangeReturn ret;
  GstState state = 0;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;
  priv->asink = gst_element_factory_make ("ismd_audio_sink", NULL);
  if (!priv->asink) {
    UMMS_DEBUG ("Creating ismd_audio_sink failed");
    return;
  }
  if ((ret = gst_element_set_state (priv->asink, GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Setting ismd_audio_sink to playing failed");
    goto FAILED;
  }
  UMMS_DEBUG ("gst_element_set_state to asink,  ret = %d", ret);

  //Should immediately return.
  ret = gst_element_get_state (priv->asink, &state, NULL, 0);
  UMMS_DEBUG ("get audio sink state = %d, ret = %d", state, ret);
  switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
      UMMS_DEBUG ("ismd_audio_sink failed to go playing");
      goto FAILED;
      break;
    case GST_STATE_CHANGE_SUCCESS:
      UMMS_DEBUG ("Setting ismd_audio_sink to playing succeed");
      return;
      break;
    default:
      UMMS_DEBUG ("Should not reach here, strange!");
      goto FAILED;
      break;
  }

FAILED:
  TEARDOWN_ELEMENT (priv->asink);
  return;
}

AudioManagerEngine *
audio_manager_engine_new (void)
{
  return g_object_new (AUDIO_TYPE_MANAGER_ENGINE, NULL);
}

