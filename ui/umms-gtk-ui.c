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
        ply_seek_stream_from_beginging((get->value/get->upper) * ply_maindata->duration_nanosecond);
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
    if( (len/GST_SECOND) != 0)
    {
        adj->value = ((pos/GST_SECOND) * 1000) / (len/GST_SECOND);
    }
    if ( (len/GST_SECOND) == 0 ||
            adj->value > 1000)
    {
        adj->value = 1000;
        pos = len;
    }

    if (ply_get_state()== PLY_MAIN_STATE_READY) {
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
    gtk_widget_set_size_request(window, 640, 480);
    gtk_window_set_title(GTK_WINDOW(window), "AdvPlayer");
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
    menubar  = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(topvbox), menubar, FALSE, TRUE, 0);
    menuitem_file = gtk_menu_item_new_with_mnemonic("File(_F)");
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menuitem_file);
    file_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_file), file_menu);
    GtkWidget *menuitem_file1 = gtk_menu_item_new_with_mnemonic("File(_F)");
    gtk_container_add(GTK_CONTAINER(file_menu), menuitem_file1);

    menuitem_play = gtk_menu_item_new_with_mnemonic("Play(_P)");
    //gtk_container_add(GTK_CONTAINER(menubar), menuitem_play);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menuitem_play);
    play_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_play), play_menu);

    menuitem_help = gtk_menu_item_new_with_label("Help(_P)");
    //gtk_container_add(GTK_CONTAINER(menubar), menuitem_help);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menuitem_help);
    help_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem_help), help_menu);
    gtk_widget_show_all(window);

    /* image widget */
    video_window = gtk_drawing_area_new();
    gtk_widget_set_size_request(video_window,640,380); 
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
    button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topvbox), button_hbox, FALSE, FALSE, 0);
    button_rewind = gtk_button_new();
    image_rewind = gtk_image_new_from_stock("gtk-media-rewind", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_rewind), image_rewind);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_rewind, TRUE, TRUE, 0);
    button_play = gtk_button_new();
    image_play = gtk_image_new_from_stock("gtk-media-play", GTK_ICON_SIZE_BUTTON);
    image_pause = gtk_image_new_from_stock("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
    g_object_ref((gpointer)image_play);
    g_object_ref((gpointer)image_pause);
    gtk_container_add(GTK_CONTAINER(button_play), image_play);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_play, TRUE, TRUE, 0);
    button_stop = gtk_button_new();
    image_stop = gtk_image_new_from_stock("gtk-media-stop", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_stop), image_stop);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_stop, TRUE, TRUE, 0);
    button_forward = gtk_button_new();
    image_forward = gtk_image_new_from_stock("gtk-media-forward", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_forward), image_forward);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_forward, TRUE, TRUE, 0);
    button_fileopen = gtk_button_new();
    image_fileopen = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(button_fileopen), image_fileopen);
    gtk_box_pack_start(GTK_BOX(button_hbox), button_fileopen, TRUE, TRUE, 0);

    g_signal_connect((gpointer)button_fileopen, "clicked", G_CALLBACK(ui_fileopen_dlg), NULL);
    g_signal_connect((gpointer)button_play, "clicked", G_CALLBACK(ui_pause_bt_cb), NULL);
    g_signal_connect((gpointer)button_stop, "clicked", G_CALLBACK(ui_stop_bt_cb), NULL);

    gtk_widget_show_all(window);


    return 0;
}

void ui_main_loop(void)
{
    gtk_main();
}
