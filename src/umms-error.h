#ifndef _UMMS_ERROR_H
#define _UMMS_ERROR_H
#include <glib.h>

typedef enum {
    UMMS_ENGINE_ERROR_NOT_LOADED = 0,
    UMMS_ENGINE_ERROR_INVALID_STATE,
    UMMS_ENGINE_ERROR_FAILED,
    UMMS_RESOURCE_ERROR_NO_RESOURCE = 20,
    UMMS_RESOURCE_ERROR_FAILED,
    UMMS_GENERIC_ERROR_CREATING_OBJ_FAILED = 30,
    UMMS_AUDIO_ERROR_FAILED = 40
}UmmsErrorCode;


#define UMMS_ENGINE_ERROR umms_engine_error_quark()
#define UMMS_RESOURCE_ERROR umms_resource_error_quark()
#define UMMS_GENERIC_ERROR umms_generic_error_quark()
#define UMMS_AUDIO_ERROR umms_audio_error_quark()

#endif /* _UMMS_ERROR_H */

