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

#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <stdlib.h>
#include <glib.h>

#define TESTER_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)

#define TESTER_ERROR(prefix, gerror) \
          do {\
                g_print (prefix ": %s\n", error->message); \
                g_error_free(gerror); \
          }while(0)

/* redefine 
typedef enum {
    UMMS_ENGINE_ERROR_NOT_LOADED,
    UMMS_ENGINE_ERROR_FAILED
}UmmsEngineError;
*/

typedef enum {
  SetUri,
  SetTarget,
  Play,
  Pause,
  Stop,
  SetPosition,
  GetPosition,//6
  SetPlaybackRate,
  GetPlaybackRate,
  SetVolume,
  GetVolume,
  SetWindowId,//11
  SetVideoSize,
  GetVideoSize,
  GetBufferedTime,
  GetBufferedBytes,
  GetMediaSizeTime,//16
  GetMediaSizeBytes,
  HasVideo,
  HasAudio,
  IsStreaming,
  IsSeekable,//21
  SupportFullscreen,
  GetPlayerState,
  N_METHOD
}MethodId;

typedef enum {
    ErrorTypeEngine,
    NumOfErrorType
}ErrorType;

/*
 * redefine
typedef enum {
    PlayerStateNull,
    PlayerStatePaused,
    PlayerStatePlaying,
    PlayerStateStopped,
    NumOfPlayerState
}PlayerState;
*/


extern const gchar *engine_error_str[];
extern const gchar *state_name[];
extern gchar args[2][256];
extern gchar *DEFAULT_URI;
extern gboolean to_continue;

gpointer cmd_thread_func (gpointer data);
#endif /* _TEST_COMMON_H */
