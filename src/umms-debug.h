#ifndef _UMMS_DEBUG_H
#define _UMMS_DEBUG_H

#define UMMS_ENABLE_DEBUG 

#ifdef UMMS_ENABLE_DEBUG
#define UMMS_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)

#else
#define UMMS_DEBUG(format, ...)
#endif

#include <stdlib.h>
#define UMMS_GERROR(prefix, gerror) \
          do {\
                g_print (prefix ":%s", error->message); \
                g_error_free(gerror); \
                exit(1);\
          }while(0)

#endif /* _UMMS_DEBUG_H */

