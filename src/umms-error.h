#ifndef _UMMS_ERROR_H
#define _UMMS_ERROR_H
#include <glib.h>

typedef enum {
    UMMS_ENGINE_ERROR_NOT_LOADED,
    UMMS_ENGINE_ERROR_INVALID_STATE,
    UMMS_ENGINE_ERROR_FAILED
}UmmsEngineError;

typedef enum {
    UMMS_RESOURCE_ERROR_NO_RESOURCE,
    UMMS_RESOURCE_ERROR_FAILED
}UmmsResourceError;


#define UMMS_ENGINE_ERROR umms_engine_error_quark()
#define UMMS_RESOURCE_ERROR umms_resource_error_quark()

#endif /* _UMMS_ERROR_H */

