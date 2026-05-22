/** XClicker, a x11 autoclicker
 * All source files in this repository are licensed under the
 * GNU General Public License v3.0.
 * Dependencies are licensed by their own.
 *
 * https://github.com/robiot/xclicker
 */

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <locale.h>
#include <libintl.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include "xclicker-app.h"
#include "config.h"

static const char *get_local_locale_dir()
{
    static char locale_dir[PATH_MAX];
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return NULL;
    exe_path[len] = '\0';

    char *last_slash = strrchr(exe_path, '/');
    if (!last_slash)
        return NULL;
    *last_slash = '\0';
    last_slash = strrchr(exe_path, '/');
    if (!last_slash)
        return NULL;
    *last_slash = '\0';

    snprintf(locale_dir, sizeof(locale_dir), "%s/po", exe_path);

    if (access(locale_dir, F_OK) == 0)
        return locale_dir;

    return NULL;
}

/**
 * Read language setting directly from config file before locale init.
 * Returns a static string — do not free.
 */
static const char *get_config_language()
{
    static char lang_buf[32];
    const char *config_dir = g_get_user_config_dir();
    char *config_file = g_strdup_printf("%s/xclicker.conf", config_dir);

    lang_buf[0] = '\0';

    if (g_file_test(config_file, G_FILE_TEST_EXISTS))
    {
        GKeyFile *keyfile = g_key_file_new();
        if (g_key_file_load_from_file(keyfile, config_file, G_KEY_FILE_NONE, NULL))
        {
            gchar *lang = g_key_file_get_string(keyfile, "Options", "LANGUAGE", NULL);
            if (lang && *lang)
            {
                g_strlcpy(lang_buf, lang, sizeof(lang_buf));
                g_free(lang);
            }
        }
        g_key_file_free(keyfile);
    }

    g_free(config_file);

    if (lang_buf[0] == '\0')
        strcpy(lang_buf, "en");

    return lang_buf;
}

int main(int argc, char *argv[])
{
    // 1. Read saved language from config and set LANGUAGE env var
    //    This must happen BEFORE setlocale() so gettext picks it up.
    const char *saved_lang = get_config_language();
    setenv("LANGUAGE", saved_lang, 1);

    // 2. Initialize locale (uses LANGUAGE env var)
    setlocale(LC_ALL, "");

    const char *locale_dir = get_local_locale_dir();
    if (locale_dir)
        bindtextdomain(GETTEXT_PACKAGE, locale_dir);
    else
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);

    textdomain(GETTEXT_PACKAGE);

    XInitThreads();
    srand(time(NULL));

    return g_application_run(G_APPLICATION(xclicker_app_new()), argc, argv);
}
