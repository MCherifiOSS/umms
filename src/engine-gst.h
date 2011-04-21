#ifndef _ENGINE_GST_H
#define _ENGINE_GST_H

#include <glib-object.h>

G_BEGIN_DECLS

#define ENGINE_TYPE_GST engine_gst_get_type()

#define ENGINE_GST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  ENGINE_TYPE_GST, EngineGst))

#define ENGINE_GST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  ENGINE_TYPE_GST, EngineGstClass))

#define ENGINE_IS_GST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  ENGINE_TYPE_GST))

#define ENGINE_IS_GST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  ENGINE_TYPE_GST))

#define ENGINE_GST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  ENGINE_TYPE_GST, EngineGstClass))

typedef struct _EngineGst EngineGst;
typedef struct _EngineGstClass EngineGstClass;
typedef struct _EngineGstPrivate EngineGstPrivate;

struct _EngineGst
{
  GObject parent;

  EngineGstPrivate *priv;
};


struct _EngineGstClass
{
  GObjectClass parent_class;

};

GType engine_gst_get_type (void) G_GNUC_CONST;

EngineGst *engine_gst_new (void);

G_END_DECLS

#endif /* _ENGINE_GST_H */