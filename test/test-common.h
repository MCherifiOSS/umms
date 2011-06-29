#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <stdlib.h>
#include <glib.h>

#define TESTER_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)

#define TESTER_ERROR(prefix, gerror) \
          do {\
                g_print (prefix ":%s", error->message); \
                g_error_free(gerror); \
                exit(1);\
          }while(0)


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

typedef enum {
    PlayerStateNull,
    PlayerStatePaused,
    PlayerStatePlaying,
    PlayerStateStopped,
    NumOfPlayerState
}PlayerState;


extern const gchar *error_type[];
extern const gchar *state_name[];
extern gchar args[2][256];
extern gchar *DEFAULT_URI;
extern gboolean to_continue;

gpointer cmd_thread_func (gpointer data);
#endif /* _TEST_COMMON_H */
