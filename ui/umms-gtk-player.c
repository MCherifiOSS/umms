#include <glib.h>
#include <gtk/gtk.h>
#include "umms-gtk-player.h"
#include "umms-gtk-backend.h"

gchar dbg_state_name[][30] = {
    "PLY_MAIN_STATE_IDLE",
    "PLY_MAIN_STATE_READY",
    "PLY_MAIN_STATE_RUN",
    "PLY_MAIN_STATE_PAUSE",
};

static PlyMainData ply_main_data;

gint ply_init(void)
{
    ply_main_data.state = PLY_MAIN_STATE_IDLE;
    avdec_init();
    return 0;
}

PlyMainData *ply_get_maindata(void)
{
    return &ply_main_data;
}


PlyMainState ply_get_state(void)
{
    return ply_main_data.state;
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
    ply_main_data.state = PLY_MAIN_STATE_RUN;

    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    return 0;
}

gint ply_stop_stream(void)
{
    /* If main state is idle or ready, remain its current state */
    if (ply_main_data.state != PLY_MAIN_STATE_IDLE &&
            ply_main_data.state != PLY_MAIN_STATE_READY) {
        avdec_stop();
        ply_main_data.state = PLY_MAIN_STATE_READY;
    }

    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    return 0;
}

gint ply_pause_stream(void)
{
    if (ply_main_data.state == PLY_MAIN_STATE_RUN) {
        ply_main_data.state = PLY_MAIN_STATE_PAUSE;
        avdec_pause();
    }

    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    return 0;
}

gint ply_resume_stream(void)
{
    if (ply_main_data.state == PLY_MAIN_STATE_PAUSE) {
        ply_main_data.state = PLY_MAIN_STATE_RUN;
        avdec_resume();
    }

    g_print("%s\n", dbg_state_name[ply_main_data.state]);
    return 0;
}

gint ply_seek_stream_from_beginging(gint64 nanosecond)
{
    avdec_seek_from_beginning(nanosecond);
    return 0;
}

