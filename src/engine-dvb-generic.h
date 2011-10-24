#ifndef _ENGINE_DVB_GENERIC_H
#define _ENGINE_DVB_GENERIC_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DVB_TYPE_GPLAYER dvb_Gplayer_get_type()

#define DVB_GPLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DVB_TYPE_GPLAYER, DvbGPlayer))

#define DVB_GPLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DVB_TYPE_GPLAYER, DvbGPlayerClass))

#define DVB_IS_GPLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DVB_TYPE_GPLAYER))

#define DVB_IS_GPLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DVB_TYPE_GPLAYER))

#define DVB_GPLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DVB_TYPE_GPLAYER, DvbGPlayerClass))

typedef struct _DvbGPlayer DvbGPlayer;
typedef struct _DvbGPlayerClass DvbGPlayerClass;
typedef struct _DvbGPlayerPrivate DvbGPlayerPrivate;

struct _DvbGPlayer
{
  GObject parent;

  DvbGPlayerPrivate *priv;
};


struct _DvbGPlayerClass
{
  GObjectClass parent_class;

};


GType dvb_Gplayer_get_type (void) G_GNUC_CONST;

DvbGPlayer *engine_dvb_generic_new (void);

G_END_DECLS

#endif /* _ENGINE_DVB_GENERIC_H */
