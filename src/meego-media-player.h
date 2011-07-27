#ifndef _MEEGO_MEDIA_PLAYER_H
#define _MEEGO_MEDIA_PLAYER_H

#include <glib-object.h>
#include <meego-media-player-control.h>

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
  MeegoMediaPlayerControl *player_control;

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

gboolean meego_media_player_set_uri (MeegoMediaPlayer *self, const gchar *uri, GError **error);
gboolean meego_media_player_set_target (MeegoMediaPlayer *self, gint type, GHashTable *params, GError **error);
gboolean meego_media_player_play(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_pause(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_stop(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_set_position(MeegoMediaPlayer *self, gint64 pos, GError **error);
gboolean meego_media_player_get_position(MeegoMediaPlayer *self, gint64 *pos, GError **error);
gboolean meego_media_player_set_playback_rate(MeegoMediaPlayer *self, gdouble rate, GError **error);
gboolean meego_media_player_get_playback_rate(MeegoMediaPlayer *self, gdouble *rate, GError **error);
gboolean meego_media_player_set_volume(MeegoMediaPlayer *self, gint vol, GError **error);
gboolean meego_media_player_get_volume(MeegoMediaPlayer *self, gint *vol, GError **error);
gboolean meego_media_player_set_window_id(MeegoMediaPlayer *self, gdouble id, GError **error);
gboolean meego_media_player_set_video_size(MeegoMediaPlayer *self, guint x, guint y, guint w, guint h, GError **error);
gboolean meego_media_player_get_video_size(MeegoMediaPlayer *self, guint *w, guint *h, GError **error);
gboolean meego_media_player_get_buffered_time(MeegoMediaPlayer *self, gint64 *buffered_time, GError **error);
gboolean meego_media_player_get_buffered_bytes(MeegoMediaPlayer *self, gint64 *buffered_bytes, GError **error);
gboolean meego_media_player_get_media_size_time(MeegoMediaPlayer *self, gint64 *size_time, GError **error);
gboolean meego_media_player_get_media_size_bytes(MeegoMediaPlayer *self, gint64 *size_bytes, GError **error);
gboolean meego_media_player_has_video(MeegoMediaPlayer *self, gboolean *has_video, GError **error);
gboolean meego_media_player_has_audio(MeegoMediaPlayer *self, gboolean *has_audio, GError **error);
gboolean meego_media_player_is_streaming(MeegoMediaPlayer *self, gboolean *is_streaming, GError **error);
gboolean meego_media_player_is_seekable(MeegoMediaPlayer *self, gboolean *is_seekable, GError **error);
gboolean meego_media_player_support_fullscreen(MeegoMediaPlayer *self, gboolean *support_fullscreen, GError **error);
gboolean meego_media_player_get_player_state(MeegoMediaPlayer *self, gint *state, GError **error);
gboolean meego_media_player_reply(MeegoMediaPlayer *self, GError **error);
gboolean meego_media_player_set_proxy (MeegoMediaPlayer *self, GHashTable *params, GError **error);
gboolean meego_media_player_get_current_video (MeegoMediaPlayer *player, gint *cur_video, GError **err);
gboolean meego_media_player_get_current_audio (MeegoMediaPlayer *player, gint *cur_audio, GError **err);
gboolean meego_media_player_set_current_video (MeegoMediaPlayer *player, gint cur_video, GError **err);
gboolean meego_media_player_set_current_audio (MeegoMediaPlayer *player, gint cur_audio, GError **err);

G_END_DECLS

#endif /* _MEEGO_MEDIA_PLAYER_H */
