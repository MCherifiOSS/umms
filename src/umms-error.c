#include <glib.h>


#define QUARK_FUNC(string)                                              \
  GQuark umms_ ## string ## _error_quark (void) {                          \
    static GQuark quark;                                                  \
    if (!quark)                                                           \
    quark = g_quark_from_static_string ("umms-" # string "-error-quark"); \
    return quark; }

QUARK_FUNC(engine);
QUARK_FUNC(resource);
QUARK_FUNC(generic);
