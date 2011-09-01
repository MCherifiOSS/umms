/*
 * Copyright (C) 2008 Felipe Contreras.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <string.h>

#include "gst-backend.h"

static char *uri_to_play;
static GtkWidget *video_output;
static GtkWidget *pause_button;
static GtkWidget *scale;
static guint64 duration;
static GtkWidget *window;
static GtkWidget * subwin;

static guint64 _step = 97*1000000000;

#define DURATION_IS_VALID(x) (x != 0 && x != (guint64) -1)

static void
toggle_paused (void)
{
    static gboolean paused = FALSE;
    if (paused)
    {
        umms_backend_resume ();
        gtk_button_set_label (GTK_BUTTON (pause_button), "Pause");
        paused = FALSE;
    }
    else
    {
        umms_backend_pause ();
        gtk_button_set_label (GTK_BUTTON (pause_button), "Resume");
        paused = TRUE;
    }
}

static void
toggle_fullscreen (void)
{
    static gboolean fullscreen = FALSE;
    if (fullscreen)
    {
        gtk_window_unfullscreen (window);
        fullscreen = FALSE;
    }
    else
    {
        gtk_window_fullscreen (window);
        fullscreen = TRUE;
    }
}

static void
pause_cb (GtkWidget *widget,
          gpointer data)
{
    toggle_paused ();
}

static void
reset_cb (GtkWidget *widget,
          gpointer data)
{
    backend_reset ();
}

static gboolean
delete_event (GtkWidget *widget,
              GdkEvent *event,
              gpointer data)
{
    return FALSE;
}

static void
destroy (GtkWidget *widget,
         gpointer data)
{
    gtk_main_quit ();
}

static gboolean
key_press (GtkWidget *widget,
           GdkEventKey *event,
           gpointer data)
{
    switch (event->keyval)
    {
        case GDK_P:
        case GDK_p:
        case GDK_space:
            toggle_paused ();
            break;
        case GDK_F:
        case GDK_f:
            toggle_fullscreen ();
            break;
        case GDK_R:
        case GDK_r:
            umms_backend_reset ();
            break;
        case GDK_Right:
            umms_backend_seek (10);
            break;
        case GDK_Left:
            umms_backend_seek (-10);
            break;
        case GDK_Q:
        case GDK_q:
            gtk_main_quit ();
            break;
        default:
            break;
    }

    return TRUE;
}

static void
seek_cb (GtkRange *range,
         GtkScrollType scroll,
         gdouble value,
         gpointer data)
{
    guint64 to_seek;

    if (!DURATION_IS_VALID (duration))
        duration = backend_query_duration ();

    if (!DURATION_IS_VALID (duration))
        return;

    to_seek = (value / 100) * duration;

#if 0
    g_print ("value: %f\n", value);
    g_print ("duration: %llu\n", duration);
    g_print ("seek: %llu\n", to_seek);
#endif
    umms_backend_seek_absolute (to_seek);
}

static void
seekforward_cb (GtkRange *range,
         GtkScrollType scroll,
         gdouble value,
         gpointer data)
{
  guint64 to_seek;
  guint64 pos;

  if (!DURATION_IS_VALID (duration))
    duration = backend_query_duration ();

  if (!DURATION_IS_VALID (duration))
    return;

  pos = backend_query_position ();

  if (pos + _step < duration)
    to_seek = pos + _step;
  else
    to_seek = duration;

  umms_backend_seek_absolute (to_seek);
}

static void
seekbackward_cb (GtkRange *range,
                GtkScrollType scroll,
                gdouble value,
                gpointer data)
{
  guint64 to_seek;
  guint64 pos;

  if (!DURATION_IS_VALID (duration))
    duration = backend_query_duration ();

  if (!DURATION_IS_VALID (duration))
    return;

  pos = backend_query_position ();

  if (pos - _step > 0)
    to_seek = pos - _step;
  else
    to_seek = 0;

  backend_seek_absolute (to_seek);
}

static void
seekto485s_cb (GtkRange *range,
                 GtkScrollType scroll,
                 gdouble value,
                 gpointer data)
{
  guint64 to_seek;

  if (!DURATION_IS_VALID (duration))
    duration = backend_query_duration ();

  /*if (!DURATION_IS_VALID (duration))
    return;*/

  to_seek = 485000000000;

  backend_seek_absolute (to_seek);
}

static void
start (void)
{
    GtkWidget *button;
    GtkWidget *hbox;
    GtkWidget *vbox;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event), NULL);

    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy), NULL);

    g_signal_connect (G_OBJECT (window), "key-press-event",
                      G_CALLBACK (key_press), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    vbox = gtk_vbox_new (FALSE, 0);

    gtk_container_add (GTK_CONTAINER (window), vbox);

    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 0);

    gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);

    gtk_widget_show (hbox);

    {
      GdkWindow * w = NULL;
      video_output = gtk_drawing_area_new ();

      gtk_box_pack_start (GTK_BOX (vbox), video_output, TRUE, TRUE, 0);

      gtk_widget_set_size_request (video_output, 800, 50);

      gtk_widget_show (video_output);

      /* for upp alpha plane */
      w = gtk_widget_get_window (video_output);
      if (w)
        {
          Display * dpy = GDK_WINDOW_XDISPLAY (w); // todo: usr env
          Window win = GDK_WINDOW_XID (w);
          if (dpy)
            {
              unsigned char data[1] = {1};
              Atom property = XInternAtom (dpy, "UPP_ALPHA", 0);
              Atom type = XInternAtom (dpy, "INTERGER", 1);
              XChangeProperty (dpy, win, property, type, 8, PropModeReplace,
                               data, 1);
              XFlush (dpy);
              fprintf (stderr, "get set display sucess!\n");
            }
          else
            {
              fprintf (stderr, "opendisplay error!\n");
            }
          
        }
      else
        {
          fprintf (stderr, "get window error!\n");
        }
    }

    {
        button = gtk_button_new_with_label ("Pause");

        g_signal_connect (G_OBJECT (button), "clicked",
                          G_CALLBACK (pause_cb), NULL);

        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

        gtk_widget_show (button);

        pause_button = button;
    }

    {
        button = gtk_button_new_with_label ("Reset");

        g_signal_connect (G_OBJECT (button), "clicked",
                          G_CALLBACK (reset_cb), NULL);

        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

        gtk_widget_show (button);
    }

    {
      button = gtk_button_new_with_label ("<<");

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (seekbackward_cb), NULL);

      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

      gtk_widget_show (button);
    }

    {
      button = gtk_button_new_with_label (">>");

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (seekforward_cb), NULL);

      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

      gtk_widget_show (button);
    }

    {
      button = gtk_button_new_with_label ("seekto485s");

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (seekto485s_cb), NULL);

      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

      gtk_widget_show (button);
    }

    {
        GtkObject *adjustment;
        adjustment = gtk_adjustment_new (0, 0, 101, 1, 5, 1);
        scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));

        gtk_box_pack_end (GTK_BOX (hbox), scale, TRUE, TRUE, 2);

        g_signal_connect (G_OBJECT (scale), "change-value",
                          G_CALLBACK (seek_cb), NULL);

        gtk_widget_show (scale);
    }

    //gtk_widget_show (window);

    {
      gint x = 0;
      gint y = 0;
      gtk_window_move (window, 50, 50);
      //gtk_window_get_position (window, &x, &y);
      //fprintf (stderr, "window pos, x:%d, y:%d\n", x, y);
      //backend_set_video_pos (x, y, 800, 450);
    }
    gtk_widget_show (window);
}

static gboolean
init (gpointer data)
{
    backend_set_window (GINT_TO_POINTER (GDK_WINDOW_XWINDOW (video_output->window)));

    if (uri_to_play)
        backend_play (uri_to_play);

    return FALSE;
}

static gboolean
timeout (gpointer data)
{
    guint64 pos;

    pos = backend_query_position ();
    if (!DURATION_IS_VALID (duration))
        duration = backend_query_duration ();

    if (!DURATION_IS_VALID (duration))
        return TRUE;

#if 0
    g_print ("duration=%f\n", duration / ((double) 60 * 1000 * 1000 * 1000));
    g_print ("position=%llu\n", pos);
#endif

    /** @todo use events for seeking instead of checking for bad positions. */
    if (pos != 0)
    {
        double value;
        value = (pos * (((double) 100) / duration));
        gtk_range_set_value (GTK_RANGE (scale), value);
    }

    return TRUE;
}

static gboolean
taskStatus (gpointer data)
{
  /* query thunder task status */
  backend_query_thunder_task_status ();
  return TRUE;
}

int
main (int argc,
      char *argv[])
{
    gtk_init (&argc, &argv);
    backend_init (&argc, &argv);

    start ();

    if (argc > 1)
    {
        if (strchr (argv[1], ':'))
            uri_to_play = g_strdup (argv[1]);
        else
            uri_to_play = g_strdup_printf ("file://%s", argv[1]);
    }

    //toggle_fullscreen ();
    g_idle_add (init, NULL);
    g_timeout_add (1000, timeout, NULL);
    //g_timeout_add (5000, taskStatus, NULL);

    gtk_main ();

    g_free (uri_to_play);

    backend_deinit ();

    return 0;
}
