#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gdk/gdkx.h> 
#include <gst/interfaces/xoverlay.h> 
#include "umms-gtk-backend.h"
#include "umms-gtk-ui.h"

static GstElement *pipeline;
static GstElement *video_sink;

static gboolean expose_cb(GtkWidget *widget,GdkEventExpose *event,gpointer data)
{ 
    g_print("---->\n");
    gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data),GDK_WINDOW_XWINDOW(widget->window)); 
    //gulong video_window_xid = GDK_WINDOW_XID (widget->window);
    //gst_x_overlay_set_window_handle(GST_X_OVERLAY(data), video_window_xid); 
    //gst_x_overlay_set_render_rectangle(GST_X_OVERLAY(data), 0,0, 480,320); 
    return FALSE; 
}

static gboolean avdec_bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) loop;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            ui_send_stop_signal();
            break;
        case GST_MESSAGE_ERROR:
            g_print("Error\n");
            g_main_loop_quit(loop);
            break;
        default:
            //g_print("Default branch:%s\n", GST_MESSAGE_TYPE_NAME(msg));
            break;
    }

    return TRUE;
}

static gint avdec_getduration(GstElement *pipeline)
{
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 pos, len;
    GstState current, pending;

    gst_element_get_state(pipeline, &current, &pending, 0);


    gst_element_query_position(pipeline, &fmt, &pos);
    gst_element_query_duration(pipeline, &fmt, &len);
    g_print("duration is:%"GST_TIME_FORMAT"/%"GST_TIME_FORMAT"\n", GST_TIME_ARGS(pos), GST_TIME_ARGS(len));

    ui_update_progressbar(pos, len);

    if (current == GST_STATE_PLAYING) {
        g_timeout_add(500, (GSourceFunc)avdec_getduration, pipeline);
    }

    return 0;
}

gint avdec_init(void)
{
    GstBus *bus;
    int argc = 0;
    char **argv;

    gst_init(&argc, &argv);

    /* Create gstreamer elements */
    pipeline = gst_element_factory_make("playbin", "playerbin");
    g_object_get(G_OBJECT(pipeline), "video-sink", &video_sink, NULL);
    gst_bin_remove(GST_BIN(pipeline), video_sink);
    video_sink = gst_element_factory_make("ximagesink","video- sink"); 
    if (!pipeline || !video_sink) {
        g_printerr("One element can not be created!\n");
        return -1;
    }

    /* add message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, avdec_bus_call, NULL);
    gst_object_unref(bus);

    g_object_set(G_OBJECT(pipeline), "video-sink", video_sink, NULL);

    //gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(video_sink),
    //        GDK_WINDOW_XWINDOW(video_image)); 
    g_signal_connect(video_window, "expose-event", 
            G_CALLBACK(expose_cb), video_sink); 


    return 0;
}

gint avdec_set_source(gchar *filename)
{
    gchar fname[200] = "file://";

    strcat(fname, filename);

    g_object_set(G_OBJECT(pipeline), "uri", fname, NULL);


    return 0;
}

gint avdec_start(void)
{
    avdec_seek_from_beginning(0);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_timeout_add(500, (GSourceFunc)avdec_getduration, pipeline);

    gtk_window_set_focus(GTK_WINDOW(window), video_window);

    //g_signal_emit_by_name(video_window, "expose-event");

    return 0;
}

gint avdec_stop(void)
{
    gst_element_set_state(pipeline, GST_STATE_READY);
    return 0;
}

gint avdec_pause(void)
{
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    return 0;
}

gint avdec_resume(void)
{
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_timeout_add(500, (GSourceFunc)avdec_getduration, pipeline);
    return 0;
}

gint avdec_seek_from_beginning(gint64 nanosecond)
{
    gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
            GST_SEEK_TYPE_SET, nanosecond,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
    return 0;
}

