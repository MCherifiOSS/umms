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

#ifndef _UMMS_TYPES_H
#define _UMMS_TYPES_H

typedef enum {
  PlayerStateNull,
  PlayerStateStopped,
  PlayerStatePaused,
  PlayerStatePlaying
} PlayerState;

typedef enum {
  TargetTypeInvalid = -1,
  XWindow,
  DataCopy,
  Socket,
  ReservedType0,//On CE4100 platform, indicate using gdl plane directly to render video data
  ReservedType1,
  ReservedType2,
  ReservedType3
} TargetType;

typedef enum {
  BufferFormatInvalid = -1,
  BufferFormatByTime,
  BufferFormatByBytes
} BufferFormat;

typedef enum {
  ScaleModeInvalid = -1,
  ScaleModeNoScale,             /*  Do nothing, just leave the output x, y as the input. */
  ScaleModeFill,                /*  fill entire target, regardless of original video aspect ratio. */
  ScaleModeKeepAspectRatio,     /*  respect the original video aspect ratio. */
  ScaleModeFillKeepAspectRatio, /*  fill the whole screen but keep the ratio, some part of video may out of screen.*/
} ScaleMode;

typedef enum {
  AUDIO_OUTPUT_TYPE_HDMI,
  AUDIO_OUTPUT_TYPE_SPDIF,
  AUDIO_OUTPUT_TYPE_I2S0,
  AUDIO_OUTPUT_TYPE_I2S1,
  AUDIO_OUTPUT_TYPE_NUM
} AudioOutputType;

typedef enum {
  AUDIO_OUTPUT_STATE_OFF,
  AUDIO_OUTPUT_STATE_ON,
  AUDIO_OUTPUT_STATE_NUM
} AudioOutputState;

typedef enum _Mesg {
  MSG_NOT_IMPLEMENTED,
  MSG_BACKEND_NOT_LOADED,
  MSG_NUM
} Mesg;

#endif /* _UMMS_TYPES_H */

