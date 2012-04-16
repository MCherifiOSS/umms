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

#include <glib.h>

#define UMMS_SERVICE_NAME "com.UMMS"
#define UMMS_OBJECT_MANAGER_OBJECT_PATH "/com/UMMS/ObjectManager"
#define UMMS_OBJECT_MANAGER_INTERFACE_NAME "com.UMMS.ObjectManager.iface"
#define MEDIA_PLAYER_INTERFACE_NAME "com.UMMS.MediaPlayer"

#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_OBJECT_PATH "/com/UMMS/PlayingContentMetadataViewer"
#define UMMS_PLAYING_CONTENT_METADATA_VIEWER_INTERFACE_NAME "com.UMMS.PlayingContentMetadataViewer"

#define UMMS_AUDIO_MANAGER_OBJECT_PATH "/com/UMMS/AudioManager"
#define UMMS_AUDIO_MANAGER_INTERFACE_NAME "com.UMMS.AudioManger"

#define UMMS_VIDEO_OUTPUT_OBJECT_PATH "/com/UMMS/VideoOutput"
#define UMMS_VIDEO_OUTPUT_INTERFACE_NAME "com.UMMS.VideoOutput"

#define RESOURCE_GROUP "Resource Definition"
#define PROXY_GROUP "Proxy"
#define PLAYER_PLUGIN_GROUP "Player Plugin Preference"
#define UMMS_PLUGINS_PATH_DEFAULT "/usr/lib/umms"

typedef struct _UmmsCtx {
  GList *plugins;
  GKeyFile *conf;
  GKeyFile *resource_conf;
  gchar *proxy_uri;
  gchar *proxy_id;
  gchar *proxy_pw;
} UmmsCtx;

extern UmmsCtx *umms_ctx;

#endif /* _UMMS_COMMON_H */

