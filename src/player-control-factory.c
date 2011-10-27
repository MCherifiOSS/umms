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

#include "umms-debug.h"
#include "umms-common.h"
#include "player-control-factory.h"
#include "media-player-control.h"
#include "player-control-tv.h"
#include "player-control-generic.h"
#include "engine-dvb-generic.h"
#include "dvb-player.h"


/*
 * G_DEFINE_TYPE (PlayerControlFactory, player_control_factory, TYPE_MEDIA_PLAYER)
 */
G_DEFINE_TYPE (PlayerControlFactory, player_control_factory, G_TYPE_OBJECT)

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PLAYER_CONTROL_TYPE_FACTORY, PlayerControlFactoryPrivate))

#define GET_PRIVATE(o) ((PlayerControlFactory *)o)->priv

enum EngineType {
  PLAYER_CONTROL_FACTORY_ENGINE_TYPE_INVALID,
  PLAYER_CONTROL_FACTORY_ENGINE_TYPE_NORMAL,
  PLAYER_CONTROL_FACTORY_ENGINE_TYPE_DVB,
  N_PLAYER_CONTROL_FACTORY_ENGINE_TYPE
};

struct _PlayerControlFactoryPrivate {
  enum EngineType engine_type;
  PlatformType platform_type;
};

//implement load_engine vmethod
#define TV_PREFIX "dvb:"

enum {
  PROP_0,
  PROP_NAME,
  PROP_ATTENDED,
  PROP_PLATFORM,
  PROP_LAST
};


/*
 * fake engine 
 *
 */
MediaPlayerControl* player_control_fake_new(void)
{

  UMMS_DEBUG("fake player_control error, is not supportted\n");
  return NULL;
}

/* [platform][engine]
 *  
 * platform: 0-> CETV
 *           1-> NETBOOK
 *
 * engine : 1-> Normal
 *          2-> DVB
 *
 */

typedef MediaPlayerControl* (*func)(void);

MediaPlayerControl* (*engine_factory[PLATFORM_INVALID][N_PLAYER_CONTROL_FACTORY_ENGINE_TYPE])(void) =
{
/*00*/player_control_fake_new, 
/*01: CETV-Normal*/ (func)player_control_tv_new,
/*02: CETV-DVB*/(func)dvb_player_new,
/*10: */NULL,
/*11: Netbook->Normal*/ (func)player_control_generic_new,
/*12: Netbook->DVB*/ (func)engine_dvb_generic_new
};

MediaPlayerControl *
create_engine (gint engine_type, PlatformType platform)
{
  MediaPlayerControl *engine = NULL;
  UMMS_DEBUG ("Trying to create engine type (%d) on platform (%d)", engine_type, platform);

  g_return_val_if_fail((engine_type < N_PLAYER_CONTROL_FACTORY_ENGINE_TYPE), NULL); 
  g_return_val_if_fail((platform < PLATFORM_INVALID), NULL); 
  g_return_val_if_fail(( engine_factory[platform][engine_type] != NULL), NULL); 

  engine = engine_factory[platform][engine_type]();

  g_return_val_if_fail((engine != NULL), NULL);

#if 0
  g_print("addr of engien: %x\n", engine_factory[platform][engine_type]);
  if (engine_type == PLAYER_CONTROL_FACTORY_ENGINE_TYPE_DVB)
    engine = dvb_player_new();
  else if (engine_type == PLAYER_CONTROL_FACTORY_ENGINE_TYPE_NORMAL)
    engine = engine_gst_new();
  else {
    UMMS_DEBUG ("Unknown engine type (%d)", engine_type);
  }
#endif

  return engine;
}

/* extend API to support multi-platform and multi-engine
 *
 */
static MediaPlayerControl* player_control_factory_load_engine (PlayerControlFactory *self, const char *uri, gboolean *new_engine)
{
  gboolean updated;
  gint type;
  PlayerControlFactoryPrivate *priv = NULL;
  MediaPlayerControl *player_control = NULL;

  g_return_val_if_fail (IS_PLAYER_CONTROL_FACTORY(self), FALSE);
  g_return_val_if_fail (uri, FALSE);

  priv = self->priv;
  type = (g_str_has_prefix (uri, TV_PREFIX)) ? (PLAYER_CONTROL_FACTORY_ENGINE_TYPE_DVB): (PLAYER_CONTROL_FACTORY_ENGINE_TYPE_NORMAL);

  if (priv->engine_type == type) {
    UMMS_DEBUG ("Already loaded engine(type = %d), no need to load again", type);
    updated = FALSE;
  } else {
    UMMS_DEBUG ("Changing engine from type(%d) to type(%d)", priv->engine_type, type);
    player_control = create_engine (type, priv->platform_type);

    if(!player_control){
      UMMS_DEBUG ("Loading engine failed");
      return player_control;
    }
    updated = TRUE;
  }

  priv->engine_type = type;
  *new_engine = updated;

  return player_control;
}

#if 0
static gboolean player_control_factory_load_engine (PlayerControl *player, const char *uri, gboolean *new_engine)
{
  gboolean ret;
  gboolean updated;
  gint type;
  PlayerControlFactoryPrivate *priv;

  g_return_val_if_fail (IS_PLAYER_CONTROL_FACTORY(player), FALSE);
  g_return_val_if_fail (uri, FALSE);

  priv = GET_PRIVATE(player);
  type = (g_str_has_prefix (uri, TV_PREFIX)) ? (PLAYER_CONTROL_FACTORY_ENGINE_TYPE_DVB): (PLAYER_CONTROL_FACTORY_ENGINE_TYPE_NORMAL);

  if (!player->player_control) {
    UMMS_DEBUG ("We have no engine loaded, to load one(type = %d)", type);
    player->player_control = create_engine (type,priv->platform_type);
      updated = TRUE;
  } else {
    if (priv->engine_type == type) {
      UMMS_DEBUG ("Already loaded engine(type = %d), no need to load again", type);
      updated = FALSE;
    } else {
      UMMS_DEBUG ("Changing engine from type(%d) to type(%d)", priv->engine_type, type);
      g_object_unref (player->player_control);
      player->player_control = create_engine (type, priv->platform_type);
      updated = TRUE;
    }
  }

  if (player->player_control) {
    priv->engine_type = type;
    *new_engine = updated;
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Loading engine failed");
    ret = FALSE;
  }

  return ret;
}
#endif


static void
player_control_factory_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  PlayerControlFactoryPrivate *priv = GET_PRIVATE (object);
  gint tmp;
  switch (property_id) {
    case PROP_PLATFORM:
      priv->platform_type = g_value_get_int(value);
      UMMS_DEBUG("platform type: %d", tmp);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
player_control_factory_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  PlayerControlFactoryPrivate *priv = GET_PRIVATE (object);
  gint tmp;
  switch (property_id) {
    case PROP_PLATFORM:
      g_value_set_int(value, priv->platform_type);
      UMMS_DEBUG("platform type: %d", priv->platform_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
player_control_factory_dispose (GObject *object)
{
  //PlayerControlFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (player_control_factory_parent_class)->dispose (object);
}

static void
player_control_factory_finalize (GObject *object)
{
  G_OBJECT_CLASS (player_control_factory_parent_class)->finalize (object);
}

static void
player_control_factory_class_init (PlayerControlFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
/*
  PlayerControlClass *p_class = PLAYER_CONTROL_CLASS (klass);
*/

  g_type_class_add_private (klass, sizeof (PlayerControlFactoryPrivate));

  object_class->get_property = player_control_factory_get_property;
  object_class->set_property = player_control_factory_set_property;
  object_class->dispose = player_control_factory_dispose;
  object_class->finalize = player_control_factory_finalize;
  klass->load_engine = player_control_factory_load_engine;

/*
  p_class->load_engine = player_control_factory_load_engine;
*/

  g_object_class_install_property (object_class, PROP_PLATFORM,
      g_param_spec_int ("platform", "platform type", "indication for platform type: Tv, netbook, etc",0,PLATFORM_INVALID,
          CETV, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
player_control_factory_init (PlayerControlFactory *self)
{
  PlayerControlFactoryPrivate *priv;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->engine_type = PLAYER_CONTROL_FACTORY_ENGINE_TYPE_INVALID;

}

PlayerControlFactory *
player_control_factory_new (void)
{
  return g_object_new (PLAYER_CONTROL_TYPE_FACTORY, NULL);
}



