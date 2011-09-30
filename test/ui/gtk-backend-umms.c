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

    dbus_g_object_register_marshaller (umms_marshal_VOID__UINT_STRING, G_TYPE_NONE, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "Error", G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (player, "PlayerStateChanged", G_TYPE_INT, G_TYPE_INVALID);
}


static void __initialized_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG ("MeidaPlayer initialized");
}

static void __player_state_changed_cb(DBusGProxy *player, gint state, gpointer user_data)
{
    UMMS_DEBUG("State changed to '%s'", state_name[state]);
}

static void __eof_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "EOF....");
}

static void __begin_buffering_cb(DBusGProxy *player, gpointer user_data)
{
}

static void __buffered_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Buffering completed");
}

static void __seeked_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Seeking completed");
}

static void __stopped_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Player stopped");
}

static void __error_cb(DBusGProxy *player, guint err_id, gchar *msg, gpointer user_data)
{
    UMMS_DEBUG( "Error Domain:'%s', msg='%s'", error_type[err_id], msg);
}

static void __request_window_cb(DBusGProxy *player, gpointer user_data)
{
    UMMS_DEBUG( "Player engine request a X window");
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
        UMMS_GERROR ("Failed to SetUri", error);
        return -1;
    }
    return 0;
}

gint avdec_set_speed(int speed)
{
    GError *error = NULL;

    if (!dbus_g_proxy_call (player, "SetPlaybackRate", &error,
            G_TYPE_INT64, speed,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
        UMMS_GERROR ("Failed to SetPlaybackRate", error);
        return -1;
    }
    return 0;
}

