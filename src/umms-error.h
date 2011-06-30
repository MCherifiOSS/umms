#ifndef _UMMS_ERROR_H
#define _UMMS_ERROR_H
#include <glib.h>

typedef enum {
    UMMS_ENGINE_ERROR_NOT_LOADED,
    UMMS_ENGINE_ERROR_FAILED
}UmmsEngineError;

#define UMMS_ENGINE_ERROR umms_engine_error_quark()


#endif /* _UMMS_ERROR_H */

