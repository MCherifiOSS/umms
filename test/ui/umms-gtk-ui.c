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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "umms-gtk-player.h"
#include "umms-gtk-ui.h"

#define MY_GST_TIME_ARGS(t) \
            GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) (((GstClockTime)(t)) / (GST_SECOND * 60 * 60)) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / (GST_SECOND * 60)) % 60) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / GST_SECOND) % 60) : 99

GtkWidget *video_window;
GtkWidget *window;

static GtkWidget *button_play;
static GtkWidget *progressbar;
static GtkWidget *progress_time;
static GtkWidget *image_play;
static GtkWidget *image_pause;

static int av_sub_update_flag;
static GtkWidget *video_combo;
static GtkWidget *audio_combo;
static GtkWidget *text_combo;

int get_raw_data(void)
{
    int sockfd;
    char buffer[1024];
    struct sockaddr_in server_addr;
    int portnumber, nbytes;
    struct hostent *host;
    int i;

    host = gethostbyname("localhost");

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error:%s\a\n", (char *)strerror(errno));
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(112131);
    server_addr.sin_addr = *((struct in_addr*)host->h_addr);

    if (connect (sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1) {
        printf("Connect Error: %s\a\n", strerror(errno));
        close(sockfd);
        return;
    }

    for (i = 0; i < 3; i++) {
        nbytes = read(sockfd, buffer, 1024);
        printf( "received:%d bytes\n", nbytes);
        if (nbytes >= 0) {
            buffer[nbytes] = '\0';
            printf( "I have received:%x %x %x %x %x %x %x %x\n",
                    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],
                    buffer[5], buffer[6], buffer[7]);
        }
    }

    close(sockfd);
}


static void ui_pause_bt_cb(GtkWidget *widget, gpointer data)
{
    if (ply_get_state() == PLY_MAIN_STATE_READY) {
        ply_play_stream();
        gtk_container_remove(GTK_CONTAINER(button_play), image_play);
        gtk_container_add(GTK_CONTAINER(button_play), image_pause);
    } else if (ply_get_state() == PLY_MAIN_STATE_RUN) {
        ply_pause_stream();
        gtk_container_remove(GTK_CONTAINER(button_play), image_pause);
        gtk_container_add(GTK_CONTAINER(button_play), image_play);
    } else if (ply_get_state() == PLY_MAIN_STATE_PAUSE) {
        ply_resume_stream();
        gtk_container_remove(GTK_CONTAINER(button_play), image_play);
        gtk_container_add(GTK_CONTAINER(button_play), image_pause);
    }
    gtk_widget_show_all(window);
}

static void ui_stop_bt_cb(GtkWidget *widget, gpointer data)
{
    if (ply_get_state() == PLY_MAIN_STATE_RUN) {
        ply_stop_stream();
        gtk_container_remove(GTK_CONTAINER(button_play), image_pause);
        gtk_container_add(GTK_CONTAINER(button_play), image_play);
    } else if (ply_get_state() == PLY_MAIN_STATE_PAUSE) {
        ply_stop_stream();
    }
}

static void ui_forward_bt_cb(GtkWidget *widget, gpointer data)
{
}


static void ui_rewind_bt_cb(GtkWidget *widget, gpointer data)
{

}

static void ui_info_bt_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *info_dlg;
    GtkWidget *lab;
    GtkWidget *frame;

    info_dlg = gtk_dialog_new_with_buttons("Media Info",
               GTK_WINDOW(gtk_widget_get_toplevel (window)),
               GTK_DIALOG_MODAL,
               GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
               NULL);

    gtk_window_set_default_size (GTK_WINDOW (info_dlg), 400, 400);

    g_signal_connect (info_dlg, "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &info_dlg);

    g_signal_connect (info_dlg, "response",
                      G_CALLBACK (gtk_widget_destroy),
                      NULL);

    /* Add the media info here. */
    frame = gtk_frame_new ("Container Type");
    lab = gtk_label_new(NULL);
    gtk_label_set_text(GTK_LABEL(lab), "ASF");
    gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
    gtk_container_add (GTK_CONTAINER (frame), lab);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dlg)->vbox),
                        frame, FALSE, FALSE, 0);

    gtk_widget_show_all (info_dlg);
}


static void ui_options_bt_cb(GtkWidget *widget, gpointer data)
{

}

static void ui_fileopen_dlg(GtkWidget *widget, gpointer data)
{
    GtkWidget *fileopen_dlg;
    gint result;
    gchar *filename;

    fileopen_dlg = gtk_file_chooser_dialog_new("Select media file",
                   GTK_WINDOW(window),
                   GTK_FILE_CHOOSER_ACTION_OPEN,
                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                   GTK_STOCK_OK, GTK_RESPONSE_OK,
                   NULL);
    result = gtk_dialog_run(GTK_DIALOG(fileopen_dlg));
    if (GTK_RESPONSE_OK == result) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileopen_dlg));
        gtk_widget_destroy(fileopen_dlg);
        g_print("filename is %s\n", filename);
        ply_reload_file(filename);
        ui_pause_bt_cb(widget, data);
        g_free(filename);
        av_sub_update_flag = 1;
    } else {
        gtk_widget_destroy(fileopen_dlg);
    }
}

static void ui_progressbar_vchange_cb( GtkAdjustment *get,
        GtkAdjustment *set )
{
    PlyMainData *ply_maindata;

    g_print("fasfafafasfafsa:%f\n", get->value);
    if (ply_get_state() == PLY_MAIN_STATE_RUN ||
         ply_get_state() == PLY_MAIN_STATE_PAUSE) {
        ply_maindata = ply_get_maindata();
        ply_seek_stream_from_beginging((get->value / get->upper) * ply_maindata->duration_nanosecond);
    }

    if (av_sub_update_flag) {
        GList * c_list = NULL;
        c_list = g_list_append(c_list, "video0: h264");
        c_list = g_list_append(c_list, "video1: mpeg2");
        gtk_combo_set_popdown_strings (GTK_COMBO (video_combo), c_list);
        g_list_free(c_list);

        c_list = NULL;
        c_list = g_list_append(c_list, "audio0: AAC");
        c_list = g_list_append(c_list, "audio1: AC3");
        gtk_combo_set_popdown_strings (GTK_COMBO (audio_combo), c_list);
        g_list_free(c_list);

        c_list = NULL;
        c_list = g_list_append(c_list, "text0");
        c_list = g_list_append(c_list, "text1");
        gtk_combo_set_popdown_strings (GTK_COMBO (text_combo), c_list);
        g_list_free(c_list);

        av_sub_update_flag = 0;
    }
    
}

void ui_send_stop_signal(void)
{
    ui_stop_bt_cb(NULL, NULL);
}

void ui_update_progressbar(gint64 pos, gint64 len)
{
    PlyMainData *ply_maindata;
    gchar label_str[30];

    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(progressbar));
    if ( (len / GST_SECOND) != 0) {
        adj->value = ((pos / GST_SECOND) * 1000) / (len / GST_SECOND);
    }
    if ( (len / GST_SECOND) == 0 ||
         adj->value > 1000) {
        adj->value = 1000;
        pos = len;
    }

    if (ply_get_state() == PLY_MAIN_STATE_READY) {
        pos = 0;
        adj->value = 0;
    }

    ply_maindata = ply_get_maindata();
    ply_maindata->duration_nanosecond = len;

    gtk_signal_emit_by_name(GTK_OBJECT(adj), "changed");
    g_sprintf(label_str, "%02u:%02u:%02u/%02u:%02u:%02u",
              MY_GST_TIME_ARGS(pos), MY_GST_TIME_ARGS(len));
    gtk_label_set_text(GTK_LABEL(progress_time), label_str);
    g_print("%s\n", label_str);
    gtk_widget_show_all(progress_time);
}


gint ui_create(void)
{
    GtkWidget *topvbox;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, 720, 670);
    gtk_window_set_title(GTK_WINDOW(window), "UMMS Player");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);


    /* Create Top vertical box */
    topvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), topvbox);

    /* Create MenuBar */
    GtkWidget *menubar;
    GtkWidget *menuitem_file;
    GtkWidget *menuitem_play;
    GtkWidget *menuitem_help;
    GtkWidget *file_menu;
    GtkWidget *play_menu;
    GtkWidget *help_menu;

    gtk_widget_show_all(window);

    /* image widget */
    video_window = gtk_drawing_area_new();
    gtk_widget_set_size_request(video_window, 720, 576);
    gtk_box_pack_start(GTK_BOX(topvbox), video_window, FALSE, FALSE, 0);

    /* progress widget */
    GtkWidget *progress_hbox;
    progress_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topvbox), progress_hbox, TRUE, TRUE, 0);
    progressbar = gtk_hscale_new_with_range(0, 1000, 1);
    gtk_scale_set_digits(GTK_SCALE(progressbar), 0);
    gtk_scale_set_draw_value(GTK_SCALE(progressbar), FALSE);
    gtk_widget_set_size_request(progressbar, 32, 20);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(progressbar));
    gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(ui_progressbar_vchange_cb), adj);
    gtk_box_pack_start(GTK_BOX(progress_hbox), progressbar, TRUE, TRUE, 0);
    progress_time = gtk_label_new("00:00:00/00:00:00");
    gtk_box_pack_start(GTK_BOX(progress_hbox), progress_time, FALSE, FALSE, 0);

    /* AV/Text streams select and volume */
    GtkWidget *switch_hbox;
    switch_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topvbox), switch_hbox, TRUE, TRUE, 0);
    GtkWidget *volume_bar;

    video_combo = gtk_combo_new();
    gtk_combo_set_popdown_strings (GTK_COMBO (video_combo), NULL);
    gtk_box_pack_start(GTK_BOX(switch_hbox), video_combo, TRUE, TRUE, 0);

    audio_combo = gtk_combo_new();
    gtk_combo_set_popdown_strings (GTK_COMBO (audio_combo), NULL);
    gtk_box_pack_start(GTK_BOX(switch_hbox), audio_combo, TRUE, TRUE, 0);

    text_combo = gtk_combo_new();
    gtk_combo_set_popdown_strings (GTK_COMBO (text_combo), NULL);
    gtk_box_pack_start(GTK_BOX(switch_hbox), text_combo, TRUE, TRUE, 0);

    volume_bar = gtk_hscale_new_with_range(0, 100, 1);
    gtk_box_pack_start(GTK_BOX(switch_hbox), volume_bar, TRUE, TRUE, 0);
    gtk_widget_set_size_request(progressbar, 32, 20);

    /* Control Button */
    GtkWidget *button_hbox;
    GtkWidget *button_rewind;
    GtkWidget *button_stop;
    GtkWidget *button_forward;
    GtkWidget *button_fileopen;
    GtkWidget *image_rewind;
    GtkWidget *image_stop;
    GtkWidget *image_forward;
    GtkWidget *image_fileopen;
    GtkWidget *button_info;
    GtkWidget *button_option;

    button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topvbox), button_hbox, FALSE, FALSE, 0);
    button_rewind = gtk_button_new();
    image_rewind = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_rewind), image_rewind);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_rewind, TRUE, TRUE, 0);
    button_play = gtk_button_new();
    image_play = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
    image_pause = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
    g_object_ref((gpointer)image_play);
    g_object_ref((gpointer)image_pause);
    gtk_container_add(GTK_CONTAINER(button_play), image_play);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_play, TRUE, TRUE, 0);
    button_stop = gtk_button_new();
    image_stop = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_stop), image_stop);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_stop, TRUE, TRUE, 0);
    button_forward = gtk_button_new();
    image_forward = gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_forward), image_forward);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_forward, TRUE, TRUE, 0);
    button_fileopen = gtk_button_new();
    image_fileopen = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_fileopen), image_fileopen);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_fileopen, TRUE, TRUE, 0);

    g_signal_connect((gpointer)button_fileopen, "clicked", G_CALLBACK(ui_fileopen_dlg), NULL);
    g_signal_connect((gpointer)button_play, "clicked", G_CALLBACK(ui_pause_bt_cb), NULL);
    g_signal_connect((gpointer)button_stop, "clicked", G_CALLBACK(ui_stop_bt_cb), NULL);
    g_signal_connect((gpointer)button_rewind, "clicked", G_CALLBACK(ui_rewind_bt_cb), NULL);
    g_signal_connect((gpointer)button_forward, "clicked", G_CALLBACK(ui_forward_bt_cb), NULL);

    button_info = gtk_button_new_with_label("Info");
    button_option = gtk_button_new_with_label("Options");
    gtk_box_pack_start(GTK_BOX(button_hbox), button_info, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_option, TRUE, TRUE, 0);
    g_signal_connect((gpointer)button_info, "clicked", G_CALLBACK(ui_info_bt_cb), NULL);
    g_signal_connect((gpointer)button_option, "clicked", G_CALLBACK(ui_options_bt_cb), NULL);

    gtk_widget_show_all(window);

    return 0;
}

void ui_main_loop(void)
{
    gtk_main();
}
