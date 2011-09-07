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
  MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_GST,
  MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_TV,
  N_MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE
};

struct _MeegoMediaPlayerGstreamerPrivate {
  enum EngineType engine_type;
};

//implement load_engine vmethod
#define TV_PREFIX "dvb:"

static gboolean meego_media_player_gstreamer_load_engine (MeegoMediaPlayer *player, const char *uri, gboolean *new_engine)
{
  MeegoMediaPlayerGstreamerPrivate *priv = ((MeegoMediaPlayerGstreamer *)player)->priv;
  gboolean ret = TRUE;

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("uri = %s", uri);

  if (g_str_has_prefix (uri, TV_PREFIX)) {
    ret = FALSE; //TODO:implement TV Engine other place and create TV engine here.
    UMMS_DEBUG ("not support dvb:// source type, tv engine not implemented");
  } else {
    if (player->player_control) {
      //we already loaded gstreamer engine.
      UMMS_DEBUG ("gstreamer engine alrady loaded");
      *new_engine = FALSE;
      ret = TRUE;
    } else {
      UMMS_DEBUG ("load gstreamer engine");
      player->player_control = (MeegoMediaPlayerControl*)engine_gst_new();
      if (player->player_control) {
        priv->engine_type = MEEGO_MEDIA_PLAYER_GSTREAMER_ENGINE_TYPE_GST;
        *new_engine = TRUE;
        ret = TRUE;
      } else {
        ret = FALSE;
      }
    }
  }
  if (ret)
    ret = meego_media_player_control_set_uri (player->player_control, uri);

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




