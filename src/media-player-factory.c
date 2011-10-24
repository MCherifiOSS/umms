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
#include "media-player-factory.h"
#include "media-player-control.h"
#include "engine-gst.h"
#include "engine-generic.h"
#include "engine-dvb-generic.h"
#include "dvb-player.h"


G_DEFINE_TYPE (MediaPlayerFactory, media_player_factory, TYPE_MEDIA_PLAYER)
#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_MEDIA_PLAYER_FACTORY, MediaPlayerFactoryPrivate))

#define GET_PRIVATE(o) ((MediaPlayerFactory *)o)->priv

enum EngineType {
  MEDIA_PLAYER_FACTORY_ENGINE_TYPE_INVALID,
  MEDIA_PLAYER_FACTORY_ENGINE_TYPE_NORMAL,
  MEDIA_PLAYER_FACTORY_ENGINE_TYPE_DVB,
  N_MEDIA_PLAYER_FACTORY_ENGINE_TYPE
};

struct _MediaPlayerFactoryPrivate {
  enum EngineType engine_type;
  PlatformType platform_type;
};

//implement load_engine vmethod
#define TV_PREFIX "dvb:"

/*property for object manager*/
enum PROPTYPE{
  PROP_0,
  PROP_PLATFORM,
  PROP_LAST
};

/*
 * fake engine 
 *
 */
MediaPlayerControl* engine_fake_new(void)
{

  UMMS_DEBUG("fake engine error, is not supportted\n");
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

MediaPlayerControl* (*engine_factory[PLATFORM_INVALID][N_MEDIA_PLAYER_FACTORY_ENGINE_TYPE])(void) =
{
/*00*/engine_fake_new, 
/*01: CETV-Normal*/ engine_gst_new,
/*02: CETV-DVB*/dvb_player_new,
/*10: */NULL,
/*11: Netbook->Normal*/ engine_generic_new,
/*12: Netbook->DVB*/ engine_dvb_generic_new
};

MediaPlayerControl *
create_engine (gint engine_type, PlatformType platform)
{
  MediaPlayerControl *engine = NULL;
  UMMS_DEBUG ("Trying to create engine type (%d) on platform (%d)", engine_type, platform);

  g_return_val_if_fail((engine_type < N_MEDIA_PLAYER_FACTORY_ENGINE_TYPE), NULL); 
  g_return_val_if_fail((platform < PLATFORM_INVALID), NULL); 
  g_return_val_if_fail(( engine_factory[platform][engine_type] != NULL), NULL); 

  engine = engine_factory[platform][engine_type]();

  g_return_val_if_fail((engine != NULL), NULL);

#if 0
  g_print("addr of engien: %x\n", engine_factory[platform][engine_type]);
  if (engine_type == MEDIA_PLAYER_FACTORY_ENGINE_TYPE_DVB)
    engine = dvb_player_new();
  else if (engine_type == MEDIA_PLAYER_FACTORY_ENGINE_TYPE_NORMAL)
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

static gboolean media_player_factory_load_engine (MediaPlayer *player, const char *uri, gboolean *new_engine)
{
  gboolean ret;
  gboolean updated;
  gint type;
  MediaPlayerFactoryPrivate *priv;

  g_return_val_if_fail (IS_MEDIA_PLAYER_FACTORY(player), FALSE);
  g_return_val_if_fail (uri, FALSE);

  priv = GET_PRIVATE(player);
  type = (g_str_has_prefix (uri, TV_PREFIX)) ? (MEDIA_PLAYER_FACTORY_ENGINE_TYPE_DVB): (MEDIA_PLAYER_FACTORY_ENGINE_TYPE_NORMAL);

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


static void
media_player_factory_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  MediaPlayerFactoryPrivate *priv = GET_PRIVATE (object);
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
media_player_factory_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  MediaPlayerFactoryPrivate *priv = GET_PRIVATE (object);
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
media_player_factory_dispose (GObject *object)
{
  //MediaPlayerFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (media_player_factory_parent_class)->dispose (object);
}

static void
media_player_factory_finalize (GObject *object)
{
  G_OBJECT_CLASS (media_player_factory_parent_class)->finalize (object);
}

static void
media_player_factory_class_init (MediaPlayerFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MediaPlayerClass *p_class = MEDIA_PLAYER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MediaPlayerFactoryPrivate));

  object_class->get_property = media_player_factory_get_property;
  object_class->set_property = media_player_factory_set_property;
  object_class->dispose = media_player_factory_dispose;
  object_class->finalize = media_player_factory_finalize;

  p_class->load_engine = media_player_factory_load_engine;

  g_object_class_install_property (object_class, PROP_PLATFORM,
      g_param_spec_int ("platform", "platform type", "indication for platform type: Tv, netbook, etc",0,PLATFORM_INVALID,
          CETV, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
media_player_factory_init (MediaPlayerFactory *self)
{
  MediaPlayerFactoryPrivate *priv;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->engine_type = MEDIA_PLAYER_FACTORY_ENGINE_TYPE_INVALID;
}

MediaPlayerFactory *
media_player_factory_new (void)
{
  return g_object_new (TYPE_MEDIA_PLAYER_FACTORY, NULL);
}



