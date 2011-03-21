#ifndef _MEEGO_TV_PLAYER_H
#define _MEEGO_TV_PLAYER_H

#include <glib-object.h>
#include <meego-media-player.h>

G_BEGIN_DECLS

#define MEEGO_TYPE_TV_PLAYER meego_tv_player_get_type()

#define MEEGO_TV_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_TV_PLAYER, MeegoTvPlayer))

#define MEEGO_TV_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_TV_PLAYER, MeegoTvPlayerClass))

#define MEEGO_IS_TV_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_TV_PLAYER))

#define MEEGO_IS_TV_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_TV_PLAYER))

#define MEEGO_TV_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_TV_PLAYER, MeegoTvPlayerClass))

typedef struct _MeegoTvPlayer MeegoTvPlayer;
typedef struct _MeegoTvPlayerClass MeegoTvPlayerClass;
typedef struct _MeegoTvPlayerPrivate MeegoTvPlayerPrivate;

struct _MeegoTvPlayer
{
  MeegoMediaPlayer parent;

  MeegoTvPlayerPrivate *priv;
};


struct _MeegoTvPlayerClass
{
  MeegoMediaPlayerClass parent_class;

};

GType meego_tv_player_get_type (void) G_GNUC_CONST;

MeegoTvPlayer *meego_tv_player_new (void);

G_END_DECLS

#endif /* _MEEGO_TV_PLAYER_H */
