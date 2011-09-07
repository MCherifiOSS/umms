#ifndef _UMMS_COMMON_H
#define _UMMS_COMMON_H

#define UMMS_SERVICE_NAME "com.meego.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/meego/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.meego.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.meego.UMMS.MediaPlayer"

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH "/com/meego/UMMS/PlayingContentMetadataViewer"
#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_INTERFACE_NAME "com.meego.UMMS.PlayingContentMetadataViewer"

#define UMMS_AUDIO_MANAGER_OBJECT_PATH "/com/meego/UMMS/AudioManager"
#define UMMS_AUDIO_MANAGER_INTERFACE_NAME "com.meego.UMMS.AudioManger"


#define RESET_STR(str) \
     if (str) {       \
       g_free(str);   \
       str = NULL;    \
     }

typedef enum {
    PlayerStateNull,
    PlayerStateStopped,
    PlayerStatePaused,
    PlayerStatePlaying
}PlayerState;

typedef enum {
    TargetTypeInvalid = -1,
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

typedef enum {
  BufferFormatInvalid = -1,
  BufferFormatByTime,
  BufferFormatByBytes
}BufferFormat;

typedef enum {    
  ScaleModeInvalid = -1,
  ScaleModeNoScale,             /*  Do nothing, just leave the output x, y as the input. */ 
  ScaleModeFill,                /*  fill entire target, regardless of original video aspect ratio. */
  ScaleModeKeepAspectRatio,     /*  respect the original video aspect ratio. */
  ScaleModeFillKeepAspectRatio, /*  fill the whole screen but keep the ratio, some part of video may out of screen.*/ 
}ScaleMode;

#define TARGET_PARAM_KEY_RECTANGLE "rectangle"
#define TARGET_PARAM_KEY_PlANE_ID "plane-id"

typedef enum {
  AUDIO_OUTPUT_TYPE_HDMI,
  AUDIO_OUTPUT_TYPE_SPDIF,
  AUDIO_OUTPUT_TYPE_I2S0,
  AUDIO_OUTPUT_TYPE_I2S1,
  AUDIO_OUTPUT_TYPE_NUM
}AudioOutputType;

enum {
 AUDIO_OUTPUT_STATE_OFF,
 AUDIO_OUTPUT_STATE_ON,
 AUDIO_OUTPUT_STATE_NUM
}AudioOutputState;

#endif /* _UMMS_COMMON_H */

