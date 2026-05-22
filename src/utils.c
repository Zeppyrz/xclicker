#include <utils.h>

#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <libintl.h>
#include <linux/limits.h>

void xapp_error(const char *when, int code)
{
    g_printerr(_("Something went wrong when: '%s'. Please report errors like this.\n"), when);

    if (code == -1)
        return;

    exit(code);
}

void set_window_icon(GtkWindow *window)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource("/res/icon.png", NULL);

    gtk_window_set_icon(window, pixbuf);
}

void gtk_entry_set_text_if_not_null(GtkEntry *entry, const gchar *text)
{
    if (text)
    {
        gtk_entry_set_text(entry, text);
    } else {
        gtk_entry_set_text(entry, ""); // for reseting field
    }
}

void restart_app()
{
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        g_spawn_command_line_async(exe_path, NULL);
    }
    exit(0);
}
