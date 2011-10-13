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

#include <gst/gst.h>

#include "umms-debug.h"
#include "umms-common.h"
#include "umms-error.h"
#include "umms-marshals.h"
#include "umms-audio-manager.h"
#include "audio-manager-interface.h"
#include "audio-manager-engine.h"

G_DEFINE_TYPE (UmmsAudioManager, umms_audio_manager, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_AUDIO_MANAGER, UmmsAudioManagerPrivate))
#define GET_PRIVATE(o) ((UmmsAudioManager *)o)->priv

struct _UmmsAudioManagerPrivate {
  AudioManagerInterface *audio_mngr_if;
  PlatformType platform_type;
};

/*property for audio manager*/
enum PROPTYPE{
  PROP_0,
  PROP_PLATFORM,
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
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (object);
  gint tmp;
  switch (property_id) {
    case PROP_PLATFORM:
      tmp = g_value_get_int(value);
      priv->platform_type = tmp;
      UMMS_DEBUG("platform type: %d", tmp);
      break;
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

static UmmsAudioManager *the_only = NULL;

/* define constructor to imple:
 * Some people sometimes need to construct their object but only after the 
 * construction properties have been set. This is possible through the use
 * of the constructor class method
 * Ori: ClassInit -> Init ->SetProperty
 * New: ClassInit -> Constructor -> ParentConstructor-> Init->SetProperty->DoSomethingYourSelf
 */
static GObject*
umms_audio_manager_constructor (GType                  type,
                          guint                  n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  UMMS_DEBUG (" ");
  GObject *obj;
  if (the_only){
    obj = g_object_ref (the_only);
  }else{
    obj = G_OBJECT_CLASS (umms_audio_manager_parent_class)->constructor (type, n_construct_properties, construct_properties);
  }

  /*do something based on property setting.*/
  /*construct obj based on property*/
  UmmsAudioManagerPrivate *priv = GET_PRIVATE (obj);

  if(priv->platform_type == NETBOOK){
    UMMS_DEBUG("NetBook:NO Audio Set");
    //player->priv->audio_mngr_if = AUDIO_MANAGER_INTERFACE(audio_manager_engine_new ());//TODO:create audio manager interface instance here.
  }else{
    priv->audio_mngr_if = AUDIO_MANAGER_INTERFACE(audio_manager_engine_new ());//TODO:create audio manager interface instance here.
  }
  return obj;
}

static void
umms_audio_manager_class_init (UmmsAudioManagerClass *klass)
{
  UMMS_DEBUG (" ");
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsAudioManagerPrivate));

  object_class->get_property = umms_audio_manager_get_property;
  object_class->set_property = umms_audio_manager_set_property;
  object_class->dispose = umms_audio_manager_dispose;
  object_class->finalize = umms_audio_manager_finalize;
  object_class->constructor = umms_audio_manager_constructor;

  /*set property : platform type*/
  g_object_class_install_property (object_class, PROP_PLATFORM,
      g_param_spec_int ("platform", "platform type", "indication for platform type: Tv, netbook, etc",0,PLATFORM_INVALID,
          CETV, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
umms_audio_manager_init (UmmsAudioManager *player)
{
  UMMS_DEBUG (" ");
  g_assert (the_only == NULL);
  the_only = player;
  player->priv = PLAYER_PRIVATE (player);
#if 0
  UMMS_DEBUG ("Type : %d", player->priv->platform_type);
  if(player->priv->platform_type == NETBOOK){
    UMMS_DEBUG("Do not Init Audio Sink On NetBook");
    //player->priv->audio_mngr_if = AUDIO_MANAGER_INTERFACE(audio_manager_engine_new ());//TODO:create audio manager interface instance here.
  }else{
    player->priv->audio_mngr_if = AUDIO_MANAGER_INTERFACE(audio_manager_engine_new ());//TODO:create audio manager interface instance here.
  }
#endif
}

UmmsAudioManager *
umms_audio_manager_new (gint platform)
{
  return g_object_new (UMMS_TYPE_AUDIO_MANAGER, "platform", platform,NULL);
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
