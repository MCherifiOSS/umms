#ifndef _UMMS_COMMON_H
#define _UMMS_COMMON_H

#define UMMS_SERVICE_NAME "com.meego.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/meego/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.meego.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.meego.UMMS.MediaPlayer"

typedef enum {
    PlayerStateNull,
    PlayerStatePaused,
    PlayerStatePlaying,
    PlayerStateStopped,
    NumOfPlayerState
}PlayerState;

typedef enum {
    XWindow,
    DataCopy,
    Socket,
    ReservedType0,//On CE4100 platform, indicate using gdl plane directly to render video data 
    ReservedType1,
    ReservedType2,
    ReservedType3
}TargetType;

//CE4100 specific
typedef enum {
  UPP_A = 4,
  UPP_B = 5,
  UPP_C = 6
}GdlPlaneId;

#define TARGET_PARAM_KEY_RECTANGLE "rectangle"
#define TARGET_PARAM_KEY_PlANE_ID "plane-id"

#endif /* _UMMS_COMMON_H */

