#ifndef _MEEGO_MEDIA_PLAYER_GSTREAMER_H
#define _MEEGO_MEDIA_PLAYER_GSTREAMER_H

#include <glib-object.h>
#include <meego-media-player.h>

G_BEGIN_DECLS

#define MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER meego_media_player_gstreamer_get_type()

#define MEEGO_MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamer))

#define MEEGO_MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamerClass))

#define MEEGO_IS_MEDIA_PLAYER_GSTREAMER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER))

#define MEEGO_IS_MEDIA_PLAYER_GSTREAMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER))

#define MEEGO_MEDIA_PLAYER_GSTREAMER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER, MeegoMediaPlayerGstreamerClass))

typedef struct _MeegoMediaPlayerGstreamer MeegoMediaPlayerGstreamer;
typedef struct _MeegoMediaPlayerGstreamerClass MeegoMediaPlayerGstreamerClass;
typedef struct _MeegoMediaPlayerGstreamerPrivate MeegoMediaPlayerGstreamerPrivate;

struct _MeegoMediaPlayerGstreamer
{
  MeegoMediaPlayer parent;

  MeegoMediaPlayerGstreamerPrivate *priv;
};


struct _MeegoMediaPlayerGstreamerClass
{
  MeegoMediaPlayerClass parent_class;

};

GType meego_media_player_gstreamer_get_type (void) G_GNUC_CONST;

MeegoMediaPlayerGstreamer *meego_media_player_gstreamer_new (void);

G_END_DECLS

#endif /* _MEEGO_MEDIA_PLAYER_GSTREAMER_H */
