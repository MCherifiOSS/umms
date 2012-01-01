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
#include "meego-media-player-gstreamer.h"
#include "meego-media-player-control.h"
#include "engine-gst.h"


G_DEFINE_TYPE (MeegoMediaPlayerGstreamer, meego_media_player_gstreamer, MEEGO_TYPE_MEDIA_PLAYER)
#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamerPrivate))

#define GET_PRIVATE(o) ((MeegoMediaPlayerGstreamer *)o)->priv

enum EngineType {
  MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_INVALID,
  MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_NORMAL,
  MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_DVB,
  N_MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE
};

struct _MeegoMediaPlayerGstreamerPrivate {
  enum EngineType engine_type;
};

//implement load_engine vmethod
#define TV_PREFIX "dvb:"

MeegoMediaPlayerControl *
create_engine (gint type)
{
  MeegoMediaPlayerControl *engine = NULL;

  if (type == MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_DVB)
    engine = dvb_player_new();
  else if (type == MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_NORMAL)
    engine = engine_gst_new();
  else 
    UMMS_DEBUG ("Unknown engine type (%d)", type);

  return engine;
}

static gboolean meego_media_player_gstreamer_load_engine (MeegoMediaPlayer *player, const char *uri, gboolean *new_engine)
{
  gboolean ret;
  gboolean updated;
  gint type;
  MeegoMediaPlayerGstreamerPrivate *priv;

  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_GSTREAMER(player), FALSE);
  g_return_val_if_fail (uri, FALSE);

  priv = GET_PRIVATE(player);
  type = (g_str_has_prefix (uri, TV_PREFIX)) ? (MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_DVB): (MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_NORMAL);

  if (!player->player_control) {
    UMMS_DEBUG ("We have no engine loaded, to load one(type = %d)", type);
    player->player_control = create_engine (type);
      updated = TRUE;
  } else {
    if (priv->engine_type == type) {
      UMMS_DEBUG ("Already loaded engine(type = %d), no need to load again", type);
      updated = FALSE;
    } else {
      UMMS_DEBUG ("Changing engine from type(%d) to type(%d)", priv->engine_type, type);
      g_object_unref (player->player_control);
      player->player_control = create_engine (type);
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
meego_media_player_gstreamer_get_property (GObject    *object,
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
meego_media_player_gstreamer_set_property (GObject      *object,
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
meego_media_player_gstreamer_dispose (GObject *object)
{
  //MeegoMediaPlayerGstreamerPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (meego_media_player_gstreamer_parent_class)->dispose (object);
}

static void
meego_media_player_gstreamer_finalize (GObject *object)
{
  G_OBJECT_CLASS (meego_media_player_gstreamer_parent_class)->finalize (object);
}

static void
meego_media_player_gstreamer_class_init (MeegoMediaPlayerGstreamerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MeegoMediaPlayerClass *p_class = MEEGO_MEDIA_PLAYER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MeegoMediaPlayerGstreamerPrivate));

  object_class->get_property = meego_media_player_gstreamer_get_property;
  object_class->set_property = meego_media_player_gstreamer_set_property;
  object_class->dispose = meego_media_player_gstreamer_dispose;
  object_class->finalize = meego_media_player_gstreamer_finalize;

  p_class->load_engine = meego_media_player_gstreamer_load_engine;
}

static void
meego_media_player_gstreamer_init (MeegoMediaPlayerGstreamer *self)
{
  MeegoMediaPlayerGstreamerPrivate *priv;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->engine_type = MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_INVALID;
}

MeegoMediaPlayerGstreamer *
meego_media_player_gstreamer_new (void)
{
  return g_object_new (MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, NULL);
}




