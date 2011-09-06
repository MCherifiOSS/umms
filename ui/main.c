#include <glib.h>
#include <gtk/gtk.h>
#include "umms-gtk-ui.h"
#include "umms-gtk-player.h"


int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    ui_create();
    ply_init();

    ui_main_loop();

    return 0;
}
