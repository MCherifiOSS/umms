/* 
 * UMMS (Unified Multi Media Service) provides a set of DBus APIs to support
 * playing Audio and Video as well as DVB playback.
 *
 * Authored by Zhiwen Wu <zhiwen.wu@intel.com>
 *             Junyan He <junyan.he@intel.com>
 * Copyright (c) 2011 Intel Corp.
 * 
 * UMMS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * UMMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with UMMS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _UMMS_COMMON_H
#define _UMMS_COMMON_H

#define UMMS_SERVICE_NAME "com.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.UMMS.MediaPlayer"

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH "/com/UMMS/PlayingContentMetadataViewer"
#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_INTERFACE_NAME "com.UMMS.PlayingContentMetadataViewer"

#define UMMS_AUDIO_MANAGER_OBJECT_PATH "/com/UMMS/AudioManager"
#define UMMS_AUDIO_MANAGER_INTERFACE_NAME "com.UMMS.AudioManger"


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

/*platform type */
typedef enum {
  CETV,
  NETBOOK,
  PLATFORM_INVALID
}PlatformType;

#endif /* _UMMS_COMMON_H */

