#ifndef _UMMS_COMMON_H
#define _UMMS_COMMON_H

#include "umms-debug.h"
#define UMMS_SERVICE_NAME "com.meego.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/meego/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.meego.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.meego.UMMS.MediaPlayer"

//#define UMMS_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)

#define return_val_and_log_if_fail(val_to_check, ret_val, msg) \
  do {\
    if (val_to_check == NULL) {\
      UMMS_DEBUG (msg);\
      return ret_val;\
    }\
  }while(0)


typedef enum {
    ErrorTypeEngine,
    NumOfErrorType
}ErrorType;

typedef enum {
    PlayerStateNull,
    PlayerStatePaused,
    PlayerStatePlaying,
    PlayerStateStopped,
    NumOfPlayerState
}PlayerState;



#endif /* _UMMS_COMMON_H */

