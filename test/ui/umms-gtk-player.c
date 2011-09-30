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
#include <gtk/gtk.h>
#include "umms-gtk-player.h"
#include "gtk-backend.h"

gchar dbg_state_name[][30] = {
    "PLY_MAIN_STATE_IDLE",
    "PLY_MAIN_STATE_READY",
    "PLY_MAIN_STATE_RUN",
    "PLY_MAIN_STATE_PAUSE",
};

static PlyMainData ply_main_data;

gint ply_init(void)
{
    ply_main_data.state_lock = g_mutex_new ();
    ply_main_data.state = PLY_MAIN_STATE_IDLE;
    ply_main_data.play_speed = 1;
    avdec_init();
    return 0;
}

PlyMainState ply_get_state(void)
{
    g_mutex_lock (ply_main_data.state_lock);
    PlyMainState state = ply_main_data.state;
    g_mutex_unlock (ply_main_data.state_lock);
    return state;
}


void ply_set_state(PlyMainState state)
{
    g_mutex_lock (ply_main_data.state_lock);
    ply_main_data.state = state;
    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    g_mutex_unlock (ply_main_data.state_lock);
}


gint ply_get_speed(void)
{
    g_mutex_lock (ply_main_data.state_lock);
    gint speed = ply_main_data.play_speed;
    g_mutex_unlock (ply_main_data.state_lock);
    return speed;
}


void ply_set_speed(gint speed)
{
    g_mutex_lock (ply_main_data.state_lock);
    ply_main_data.play_speed = speed;
    g_mutex_unlock (ply_main_data.state_lock);
}


gint ply_reload_file(gchar *filename)
{
    if (ply_main_data.state != PLY_MAIN_STATE_IDLE &&
            ply_main_data.state != PLY_MAIN_STATE_READY) {
        ply_stop_stream();
    }

    //g_string_assign(&ply_main_data.filename, filename);
    avdec_set_source(filename);
    ply_main_data.state = PLY_MAIN_STATE_READY;

    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    return 0;
}

gint ply_play_stream(void)
{
    avdec_start();

    return 0;
}

gint ply_stop_stream(void)
{
    avdec_stop();
    return 0;
}

gint ply_pause_stream(void)
{
    avdec_pause();
    return 0;
}

gint ply_resume_stream(void)
{
    avdec_resume();
    return 0;
}

gint ply_seek_stream_from_beginging(gint64 nanosecond)
{
    avdec_seek_from_beginning(nanosecond);
    return 0;
}


gint ply_forward_rewind(gint speed)
{
    avdec_set_speed(speed);
    return 0;
}


gint ply_get_play_speed(void)
{
    return avdec_get_speed();
}

