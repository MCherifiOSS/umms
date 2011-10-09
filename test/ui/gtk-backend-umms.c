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
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include <gdk/gdkx.h>
#include "umms-gtk-ui.h"
#include "gtk-backend.h"
#include "../../src/umms-marshals.h"
#include "../../libummsclient/umms-client-object.h"
#include "../../src/umms-debug.h"

static UmmsClientObject *umms_client_obj = NULL;
static DBusGProxy *player = NULL;

const gchar *state_name[] = {
    "PlayerStateNull",
    "PlayerStatePaused",
    "PlayerStatePlaying",
    "PlayerStateStopped"
};

const gchar *error_type[] = {
    "ErrorTypeEngine",
    "NumOfErrorType"
};

static void
add_sigs(DBusGProxy *player)
{
    dbus_g_proxy_add_signal (player, "Initialized", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Eof", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Buffering", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Buffered", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "RequestWindow", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Seeked", G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Stopped", G_TYPE_INVALID);

    dbus_g_proxy_add_signal (player, "VideoTagChanged", G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "AudioTagChanged", G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "TextTagChanged", G_TYPE_INT, G_TYPE_INVALID);

    dbus_g_object_register_marshaller (umms_marshal_VOID__UINT_STRING, G_TYPE_NONE, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Error", G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
    
    dbus_g_object_register_marshaller (umms_marshal_VOID__INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "PlayerStateChanged", G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
}

static void __initialized_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG ("MeidaPlayer initialized");
}

static void __player_state_changed_cb(DBusGProxy *player, gint state, gpointer user_data)
{
    UMMS_DEBUG("State changed to '%s'", state_name[state]);
    ui_callbacks_for_reason(UI_CALLBACK_STATE_CHANGE, (void *)state, NULL);
}

static void __eof_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "EOF....");
    ui_callbacks_for_reason(UI_CALLBACK_EOF, NULL, NULL);
}

static void __begin_buffering_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "BUFFERING ....");
    ui_callbacks_for_reason(UI_CALLBACK_BUFFERING, NULL, NULL);
}

static void __buffered_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Buffering completed");    
    ui_callbacks_for_reason(UI_CALLBACK_BUFFERED, NULL, NULL);
}

static void __seeked_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Seeking completed");
    ui_callbacks_for_reason(UI_CALLBACK_SEEKED, NULL, NULL);
}

static void __stopped_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Player stopped");    
    ui_callbacks_for_reason(UI_CALLBACK_STOPPED, NULL, NULL);
}

static void __error_cb(DBusGProxy *player, guint err_id, gchar *msg, gpointer user_data)
{
    UMMS_DEBUG( "Error Domain:'%s', msg='%s'", error_type[err_id], msg);
    ui_callbacks_for_reason(UI_CALLBACK_ERROR, (void *)err_id, (void *)msg);
}

static void __request_window_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Player engine request a X window");
}

static void __video_tag_changed_cb(DBusGProxy *player, guint stream_id, gpointer user_data)
{
    UMMS_DEBUG( "Video tag changed");
    ui_callbacks_for_reason(UI_CALLBACK_VIDEO_TAG_CHANGED, (void *)stream_id, NULL);
}

static void __audio_tag_changed_cb(DBusGProxy *player, guint stream_id, gpointer user_data)
{
    UMMS_DEBUG( "Audio tag changed");
    ui_callbacks_for_reason(UI_CALLBACK_AUDIO_TAG_CHANGED, (void *)stream_id, NULL);
}

static void __text_tag_changed_cb(DBusGProxy *player, guint stream_id, gpointer user_data)
{
    UMMS_DEBUG( "Text tag changed");
    ui_callbacks_for_reason(UI_CALLBACK_TEXT_TAG_CHANGED, (void *)stream_id, NULL);
}

static void
connect_sigs(DBusGProxy *player)
{
    UMMS_DEBUG ("called");
    dbus_g_proxy_connect_signal (player,
            "Initialized",
            G_CALLBACK(__initialized_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Eof",
            G_CALLBACK(__eof_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Buffering",
            G_CALLBACK(__begin_buffering_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Buffered",
            G_CALLBACK(__buffered_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "RequestWindow",
            G_CALLBACK(__request_window_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Seeked",
            G_CALLBACK(__seeked_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Stopped",
            G_CALLBACK(__stopped_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "Error",
            G_CALLBACK(__error_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "PlayerStateChanged",
            G_CALLBACK(__player_state_changed_cb),
            NULL, NULL);
    
    dbus_g_proxy_connect_signal (player,
            "VideoTagChanged",
            G_CALLBACK(__video_tag_changed_cb),
            NULL, NULL);
    
    dbus_g_proxy_connect_signal (player,
            "AudioTagChanged",
            G_CALLBACK(__audio_tag_changed_cb),
            NULL, NULL);

    dbus_g_proxy_connect_signal (player,
            "TextTagChanged",
            G_CALLBACK(__text_tag_changed_cb),
            NULL, NULL);
}


static void
__val_release (gpointer data)
{
    GValue *val = (GValue *)data;
    g_value_unset(val);
    g_free (val);
}

static void __param_table_release (GHashTable *param)
{
    g_return_if_fail (param);
    g_hash_table_destroy (param);
}

static GHashTable *
__param_table_create (const gchar* key1, ...)
{
    GHashTable  *params;
    const gchar *key;
    GValue      *val;
    GType       valtype;
    const char  *collect_err;
    va_list     args;

    params = g_hash_table_new_full (g_str_hash, NULL, NULL, __val_release);

    va_start (args, key1);

    key = key1;
    while (key != NULL) {
        valtype = va_arg (args, GType);
        val = g_new0(GValue, 1);
        g_value_init (val, valtype);
        collect_err = NULL;
        G_VALUE_COLLECT (val, args, G_VALUE_NOCOPY_CONTENTS, &collect_err);
        g_hash_table_insert(params, (gpointer)key, val);
        key = va_arg (args, gpointer);
    }

    va_end (args);

    return params;
}


static gboolean umms_expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    GHashTable * para_hash;
    GError *error = NULL;

    para_hash = __param_table_create("window-id",
                G_TYPE_INT,
                GDK_WINDOW_XWINDOW(widget->window),
                NULL);

    if (!dbus_g_proxy_call (player, "SetTarget", &error,
            G_TYPE_INT, 0,
            dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
            para_hash,
            G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetTarget", error);
        return -1;
    }
    __param_table_release(para_hash);
    return FALSE;
}

gint avdec_init(void)
{
    gchar *obj_path;

    g_type_init ();

    umms_client_obj = umms_client_object_new();

    g_signal_connect(video_window, "expose-event",
                     G_CALLBACK(umms_expose_cb), NULL);

    player = umms_client_object_request_player (umms_client_obj, TRUE, 0, &obj_path);
    add_sigs (player);
    connect_sigs (player);
}


gint avdec_set_source(gchar *filename)
{
    GError *error = NULL;
    gchar * file_name;

    file_name = g_strdup_printf("file://%s", filename);
    printf("file_name is %s\n", file_name);
    if (!dbus_g_proxy_call (player, "SetUri", &error,
            G_TYPE_STRING, file_name, G_TYPE_INVALID,
            G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    g_free(file_name);
    return 0;
}

gint avdec_start(void)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "Play", &error,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    return 0;
}

gint avdec_stop(void)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "Stop", &error,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    return 0;
}

gint avdec_pause(void)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "Pause", &error,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    return 0;
}

gint avdec_resume(void)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "Play", &error,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    return 0;
}

gint avdec_seek_from_beginning(gint64 nanosecond)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "SetPosition", &error,
            G_TYPE_INT64, nanosecond,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetPosition", error);
        return -1;
    }
    return 0;
}

gint avdec_set_speed(int speed)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "SetPlaybackRate", &error,
            G_TYPE_DOUBLE, (gdouble)speed,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to set_speed", error);
        return -1;
    }
    return 0;
}


gint avdec_get_speed(void)
{
    GError *error = NULL;
    gdouble speed;

    if (!dbus_g_proxy_call (player, "GetPlaybackRate", &error,
                G_TYPE_INVALID, G_TYPE_DOUBLE, &speed,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to get_speed", error);
        return 1;
    }
    return (gint)speed;
}


gint avdec_get_duration(gint64 * len)
{
    GError *error = NULL;
    gint64 dur;

    if (!dbus_g_proxy_call (player, "GetMediaSizeTime", &error,
                G_TYPE_INVALID, G_TYPE_INT64, &dur,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to get_duration", error);
        return -1;
    }

    //printf("Current duration = %lli\n", dur);
    *len = dur;
    return 0;
}

gint avdec_get_position(gint64 * pos)
{
    GError *error = NULL;
    gint64 cur_pos;

    if (!dbus_g_proxy_call (player, "GetPosition", &error,
                G_TYPE_INVALID, G_TYPE_INT64, &cur_pos,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to get_position", error);
        return -1;
    }

    printf("Current pos = %lli\n", cur_pos);
    *pos = cur_pos;
    return 0;
}

gint avdec_get_video_num(gint * video_num)
{
    GError *error = NULL;
    gint video;

    if (!dbus_g_proxy_call (player, "GetVideoNum", &error,
                G_TYPE_INVALID, G_TYPE_INT, &video,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoNum", error);
        return -1;
    }

    *video_num = video;
    return 0;
}


gint avdec_get_cur_video(gint * cur_video)
{
    GError *error = NULL;
    gint video;

    if (!dbus_g_proxy_call (player, "GetCurrentVideo", &error,
                G_TYPE_INVALID, G_TYPE_INT, &video,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetCurrentVideo", error);
        return -1;
    }

    *cur_video = video;
    return 0;
}

gint avdec_set_cur_video(gint cur_video)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "SetCurrentVideo", &error,
            G_TYPE_INT, cur_video,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to set_cur_video", error);
        return -1;
    }
    return 0;

}

gint avdec_get_video_codec(gint video, gchar ** codec_name)
{
    GError *error = NULL;
    gchar * name;

    if (!dbus_g_proxy_call (player, "GetVideoCodec", &error,
                G_TYPE_INT, video, G_TYPE_INVALID,
                G_TYPE_STRING, &name, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoCodec", error);
        return -1;
    }

    *codec_name = g_strdup(name);
    g_free(name);
    return 0;
}


gint avdec_get_audio_num(gint * audio_num)
{
    GError *error = NULL;
    gint audio;

    if (!dbus_g_proxy_call (player, "GetAudioNum", &error,
                G_TYPE_INVALID, G_TYPE_INT, &audio,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoNum", error);
        return -1;
    }

    *audio_num = audio;
    return 0;
}


gint avdec_get_cur_audio(gint * cur_audio)
{
    GError *error = NULL;
    gint audio;

    if (!dbus_g_proxy_call (player, "GetCurrentAudio", &error,
                G_TYPE_INVALID, G_TYPE_INT, &audio,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetCurrentAudio", error);
        return -1;
    }

    *cur_audio = audio;
    return 0;
}

gint avdec_set_cur_audio(gint cur_audio)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "SetCurrentAudio", &error,
            G_TYPE_INT, cur_audio,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to set_cur_audio", error);
        return -1;
    }
    return 0;

}


gint avdec_get_audio_codec(gint audio, gchar ** codec_name)
{
    GError *error = NULL;
    gchar * name;

    if (!dbus_g_proxy_call (player, "GetAudioCodec", &error,
                G_TYPE_INT, audio, G_TYPE_INVALID,
                G_TYPE_STRING, &name, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetAudioCodec", error);
        return -1;
    }

    *codec_name = g_strdup(name);
    g_free(name);
    return 0;
}


gint avdec_get_encapsulation(gchar ** container_name)
{
    GError *error = NULL;
    gchar * name;

    if (!dbus_g_proxy_call (player, "GetEncapsulation", &error,
                G_TYPE_INVALID,
                G_TYPE_STRING, &name, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetEncapsulation", error);
        return -1;
    }

    *container_name = g_strdup(name);
    g_free(name);
    return 0;
}


gint avdec_get_video_bitrate(gint video_num, gint * bit_rate)
{
    GError *error = NULL;
    gint rate;

    if (!dbus_g_proxy_call (player, "GetVideoBitrate", &error,
                G_TYPE_INT, video_num, G_TYPE_INVALID,
                G_TYPE_INT, &rate, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoBitrate", error);
        return -1;
    }

    *bit_rate = rate;
    return 0;
}

gint avdec_get_video_framerate(gint video_num, gint * rate_num, gint * rate_denom)
{
    GError *error = NULL;
    gint num, denom;

    if (!dbus_g_proxy_call (player, "GetVideoFramerate", &error,
                G_TYPE_INT, video_num, G_TYPE_INVALID,
                G_TYPE_INT, &num,
                G_TYPE_INT, &denom,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoFramerate", error);
        return -1;
    }

    //printf("================= num %d, demon: %d\n", num, denom);
    *rate_num = num;
    *rate_denom = denom;
    return 0;
}


gint avdec_get_video_resolution(gint video_num, gint * width, gint * height)
{
    GError *error = NULL;
    gint width_, height_;

    if (!dbus_g_proxy_call (player, "GetVideoResolution", &error,
                G_TYPE_INT, video_num, G_TYPE_INVALID,
                G_TYPE_INT, &width_,
                G_TYPE_INT, &height_,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoResolution", error);
        return -1;
    }

    //printf("resolution is %d X %d\n", width_, height_);
    *width = width_;
    *height = height_;
    return 0;
}


gint avdec_get_video_aspectratio(gint video_num, gint * ratio_num, gint * ratio_denom)
{
    GError *error = NULL;
    gint num, denom;

    if (!dbus_g_proxy_call (player, "GetVideoAspectRatio", &error,
                G_TYPE_INT, video_num, G_TYPE_INVALID,
                G_TYPE_INT, &num,
                G_TYPE_INT, &denom,
                G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetVideoAspectRatio", error);
        return -1;
    }

    //printf("================= num %d, demon: %d\n", num, denom);
    *ratio_num = num;
    *ratio_denom = denom;
    return 0;
}


gint avdec_get_audio_bitrate(gint audio_num, gint * bit_rate)
{
    GError *error = NULL;
    gint rate;

    if (!dbus_g_proxy_call (player, "GetAudioBitrate", &error,
                G_TYPE_INT, audio_num, G_TYPE_INVALID,
                G_TYPE_INT, &rate, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetAudioBitrate", error);
        return -1;
    }

    *bit_rate = rate;
    return 0;
}

gint avdec_get_audio_samplerate(gint audio_num, gint * sample_rate)
{
    GError *error = NULL;
    gint rate;

    if (!dbus_g_proxy_call (player, "GetAudioSamplerate", &error,
                G_TYPE_INT, audio_num, G_TYPE_INVALID,
                G_TYPE_INT, &rate, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to GetAudioBitrate", error);
        return -1;
    }

    *sample_rate = rate;
    return 0;
}

