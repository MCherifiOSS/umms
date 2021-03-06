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
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gst/gst.h>
#include "umms-gtk-player.h"
#include "umms-gtk-ui.h"
#include "gtk-backend.h"

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
static GtkWidget *volume_bar;

static GtkWidget *video_combo;
static GtkWidget *audio_combo;
static GtkWidget *text_combo;

GtkWidget *speed_lab;

#define UPDATE_AUDIO_TAG 1
#define UPDATE_VIDEO_TAG 1<<1
#define UPDATE_TEXT_TAG 1<<2
#define UPDATE_ALL UPDATE_AUDIO_TAG|UPDATE_VIDEO_TAG|UPDATE_TEXT_TAG

static void ui_update_audio_video_text(gint what)
{
  GList * c_list = NULL;
  gint i = 0;
  gchar *codec_name = NULL;
  gchar *show_name = NULL;

  if (what & UPDATE_VIDEO_TAG) {
    static int last_time_video_num = 0;
    gint video_num = ply_get_video_num();
    gint cur_video = ply_get_cur_video();

    for (i = 0; i < last_time_video_num; i++)
      gtk_combo_box_remove_text(GTK_COMBO_BOX(video_combo), 0);

    last_time_video_num = video_num;

    for (i = 0; i < video_num; i++) {
      codec_name = ply_get_video_codec(i);
      if (!codec_name || !strcmp(codec_name, "")) {
        show_name = g_strdup_printf("Video %d: Format not known", i);
      } else {
        show_name = g_strdup_printf("Video %d: %s", i, codec_name);
      }
      g_free(codec_name);

      if (i == cur_video) {
        gtk_combo_box_prepend_text(GTK_COMBO_BOX(video_combo), show_name);
      } else {
        gtk_combo_box_append_text(GTK_COMBO_BOX(video_combo), show_name);
      }

      g_free(show_name);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX(video_combo), cur_video);
  }


  if (what & UPDATE_AUDIO_TAG) {
    static int last_time_audio_num = 0;
    gint audio_num = ply_get_audio_num();
    gint cur_audio = ply_get_cur_audio();

    for (i = 0; i < last_time_audio_num; i++)
      gtk_combo_box_remove_text(GTK_COMBO_BOX(audio_combo), 0);

    last_time_audio_num = audio_num;

    for (i = 0; i < audio_num; i++) {
      codec_name = ply_get_audio_codec(i);
      if (codec_name == NULL) {// || !strcmp(codec_name, "")) {
        show_name = g_strdup_printf("Audio %d: Format not known", i);
      } else {
        show_name = g_strdup_printf("Audio %d: %s", i, codec_name);
      }
      g_free(codec_name);

      if (i == cur_audio) {
        gtk_combo_box_prepend_text(GTK_COMBO_BOX(audio_combo), show_name);
      } else {
        gtk_combo_box_append_text(GTK_COMBO_BOX(audio_combo), show_name);
      }

      g_free(show_name);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX(audio_combo), cur_audio);
  }
}


/* The callback for update status. */
void ui_callbacks_for_reason(UI_CALLBACK_REASONS reason, void * data1, void * data2)
{
  printf("-------callback come for reason :%d\n", reason);
  switch (reason) {
  case UI_CALLBACK_STATE_CHANGE: {
    if ((int)data1 == 2) {// Playing
      gint play_speed;
      char *str;

      play_speed = ply_get_play_speed();
      str = g_strdup_printf("  Play Speed: X %d   ", play_speed);
      gtk_container_remove(GTK_CONTAINER(button_play), image_play);
      gtk_container_add(GTK_CONTAINER(button_play), image_pause);
      gtk_label_set_text(GTK_LABEL(speed_lab), str);
      ply_set_state(PLY_MAIN_STATE_RUN);
      ui_update_audio_video_text(UPDATE_ALL);
      g_free(str);
    } else {
      gtk_container_remove(GTK_CONTAINER(button_play), image_pause);
      gtk_container_add(GTK_CONTAINER(button_play), image_play);
      gtk_label_set_text(GTK_LABEL(speed_lab), "  Play Speed: X 0   ");
      if ((int)data1 == 1) {
        ply_set_state(PLY_MAIN_STATE_PAUSE);
      } else if ((int)data1 == 3) {
        ply_set_state(PLY_MAIN_STATE_READY);
      } else {
        ply_set_state(PLY_MAIN_STATE_IDLE);
      }
    }
  }
  break;

  case UI_CALLBACK_EOF:
    break;

  case UI_CALLBACK_BUFFERING:
    break;

  case UI_CALLBACK_BUFFERED:
    break;

  case UI_CALLBACK_SEEKED: {
    char *str;
    gint play_speed;

    play_speed = ply_get_play_speed();

    str = g_strdup_printf("  Play Speed: X %d   ", play_speed);
    gtk_label_set_text(GTK_LABEL(speed_lab), str);
    g_free(str);
    ply_set_speed(play_speed);
  }
  break;

  case UI_CALLBACK_STOPPED:
    gtk_label_set_text(GTK_LABEL(speed_lab), "  Play Speed: X 0   ");
    gtk_container_remove(GTK_CONTAINER(button_play), image_pause);
    gtk_container_add(GTK_CONTAINER(button_play), image_play);
    ply_set_state(PLY_MAIN_STATE_READY);
    break;

  case UI_CALLBACK_ERROR:
    break;

  case UI_CALLBACK_VIDEO_TAG_CHANGED:
    ui_update_audio_video_text(UPDATE_VIDEO_TAG);
    break;

  case UI_CALLBACK_AUDIO_TAG_CHANGED:
    ui_update_audio_video_text(UPDATE_AUDIO_TAG);
    break;

  case UI_CALLBACK_TEXT_TAG_CHANGED:
    break;

  default:
    printf("Error, should not come!!!!\n");
    return;
  }

  gtk_widget_show_all(window);
  return;
}

static int ui_create_socket(void)
{
  int sockfd;
  struct sockaddr_in server_addr;
  gchar * ip_address;
  gint port;
  int flags;

  //host = gethostbyname("localhost");
  ply_get_rawdata_address(&ip_address, &port);
  printf("We get the socket address, IP: %s, port: %d\n", ip_address, port);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Socket Error:%s\a\n", (char *)strerror(errno));
    return -1;
  }


  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  //server_addr.sin_addr = *((struct in_addr*)host->h_addr);
  server_addr.sin_addr.s_addr = inet_addr(ip_address);
  g_free(ip_address);


  if (connect (sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1) {
    printf("Connect Error: %s\a\n", strerror(errno));
    close(sockfd);
    return -1;
  }

  flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

  return sockfd;
}

static int ui_socket_fd = -1;

static char * ui_get_raw_data(gint max_len, gint * ret_len)
{
  char buffer[1024];
  fd_set fds;
  int nbytes;
  int i;
  int maxfdp;
  struct timeval tv;
  gchar* ret_str;

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  while (1) {
    FD_ZERO(&fds);
    FD_SET(ui_socket_fd, &fds);
    maxfdp = ui_socket_fd + 1;

    switch (select(maxfdp, &fds, NULL, NULL, &tv)) {
    case -1:
      *ret_len = 0;
      return NULL;
      break;

    case 0:
      break;

    default: {
      if (FD_ISSET(ui_socket_fd, &fds)) {
        nbytes = read(ui_socket_fd, buffer, 1024);

        /*
        printf( "received:%d bytes\n", nbytes);
        if (nbytes >= 0) {
            buffer[nbytes] = '\0';
            printf( "I have received:%x %x %x %x %x %x %x %x\n",
            buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],
            buffer[5], buffer[6], buffer[7]);
        }*/

        if (max_len <= 0)
          return; //do nothing

        max_len = (nbytes <= max_len) ? nbytes : max_len;
        ret_str = g_malloc(sizeof(gchar) * max_len);
        memcpy(ret_str, buffer, sizeof(gchar)*max_len);
        *ret_len = max_len;
        return ret_str;
      }
    }
    }
  }
  *ret_len = 0;
  return NULL;
}



static void ui_pause_bt_cb(GtkWidget *widget, gpointer data)
{
  //printf("the status is %d\n", ply_get_state());
  if (ply_get_state() == PLY_MAIN_STATE_READY) {
    ply_play_stream();
  } else if (ply_get_state() == PLY_MAIN_STATE_RUN) {
    ply_pause_stream();
  } else if (ply_get_state() == PLY_MAIN_STATE_PAUSE) {
    ply_resume_stream();
  }
  gtk_widget_show_all(window);
}

static void ui_stop_bt_cb(GtkWidget *widget, gpointer data)
{
  /* can stop anyway. */
  ply_stop_stream();
}

static void ui_forward_bt_cb(GtkWidget *widget, gpointer data)
{
  gint speed = ply_get_speed();
  if (speed <= 0) {
    speed = 1;
  } else {
    speed = speed * 2;
  }

  if (ply_get_state() == PLY_MAIN_STATE_RUN) {
    ply_forward_rewind(speed);
  }
}

static void ui_rewind_bt_cb(GtkWidget *widget, gpointer data)
{
  gint speed = ply_get_speed();
  if (speed >= 0) {
    speed = -1;
  } else {
    speed = speed * 2;
  }

  if (ply_get_state() == PLY_MAIN_STATE_RUN) {
    ply_forward_rewind(speed);
  }
}

static void ui_uri_open_bt_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *uri_dlg;
  GtkWidget *uri_entry;
  GtkWidget *sub_entry;
  gint response_id;
  gchar *entry_text = NULL;
  gchar *sub_entry_text = NULL;
  GtkWidget *frame;

  uri_dlg = gtk_dialog_new_with_buttons("Enter the URI",
                                        GTK_WINDOW(gtk_widget_get_toplevel (window)),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);

  gtk_window_set_default_size (GTK_WINDOW (uri_dlg), 500, 200);

  frame = gtk_frame_new ("Type the URI:");

  uri_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (uri_entry), 500);

  gtk_container_add (GTK_CONTAINER (frame), uri_entry);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (uri_dlg)->vbox),
                      frame, FALSE, FALSE, 0);

  frame = gtk_frame_new ("Type the Subtitle URI:");

  sub_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (sub_entry), 500);

  gtk_container_add (GTK_CONTAINER (frame), sub_entry);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (uri_dlg)->vbox),
                      frame, FALSE, FALSE, 0);

  gtk_widget_show_all (uri_dlg);

  response_id = gtk_dialog_run(GTK_DIALOG(uri_dlg));

  if (response_id == GTK_RESPONSE_OK) {
    entry_text = g_strdup(gtk_entry_get_text (GTK_ENTRY (uri_entry)));
    sub_entry_text = g_strdup(gtk_entry_get_text (GTK_ENTRY (sub_entry)));
    //printf("Entry contents: %s\n", entry_text);
  }

  gtk_widget_destroy(uri_dlg);

  if (entry_text && entry_text[0]) {
    ply_reload_file(entry_text);
    if (sub_entry_text && sub_entry_text[0]) {
      ply_set_subtitle(sub_entry_text);
    }

    ui_pause_bt_cb(NULL, NULL);
    g_free(entry_text);
  }
}

static void ui_sub_open_bt_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *subopen_dlg;
  gint result;
  gchar *filename;

  subopen_dlg = gtk_file_chooser_dialog_new("Select Subtitle file",
                GTK_WINDOW(window),
                GTK_FILE_CHOOSER_ACTION_OPEN,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_OK, GTK_RESPONSE_OK,
                NULL);
  result = gtk_dialog_run(GTK_DIALOG(subopen_dlg));
  if (GTK_RESPONSE_OK == result) {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(subopen_dlg));
    gtk_widget_destroy(subopen_dlg);
    g_print("subtitle filename is %s\n", filename);
    ply_set_subtitle(filename);
    g_free(filename);
  } else {
    gtk_widget_destroy(subopen_dlg);
  }

}

static void ui_update_dvb_rawdata_thread(GtkWidget * uri_entry)
{
  gchar * raw_data = NULL;
  gchar * show_data = NULL;
  gint ret_len = 0;
  gchar num_buf[16];

  if (ui_socket_fd == -1) {
    ui_socket_fd = ui_create_socket();
  }

  if (ui_socket_fd != -1) {
    int i;
    struct timeval tv;
    long tv_sec;

    gettimeofday (&tv, NULL);
    tv_sec = tv.tv_sec;

    while (1) {
      gchar * old = NULL;
      raw_data = ui_get_raw_data(8, &ret_len);
      if (raw_data == NULL)
        break;

      /* Do not append to quick. */
      gettimeofday (&tv, NULL);
      if (tv_sec == tv.tv_sec )
        continue;

      gtk_editable_delete_text(GTK_EDITABLE(uri_entry), 0, -1);

      tv_sec = tv.tv_sec;
      show_data = NULL;

      for (i = 0; i < ret_len; i++) {
        old = show_data;
        sprintf(num_buf, "%2X", (char)raw_data[i]);

        if (show_data) {
          show_data = g_strdup_printf("%s %s", old, num_buf);
        } else {
          show_data = g_strdup(num_buf);
        }
        //printf("--- the num_buf = %s\n", num_buf);

        if (old)
          g_free(old);
      }

      gtk_entry_set_text(GTK_ENTRY(uri_entry), show_data);
      g_free(raw_data);
    }
  }
}

static void ui_dvb_bt_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *dlg;
  gint result;
  GtkWidget *frame;
  GtkWidget *lab;
  GtkWidget *uri_entry;
  GThread *raw_data_update_thread;
  GArray *pat;
  GArray *pmt;
  gchar * show_str = NULL;
  guint program_num;
  guint pcr_pid;
  int i;

  if (ply_get_state() != PLY_MAIN_STATE_RUN &&
      ply_get_state() != PLY_MAIN_STATE_PAUSE) {
    return;
  }

  dlg = gtk_dialog_new_with_buttons("DVB Infomation",
                                    GTK_WINDOW(gtk_widget_get_toplevel (window)),
                                    GTK_DIALOG_MODAL,
                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                    NULL);
  gtk_window_set_default_size (GTK_WINDOW ((dlg)), 600, 500);

  frame = gtk_frame_new ("Raw Data Dump:");
  uri_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (uri_entry), 100);
  gtk_container_add (GTK_CONTAINER (frame), uri_entry);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                      frame, FALSE, FALSE, 0);

  /* Get the PAT and PMT. */
  pat = ply_get_pat();
  //printf("PAT array length is %d\n",  pat->len);

  frame = gtk_frame_new ("Program  <----------->   PID");
  show_str = NULL;
  for (i = 0; i < pat->len; i++) {
    struct UMMS_Pat_Item item = g_array_index(pat, UMMS_Pat_Item, i);

    if (show_str) {
      gchar * old = show_str;
      show_str = g_strdup_printf("%s\nProgram Number: %u ----  PID: %u",
                                 show_str, item.program_num, item.pid);
      g_free(old);
    } else {
      show_str = g_strdup_printf("Program Number: %u ----  PID: %u",
                                 item.program_num, item.pid);
    }
  }

  lab = gtk_label_new(NULL);
  gtk_label_set_text(GTK_LABEL(lab), show_str);
  gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), lab);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  g_array_free(pat, TRUE);
  g_free(show_str);


  if (ply_get_pmt(&program_num, &pcr_pid, &pmt) == 0) {
    frame = gtk_frame_new ("Current Program Infomation:");
    show_str = g_strdup_printf("Program Number: %u     PCR PID: %u",
                               program_num, pcr_pid);
    for (i = 0; i < pmt->len; i++) {
      gchar * old = NULL;
      struct UMMS_Pmt_Item item = g_array_index(pmt, UMMS_Pmt_Item, i);

      old = show_str;
      show_str = g_strdup_printf("%s\nPID: %u ----  Stream Type: %u",
                                 show_str, item.pid, item.stream_type);
      g_free(old);
    }

    lab = gtk_label_new(NULL);
    gtk_label_set_text(GTK_LABEL(lab), show_str);
    gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
    gtk_container_add (GTK_CONTAINER (frame), lab);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                        frame, FALSE, FALSE, 0);
    g_array_free(pmt, TRUE);
    g_free(show_str);
  }

  gtk_widget_show_all (dlg);

  raw_data_update_thread =
    g_thread_create ((GThreadFunc) ui_update_dvb_rawdata_thread, uri_entry, TRUE, NULL);

  gtk_dialog_run(GTK_DIALOG(dlg));

  close(ui_socket_fd);
  g_thread_join(raw_data_update_thread);
  ui_socket_fd = -1;

  gtk_widget_destroy(dlg);

}

static void ui_info_bt_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *info_dlg;
  GtkWidget *lab;
  GtkWidget *frame;

  gchar * container_name = NULL;
  gchar * codec_name = NULL;
  gchar * show_name = NULL;
  gchar * lab_str = NULL;
  gchar * uri_str = NULL;
  gchar * prot_str = NULL;
  gint video_num = 0;
  gint audio_num = 0;
  int i = 0;

  if (ply_get_state() != PLY_MAIN_STATE_RUN &&
      ply_get_state() != PLY_MAIN_STATE_PAUSE) {
    return;
  }

  info_dlg = gtk_dialog_new_with_buttons("Media Info",
                                         GTK_WINDOW(gtk_widget_get_toplevel (window)),
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                         NULL);

  gtk_window_set_default_size (GTK_WINDOW (info_dlg), 400, 400);

  /* The uri and protocol. */
  frame = gtk_frame_new ("Source");
  lab = gtk_label_new(NULL);
  uri_str = ply_get_current_uri();
  prot_str = ply_get_protocol_name();
  show_name = g_strdup_printf("URI: %s\nProtocol: %s", uri_str, prot_str);
  gtk_label_set_text(GTK_LABEL(lab), show_name);
  gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), lab);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  g_free(uri_str);
  g_free(prot_str);
  g_free(show_name);
  show_name = NULL;

  /* Add the media info here. */
  frame = gtk_frame_new ("Container Type");
  lab = gtk_label_new(NULL);
  container_name = ply_get_container_name();
  gtk_label_set_text(GTK_LABEL(lab), container_name);
  gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), lab);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  g_free(container_name);

  frame = gtk_frame_new ("Video");
  video_num = ply_get_video_num();
  lab_str = NULL;
  for (i = 0; i < video_num; i++) {
    gint video_bitrate = 0;
    gint width = 0;
    gint height = 0;
    gint a_width = 0;
    gint a_height = 0;
    gdouble video_framerate = 0.0;
    gchar * total_info = NULL;

    codec_name = ply_get_video_codec(i);
    if (!codec_name || !strcmp(codec_name, "")) {
      show_name = g_strdup_printf("video_%d\n |==== Codec: not known", i);
    } else {
      show_name = g_strdup_printf("video_%d\n |==== Codec: %s", i, codec_name);
    }
    g_free(codec_name);

    video_bitrate = ply_get_video_bitrate(i);

    video_framerate = ply_get_video_framerate(i);

    ply_get_video_resolution(i, &width, &height);

    ply_get_video_aspectratio(i, &a_width, &a_height);

    total_info = g_strdup_printf("%s,   Bitrate: %d,   FrameRate: %4.2lf,"
                                 "  Resolution: %d X %d   AspectRatio: %d:%d ====|\n",
                                 show_name, video_bitrate, video_framerate, width, height, a_width, a_height);
    g_free(show_name);

    if (lab_str) {
      gchar * old = lab_str;
      lab_str = g_strdup_printf("%s\n%s", old, total_info);
      g_free(old);
      g_free(total_info);
    } else {
      lab_str = total_info;
    }
  }

  lab = gtk_label_new(NULL);
  gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_LEFT);
  gtk_label_set_text(GTK_LABEL(lab), lab_str);
  gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), lab);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  g_free(lab_str);


  frame = gtk_frame_new ("Audio");
  audio_num = ply_get_audio_num();
  lab_str = NULL;
  for (i = 0; i < audio_num; i++) {
    gchar * total_info = NULL;
    gint audio_bitrate = 0;
    gint audio_samplerate = 0;

    codec_name = ply_get_audio_codec(i);
    if (!codec_name || !strcmp(codec_name, "")) {
      show_name = g_strdup_printf("audio_%d\n |==== Codec: not known", i);
    } else {
      show_name = g_strdup_printf("audio_%d\n |==== Codec: %s", i, codec_name);
    }
    g_free(codec_name);

    audio_bitrate = ply_get_audio_bitrate(i);
    audio_samplerate = ply_get_audio_samplerate(i);

    total_info = g_strdup_printf("%s   BitRate: %d   Sample Rate: %d  ====|\n",
                                 show_name, audio_bitrate, audio_samplerate);
    g_free(show_name);

    if (lab_str) {
      gchar * old = lab_str;
      lab_str = g_strdup_printf("%s\n%s", old, total_info);
      g_free(total_info);
      g_free(old);
    } else {
      lab_str = total_info;
    }
  }

  lab = gtk_label_new(NULL);
  gtk_label_set_justify(GTK_LABEL(lab), GTK_JUSTIFY_LEFT);
  gtk_label_set_text(GTK_LABEL(lab), lab_str);
  gtk_label_set_line_wrap (GTK_LABEL(lab), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), lab);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info_dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  g_free(lab_str);

  gtk_widget_show_all (info_dlg);

  gtk_dialog_run(GTK_DIALOG(info_dlg));
  gtk_widget_destroy(info_dlg);
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
  } else {
    gtk_widget_destroy(fileopen_dlg);
  }
}

static void ui_progressbar_vchange_cb( GtkAdjustment *get,
                                       GtkAdjustment *set )
{
  if (ply_get_state() == PLY_MAIN_STATE_RUN ||
      ply_get_state() == PLY_MAIN_STATE_PAUSE) {
    ply_seek_stream_from_beginging((get->value / get->upper) * ply_get_duration());
  }
}

static void ui_volumebar_vchange_cb( GtkAdjustment *get,
                                     GtkAdjustment *set )
{
  gint volume;

  if (ply_get_state() == PLY_MAIN_STATE_RUN ||
      ply_get_state() == PLY_MAIN_STATE_PAUSE) {
    printf("Volume:  get->value = %d\n", (gint)(get->value));
    ply_set_volume((gint)get->value);

    volume = ply_get_volume();
    //printf("///// The volume now is %d\n", volume);
    gtk_range_set_value(GTK_RANGE(volume_bar), volume);
  } else {
    gtk_range_set_value(GTK_RANGE(volume_bar), 0.0);
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

  //printf("##### pos = %lld, len = %lld\n", pos, len);
  GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(progressbar));
  if ( (len) != 0) {
    adj->value = ((pos) * 1000) / (len);
  }
  if ( (len) == 0 ||
       adj->value > 1000) {
    adj->value = 1000;
    pos = len;
  }

  if (ply_get_state() == PLY_MAIN_STATE_READY) {
    pos = 0;
    adj->value = 0;
  }

  ply_set_duration(len);

  gtk_signal_emit_by_name(GTK_OBJECT(adj), "changed");
  g_sprintf(label_str, "%02u:%02u:%02u/%02u:%02u:%02u",
            MY_GST_TIME_ARGS(pos*GST_SECOND / 1000), MY_GST_TIME_ARGS(len*GST_SECOND / 1000));
  gtk_label_set_text(GTK_LABEL(progress_time), label_str);
  g_print("%s\n", label_str);
  gtk_widget_show_all(progress_time);
}


static gboolean video_combo_changed(GtkComboBox *comboBox, GtkLabel *label)
{
  int video_num = 0;
  gchar *active = gtk_combo_box_get_active_text(comboBox);
  //printf("------------ the active is %s\n", active);

  if (active) {
    sscanf(active, "Video %d:", &video_num);
    g_free(active);
  }

  //printf("-------- the number we want to set is %d\n", video_num);
  ply_set_cur_video(video_num);
}

static gboolean audio_combo_changed(GtkComboBox *comboBox, GtkLabel *label)
{
  int audio_num = 0;
  gchar *active = gtk_combo_box_get_active_text(comboBox);
  //printf("------------ the active is %s\n", active);

  if (active) {
    sscanf(active, "Audio %d:", &audio_num);
    g_free(active);
  }

  //printf("-------- the number we want to set is %d\n", audio_num);
  ply_set_cur_audio(audio_num);
}

static gboolean text_combo_changed(GtkComboBox *comboBox, GtkLabel *label)
{
  int text_num = 0;
  gchar *active = gtk_combo_box_get_active_text(comboBox);
  //printf("------------ the active is %s\n", active);

  if (active) {
    sscanf(active, "Text %d:", &text_num);
    g_free(active);
  }

  //printf("-------- the number we want to set is %d\n", text_num);
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
  GtkWidget *table;

  gtk_widget_show_all(window);

  /* image widget */
  video_window = gtk_drawing_area_new();
  gtk_widget_set_size_request(video_window, 720, 576);
  gtk_box_pack_start(GTK_BOX(topvbox), video_window, FALSE, FALSE, 0);

  /* progress widget */
  GtkWidget *progress_hbox;
  progress_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(topvbox), progress_hbox, TRUE, TRUE, 0);
  speed_lab = gtk_label_new(NULL);
  gtk_label_set_text(GTK_LABEL(speed_lab), "  Play Speed: X 1   ");
  gtk_label_set_justify(GTK_LABEL(speed_lab), GTK_JUSTIFY_FILL);
  gtk_label_set_line_wrap (GTK_LABEL(speed_lab), FALSE);
  gtk_box_pack_start(GTK_BOX(progress_hbox), speed_lab, FALSE, FALSE, 0);

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
  table = gtk_table_new (2, 15, TRUE);
  gtk_box_pack_start(GTK_BOX(topvbox), table, TRUE, TRUE, 0);

  video_combo = gtk_combo_box_new_text();
  gtk_table_attach_defaults (GTK_TABLE (table), video_combo, 0, 4, 0, 1);
  g_signal_connect(GTK_OBJECT(video_combo),
                   "changed", G_CALLBACK(video_combo_changed), NULL);

  audio_combo = gtk_combo_box_new_text();
  gtk_table_attach_defaults (GTK_TABLE (table), audio_combo, 4, 8, 0, 1);
  g_signal_connect(GTK_OBJECT(audio_combo),
                   "changed", G_CALLBACK(audio_combo_changed), NULL);

  text_combo = gtk_combo_box_new_text();
  gtk_table_attach_defaults (GTK_TABLE (table), text_combo, 8, 12, 0, 1);
  g_signal_connect(GTK_OBJECT(text_combo),
                   "changed", G_CALLBACK(text_combo_changed), NULL);

  volume_bar = gtk_hscale_new_with_range(0, 100, 1);
  adj = gtk_range_get_adjustment(GTK_RANGE(volume_bar));
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(ui_volumebar_vchange_cb), adj);
  gtk_table_attach_defaults (GTK_TABLE (table), volume_bar, 12, 15, 0, 1);

  /* Control Button */
  GtkWidget *button_hbox;
  GtkWidget *button_rewind;
  GtkWidget *button_stop;
  GtkWidget *button_forward;
  GtkWidget *button_fileopen;
  GtkWidget *button_uriopen;
  GtkWidget *button_dvb;
  GtkWidget *button_subtitle_open;
  GtkWidget *image_rewind;
  GtkWidget *image_stop;
  GtkWidget *image_forward;
  GtkWidget *image_fileopen;
  GtkWidget *image_uri;
  GtkWidget *image_info;
  GtkWidget *button_info;
  GtkWidget *button_option;
  gint v_start = 0;

  //button_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(topvbox), button_hbox, FALSE, FALSE, 0);
  button_rewind = gtk_button_new();
  image_rewind = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_rewind), image_rewind);
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_rewind, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_rewind, v_start++, v_start + 1, 1, 2);

  button_play = gtk_button_new();
  image_play = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
  image_pause = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
  g_object_ref((gpointer)image_play);
  g_object_ref((gpointer)image_pause);
  gtk_container_add(GTK_CONTAINER(button_play), image_play);
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_play, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_play, v_start++, v_start + 1, 1, 2);

  button_stop = gtk_button_new();
  image_stop = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_stop), image_stop);
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_stop, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_stop, v_start++, v_start + 1, 1, 2);

  button_forward = gtk_button_new();
  image_forward = gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_forward), image_forward);
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_forward, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_forward, v_start++, v_start + 1, 1, 2);

  button_fileopen = gtk_button_new();
  image_fileopen = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_fileopen), image_fileopen);
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_fileopen, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_fileopen, v_start++, v_start + 1, 1, 2);

  button_uriopen = gtk_button_new();
  image_uri = gtk_image_new_from_stock("gtk-file", GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_uriopen), image_uri);
  gtk_table_attach_defaults (GTK_TABLE (table), button_uriopen, v_start++, v_start + 1, 1, 2);

  button_subtitle_open = gtk_button_new_with_label("Sub");
  gtk_table_attach_defaults (GTK_TABLE (table), button_subtitle_open, v_start++, v_start + 1, 1, 2);

  button_dvb = gtk_button_new_with_label("DVB");
  gtk_table_attach_defaults (GTK_TABLE (table), button_dvb, v_start++, v_start + 1, 1, 2);

  g_signal_connect((gpointer)button_fileopen, "clicked", G_CALLBACK(ui_fileopen_dlg), NULL);
  g_signal_connect((gpointer)button_play, "clicked", G_CALLBACK(ui_pause_bt_cb), NULL);
  g_signal_connect((gpointer)button_stop, "clicked", G_CALLBACK(ui_stop_bt_cb), NULL);
  g_signal_connect((gpointer)button_rewind, "clicked", G_CALLBACK(ui_rewind_bt_cb), NULL);
  g_signal_connect((gpointer)button_forward, "clicked", G_CALLBACK(ui_forward_bt_cb), NULL);
  g_signal_connect((gpointer)button_uriopen, "clicked", G_CALLBACK(ui_uri_open_bt_cb), NULL);
  g_signal_connect((gpointer)button_subtitle_open, "clicked", G_CALLBACK(ui_sub_open_bt_cb), NULL);
  g_signal_connect((gpointer)button_dvb, "clicked", G_CALLBACK(ui_dvb_bt_cb), NULL);

  button_info = gtk_button_new();
  image_info = gtk_image_new_from_stock("gtk-info", GTK_ICON_SIZE_BUTTON);
  gtk_container_add(GTK_CONTAINER(button_info), image_info);

  button_option = gtk_button_new_with_label("Options");
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_info, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_info, v_start, v_start + 2, 1, 2);
  v_start += 2;
  //gtk_box_pack_start(GTK_BOX(button_hbox), button_option, TRUE, TRUE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), button_option, v_start, v_start + 2, 1, 2);
  v_start += 2;
  g_signal_connect((gpointer)button_info, "clicked", G_CALLBACK(ui_info_bt_cb), NULL);
  g_signal_connect((gpointer)button_option, "clicked", G_CALLBACK(ui_options_bt_cb), NULL);

  gtk_widget_show_all(window);

  return 0;
}

void ui_main_loop(void)
{
  gtk_main();
}
