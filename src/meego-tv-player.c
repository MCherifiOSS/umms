
#include "meego-tv-player.h"
#include "meego-media-player-control-interface.h"
#include "engine-gst.h"


G_DEFINE_TYPE (MeegoTvPlayer, meego_tv_player, MEEGO_TYPE_MEDIA_PLAYER)
#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_TV_PLAYER, MeegoTvPlayerPrivate))

#define GET_PRIVATE(o) ((MeegoTvPlayer *)o)->priv

#define MEEGO_TV_PLAYER_DEBUG(x...) g_debug (G_STRLOC ": "x)
#define TICK_TIMEOUT 500

enum EngineType{
      MEEGO_TV_PLAYER_ENGINE_TYPE_INVALID,
      MEEGO_TV_PLAYER_ENGINE_TYPE_GST,
      MEEGO_TV_PLAYER_ENGINE_TYPE_TV,
      N_MEEGO_TV_PLAYER_ENGINE_TYPE
};

struct _MeegoTvPlayerPrivate
{
  enum EngineType engine_type;
};

//implement load_engine vmethod
#define TV_PREFIX "dvb:"

static gboolean meego_tv_player_load_engine (MeegoMediaPlayer *player, const char *uri, gboolean *new_engine)
{
    MeegoTvPlayerPrivate *priv = ((MeegoTvPlayer *)player)->priv;
    gboolean ret = TRUE;

    g_return_val_if_fail (uri, FALSE);

    MEEGO_TV_PLAYER_DEBUG ("uri = %s", uri);

    if (g_str_has_prefix (uri, TV_PREFIX)) {
        ret = FALSE; //TODO:implement TV Engine other place and create TV engine here. 
        MEEGO_TV_PLAYER_DEBUG ("not support dvb:// source type, tv engine not implemented");
    } else {
        if (priv->engine_type == MEEGO_TV_PLAYER_ENGINE_TYPE_GST) {
            //we already loaded gstreamer engine.
            MEEGO_TV_PLAYER_DEBUG ("gstreamer engine alrady loaded");
            *new_engine = FALSE;
            ret = TRUE;
        } else if (priv->engine_type == MEEGO_TV_PLAYER_ENGINE_TYPE_INVALID){
            MEEGO_TV_PLAYER_DEBUG ("load gstreamer engine");
            player->player_control = (MeegoMediaPlayerControlInterface *)engine_gst_new();
            if (player->player_control) {
                priv->engine_type = MEEGO_TV_PLAYER_ENGINE_TYPE_GST;
                *new_engine = TRUE;
                ret = TRUE;
            } else {
                ret = FALSE;
            }
        } else {
            //TODO:
            //tv engine had been loaded, destroy it and load gstreamer engine.
        }
    }

    return ret;
}


static void
meego_tv_player_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
meego_tv_player_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
meego_tv_player_dispose (GObject *object)
{
  //MeegoTvPlayerPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (meego_tv_player_parent_class)->dispose (object);
}

static void
meego_tv_player_finalize (GObject *object)
{
  G_OBJECT_CLASS (meego_tv_player_parent_class)->finalize (object);
}

static void
meego_tv_player_class_init (MeegoTvPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  MeegoMediaPlayerClass *p_class = MEEGO_MEDIA_PLAYER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MeegoTvPlayerPrivate));

  object_class->get_property = meego_tv_player_get_property;
  object_class->set_property = meego_tv_player_set_property;
  object_class->dispose = meego_tv_player_dispose;
  object_class->finalize = meego_tv_player_finalize;

  p_class->load_engine = meego_tv_player_load_engine;
}

static void
meego_tv_player_init (MeegoTvPlayer *self)
{
  MeegoTvPlayerPrivate *priv;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  priv->engine_type = MEEGO_TV_PLAYER_ENGINE_TYPE_INVALID;


}

MeegoTvPlayer *
meego_tv_player_new (void)
{
  return g_object_new (MEEGO_TYPE_TV_PLAYER, NULL);
}




