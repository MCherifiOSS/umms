#ifndef _UMMS_GTK_BACKEND_H_
#include <glib-object.h>

gboolean umms_backend_resume (void);

gboolean umms_backend_pause (void);

gboolean umms_backend_reset (void);

gboolean umms_backend_seek (void);

gboolean umms_backend_seek_relative (gint to_seek);

gboolean umms_backend_seek_absolute (gint to_seek);

gint umms_backend_query_duration (void);

gint umms_backend_query_position (void);

#endif
