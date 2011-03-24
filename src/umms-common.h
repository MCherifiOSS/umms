#ifndef _MTV_PLAYER_COMMON_H
#define _MTV_PLAYER_COMMON_H

#define MTV_PLAYER_OBJECT_PATH "/com/meego/UMMS"
#define MTV_PLAYER_SERVICE_NAME "com.meego.UMMS"
#define MTV_PLAYER_INTERFACE_NAME "com.meego.UMMS.MediaPlayer"

#define UMMS_DEBUG(x...) g_debug (G_STRLOC ": "x)

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



#endif /* _MTV_PLAYER_COMMON_H */

