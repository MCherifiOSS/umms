#ifndef _UMMS_COMMON_H
#define _UMMS_COMMON_H

#define UMMS_SERVICE_NAME "com.meego.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/meego/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.meego.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.meego.UMMS.MediaPlayer"

//#define UMMS_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)


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

