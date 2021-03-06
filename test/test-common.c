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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "test-common.h"

gchar args[2][256];

const gchar *engine_error_str[] = {
  "NOT_LOADED",
  "FAILED"
};

const gchar *state_name[] = {
  "PlayerStateNull",
  "PlayerStatePaused",
  "PlayerStatePlaying",
  "PlayerStateStopped"
};

static char *method_name[N_METHOD] = {
  "SetUri",
  "SetTarget",
  "Play",
  "Pause",
  "Stop",
  "SetPosition",
  "GetPosition",
  "SetPlaybackRate",
  "GetPlaybackRate",
  "SetVolume",
  "GetVolume",
  "SetVideoSize",
  "GetVideoSize",
  "GetBufferedTime",
  "GetBufferedBytes",
  "GetMediaSizeTime",
  "GetMediaSizeBytes",
  "HasVideo",
  "HasAudio",
  "IsStreaming",
  "IsSeekable",
  "SupportFullscreen",
  "GetPlayerState"
};

//gchar *DEFAULT_URI = "http://sh1sskweb001.ccr.corp.intel.com/html5_workloads/workload_mytube/workload/videos/720p.m4v";
gchar *DEFAULT_URI = "file:///root/720p.m4v";

static void shell_help ()
{
  int i;
  for (i = 0; i < N_METHOD; i++) {
    printf ("'%d':  '%s'\n", i, method_name[i]);
  }
  printf ("'?': to display this help info\n");
  printf ("'h': to display this help info\n");
  printf ("'q': to quit\n");
}


void get_param (gchar *method_str, gchar *param)
{
  gboolean need_input = TRUE;
  gint method_id;

  method_id = atoi(method_str);
  switch (method_id) {
  case SetUri:
    g_print("Input uri:\n");
    break;
  case SetPosition:
    g_print("Input pos to seek: (seconds)\n");
    break;
  case SetPlaybackRate:
    g_print("Input playback rate: \n");
    break;
  case SetVolume:
    g_print("Input volume to set: [0,100]\n");
    break;
  case SetVideoSize:
    g_print("Input dest reactangle, Example: 0,0,352,288\n");
    break;
  default:
    need_input = FALSE;
    break;
  }
  if (need_input) {
    fscanf (stdin, "%s", param);
    g_print("param is '%s'\n", param);
  }
  return;
}

gboolean to_continue = TRUE;
gboolean method_call (gpointer data);

gpointer cmd_thread_func (gpointer data)
{
  gint  rc;

  while (to_continue) {
    g_print ("Input cmd:\n");
    rc = fscanf(stdin, "%s", args[0]);
    if (rc <= 0) {
      g_print("NULL\n");
    }

    if (!strcmp(args[0], "h")) {
      shell_help();
    } else if (!strcmp(args[0], "?")) {
      shell_help();
    } else if (!strcmp(args[0], "q")) {
      g_idle_add (method_call, NULL);
    } else {
      get_param(args[0], args[1]);
      g_print ("args[0]=%s, args[1]=%s\n", args[0], args[1]);
      g_idle_add (method_call, NULL);
    }
  }
  return NULL;
}

