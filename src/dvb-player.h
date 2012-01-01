#ifndef _DVB_PLAYER_H
#define _DVB_PLAYER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DVB_TYPE_PLAYER dvb_player_get_type()

#define DVB_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DVB_TYPE_PLAYER, DvbPlayer))

#define DVB_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DVB_TYPE_PLAYER, DvbPlayerClass))

#define DVB_IS_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DVB_TYPE_PLAYER))

#define DVB_IS_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DVB_TYPE_PLAYER))

#define DVB_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DVB_TYPE_PLAYER, DvbPlayerClass))

typedef struct _DvbPlayer DvbPlayer;
typedef struct _DvbPlayerClass DvbPlayerClass;
typedef struct _DvbPlayerPrivate DvbPlayerPrivate;

struct _DvbPlayer
{
  GObject parent;

  DvbPlayerPrivate *priv;
};


struct _DvbPlayerClass
{
  GObjectClass parent_class;

};

GType dvb_player_get_type (void) G_GNUC_CONST;

DvbPlayer *dvb_player_new (void);

G_END_DECLS

#endif /* _DVB_PLAYER_H */
