#ifndef _MEEGO_MEDIA_PLAYER_H
#define _MEEGO_MEDIA_PLAYER_H

#include <glib-object.h>
#include <meego-media-player-control-interface.h>

G_BEGIN_DECLS

#define MEEGO_TYPE_MEDIA_PLAYER meego_media_player_get_type()

#define MEEGO_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayer))

#define MEEGO_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerClass))

#define MEEGO_IS_MEDIA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER))

#define MEEGO_IS_MEDIA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_MEDIA_PLAYER))

#define MEEGO_MEDIA_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_MEDIA_PLAYER, MeegoMediaPlayerClass))

typedef struct _MeegoMediaPlayer MeegoMediaPlayer;
typedef struct _MeegoMediaPlayerClass MeegoMediaPlayerClass;
typedef struct _MeegoMediaPlayerPrivate MeegoMediaPlayerPrivate;

struct _MeegoMediaPlayer
{
  GObject parent;
  MeegoMediaPlayerControlInterface *player_control;

  MeegoMediaPlayerPrivate *priv;
};


struct _MeegoMediaPlayerClass
{
    GObjectClass parent_class;

    /*
     *
     * self:            A MeegoMediaPlayer
     * uri:             Uri to play
     * new_engine[out]: TRUE if loaded a new engine and destroyed old engine
     *                  FALSE if not
     *
     * Returns:         TRUE if successful
     *                  FALSE if not
     *
     * Load engine according to uri prefix which indicates the source type, and store it in MeegoMediaPlayer::player_control.
     * Subclass should implement this vmethod to customize the procedure of backend engine loading.
     *          
     */
    gboolean (*load_engine) (MeegoMediaPlayer *self, const char *uri, gboolean *new_engine);

};

GType meego_media_player_get_type (void) G_GNUC_CONST;

MeegoMediaPlayer *meego_media_player_new (void);

G_END_DECLS

#endif /* _MEEGO_MEDIA_PLAYER_H */
