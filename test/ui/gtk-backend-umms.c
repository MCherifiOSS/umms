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

#include "gtk-backend.h"
#include "umms-gtk-ui.h"

static gboolean expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    return FALSE;
}

gint avdec_init(void)
{
    g_signal_connect(video_window, "expose-event",
            G_CALLBACK(umms_expose_cb), NULL);
}


gint avdec_set_source(gchar *filename)
{

    return 0;
}

gint avdec_start(void)
{
    return 0;
}

gint avdec_stop(void)
{
    return 0;
}

gint avdec_pause(void)
{
    return 0;
}

gint avdec_resume(void)
{
    return 0;
}

gint avdec_seek_from_beginning(gint64 nanosecond)
{
    return 0;
}

