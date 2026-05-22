#include <gtk/gtk.h>
#include <pwd.h>
#include <X11/keysymdef.h>
#include <dirent.h>
#include <unistd.h>
#include <libintl.h>

#include "settings.h"
#include "x11api.h"
#include "mainwin.h"
#include "version.h"
#include "utils.h"
#include "config.h"

gboolean isChoosingHotkey = FALSE;

struct _items
{
    GtkWidget *buttons_entry;
    GtkWidget *start_button;
    GtkWidget *xevent_switch;
    GtkWidget *reset_preset_button;
    GtkWidget *language_combo;
} items;

struct set_buttons_entry_struct
{
    char *text;
};

gboolean set_buttons_entry_text(gpointer *data)
{
    struct set_buttons_entry_struct *args = data;
    gtk_entry_set_text(GTK_ENTRY(items.buttons_entry), args->text);
    free(args->text);
    g_free(args);

    return FALSE;
}

gboolean enable_start_button()
{
    gtk_widget_set_sensitive(items.start_button, TRUE);
    return FALSE;
}

gboolean hotkey_finished()
{
    set_start_stop_button_hotkey_text();
    return FALSE;
}

/**
 * Gets the keys pressed when setting hotkey.
 */
void get_hotkeys_handler()
{
    Display *display = get_display();
    if (!display)
    {
        g_printerr("Failed to open X11 display for hotkey capture\n");
        isChoosingHotkey = FALSE;
        g_idle_add(enable_start_button, NULL);
        return;
    }

    mask_config(display, MASK_KEYBOARD_PRESS | MASK_MOUSE_PRESS);

    gboolean hasPreKey = FALSE;
    while (1)
    {
        KeyState keyState;
        get_next_key_state(display, &keyState);

        int state = keyState.button;

        // Skip if no valid key was captured (evtype -1 means event parse failed)
        if (keyState.evtype == -1 || state == 0)
            continue;

        if (keyState.evtype == XI_ButtonPress)
        {
            // Mouse button event
            if (!is_valid_mouse_button(state))
                continue;

            config->button2 = state;
            config->button2_is_mouse = TRUE;
            if (!hasPreKey)
            {
                config->button1 = -1;
                config->button1_is_mouse = FALSE;
            }

            const char *key_str = mouse_button_to_string(state);
            if (!key_str)
                key_str = "?";

            struct set_buttons_entry_struct *user_data = g_malloc0(sizeof(struct set_buttons_entry_struct));

            if (hasPreKey == TRUE)
            {
                const char *buttons_entry_text = gtk_entry_get_text(GTK_ENTRY(items.buttons_entry));
                char *text = malloc(1 + strlen(buttons_entry_text) + strlen(key_str));
                sprintf(text, "%s%s", buttons_entry_text, key_str);
                user_data->text = text;
            }
            else
            {
                config->button1 = -1;
                config->button1_is_mouse = FALSE;
                char *text = (char *)malloc(1 + strlen(key_str));
                strcpy(text, key_str);
                user_data->text = text;
            }
            // Text is freed in the set_buttons_entry_text function
            g_idle_add(set_buttons_entry_text, user_data);
            break;
        }
        else if (keyState.evtype == XI_KeyPress)
        {
            // Keyboard event
            // Numlock & caps lock is incredibly buggy and causes memory leaks, pointer errors, free errors...
            if (state == XKeysymToKeycode(display, XK_Num_Lock) || state == XKeysymToKeycode(display, XK_Caps_Lock))
                continue;

            // If prekey, ex shift, ctrl
            if (state == XKeysymToKeycode(display, XK_Shift_L) || state == XKeysymToKeycode(display, XK_Shift_R) || state == XKeysymToKeycode(display, XK_Alt_L) || state == XKeysymToKeycode(display, XK_Alt_R) || state == XKeysymToKeycode(display, XK_Escape) || state == XKeysymToKeycode(display, XK_Control_L) || state == XKeysymToKeycode(display, XK_Control_R) || state == XKeysymToKeycode(display, XK_ISO_Level3_Shift) || state == XKeysymToKeycode(display, XK_Super_L) || state == XKeysymToKeycode(display, XK_Super_R))
            {
                hasPreKey = TRUE;
                config->button1 = state;
                config->button1_is_mouse = FALSE;
                const char *key_str = keycode_to_string(display, state);
                if (!key_str)
                    key_str = "?";
                const char *plus = " + ";
                char *text = malloc(1 + strlen(key_str) + strlen(plus));
                sprintf(text, "%s%s", key_str, plus);

                struct set_buttons_entry_struct *user_data = g_malloc0(sizeof(struct set_buttons_entry_struct));
                user_data->text = text;
                g_idle_add(set_buttons_entry_text, user_data);
            }
            else
            {
                config->button2 = state;
                config->button2_is_mouse = FALSE;
                const char *key_str = keycode_to_string(display, state);
                if (!key_str)
                    key_str = "?";
                struct set_buttons_entry_struct *user_data = g_malloc0(sizeof(struct set_buttons_entry_struct));

                if (hasPreKey == TRUE)
                {
                    const char *buttons_entry_text = gtk_entry_get_text(GTK_ENTRY(items.buttons_entry));
                    char *text = malloc(1 + strlen(buttons_entry_text) + strlen(key_str));
                    sprintf(text, "%s%s", buttons_entry_text, key_str);
                    user_data->text = text;
                }
                else
                {
                    config->button1 = -1;
                    config->button1_is_mouse = FALSE;
                    char *text = (char *)malloc(1 + strlen(key_str));
                    strcpy(text, key_str);
                    user_data->text = text;
                }
                // Text is freed in the set_buttons_entry_text function
                g_idle_add(set_buttons_entry_text, user_data);
                break;
            }
        }
    }
    XCloseDisplay(display);
    g_idle_add(enable_start_button, NULL);
    g_idle_add(hotkey_finished, NULL);

    g_key_file_set_integer(config_gfile, CFGK_BUTTON_1, config->button1);
    g_key_file_set_integer(config_gfile, CFGK_BUTTON_2, config->button2);
    g_key_file_set_boolean(config_gfile, CFGK_BUTTON1_IS_MOUSE, config->button1_is_mouse);
    g_key_file_set_boolean(config_gfile, CFGK_BUTTON2_IS_MOUSE, config->button2_is_mouse);

    g_key_file_save_to_file(config_gfile, configpath, NULL);
    isChoosingHotkey = FALSE;
}

void safe_mode_changed(GtkSwitch *self, gboolean state)
{
    g_key_file_set_boolean(config_gfile, CFGK_SAFEMODE, state);
    config->safe_mode_enabled = state;

    g_key_file_save_to_file(config_gfile, configpath, NULL);
    // Hack to make the background color not glitch
    gtk_switch_set_active(self, state);
}

void xevent_switch_changed(GtkSwitch *self, gboolean state)
{
    g_key_file_set_boolean(config_gfile, CFGK_USE_XEVENT, state);

    save_and_populate_config();
    gtk_switch_set_active(self, state);
}

void start_button_pressed(GtkButton *self)
{
    isChoosingHotkey = TRUE;
    gtk_widget_set_sensitive(GTK_WIDGET(self), FALSE);
    gtk_entry_set_text(GTK_ENTRY(items.buttons_entry), _("Press Desired Keys"));
    g_thread_new("get_hotkeys_handler", get_hotkeys_handler, NULL);
}

void reset_preset_button_pressed()
{
    g_key_file_remove_group(config_gfile, PRESET_CATEGORY_CLICK_INTERVAL, NULL);
    g_key_file_remove_group(config_gfile, PRESET_CATEGORY_OPTIONS, NULL);
    g_key_file_remove_group(config_gfile, PRESET_CATEGORY_MORE_OPTIONS, NULL);

    save_and_populate_config();
    mainappwindow_import_config();
}

static void show_restart_dialog()
{
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_NONE,
        _("Language Changed")
    );
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        _("The language has been changed. Please restart XClicker for the changes to take effect.")
    );
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("Restart Later"), GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("Restart Now"), GTK_RESPONSE_OK);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_OK)
    {
        restart_app();
    }
}

void language_changed(GtkComboBoxText *combo)
{
    const gchar *lang = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo));
    if (lang == NULL)
        return;

    // Only show dialog if language actually changed
    if (config->language && strcmp(config->language, lang) == 0)
        return;

    g_key_file_set_string(config_gfile, CFGK_LANGUAGE, lang);
    config->language = lang;
    g_key_file_save_to_file(config_gfile, configpath, NULL);

    show_restart_dialog();
}

void settings_dialog_new()
{
    GtkBuilder *builder = gtk_builder_new_from_resource("/res/ui/settings-dialog.ui");
    GtkDialog *dialog = GTK_DIALOG(gtk_builder_get_object(builder, "dialog"));

    config_read_from_file();

    set_window_icon(dialog);

    gtk_builder_add_callback_symbol(builder, "safe_mode_changed", safe_mode_changed);
    gtk_builder_add_callback_symbol(builder, "xevent_switch_changed", xevent_switch_changed);
    gtk_builder_add_callback_symbol(builder, "start_button_pressed", start_button_pressed);
    gtk_builder_add_callback_symbol(builder, "reset_preset_button_pressed", reset_preset_button_pressed);
    gtk_builder_add_callback_symbol(builder, "language_changed", language_changed);

    gtk_builder_connect_signals(builder, NULL);

    // Load version
    gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "version_label")), XCLICKER_VERSION);

    // Fill struct
    items.buttons_entry = gtk_builder_get_object(builder, "buttons_entry");
    items.start_button = gtk_builder_get_object(builder, "start_button");
    items.xevent_switch = gtk_builder_get_object(builder, "xevent_switch");
    items.language_combo = gtk_builder_get_object(builder, "language_combo");

    // Load
    gtk_switch_set_active(GTK_SWITCH(gtk_builder_get_object(builder, "safe_mode_switch")), is_safemode());
    gtk_switch_set_active(GTK_SWITCH(items.xevent_switch), config->use_xevent);

    // Load language options
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(items.language_combo), "en", "English");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(items.language_combo), "zh_CN", "中文");
    if (config->language)
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(items.language_combo), config->language);
    else
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(items.language_combo), "en");

    // Load hotkeys
    Display *display = get_display();
    if (display)
    {
        const char *button_2_key = config->button2_is_mouse
            ? mouse_button_to_string(config->button2)
            : keycode_to_string(display, config->button2);
        if (!button_2_key)
            button_2_key = "?";
        const char *sep = " + ";
        char *hotkeys;

        if (config->button1 != -1)
        {
            const char *button_1_key = config->button1_is_mouse
                ? mouse_button_to_string(config->button1)
                : keycode_to_string(display, config->button1);
            if (!button_1_key)
                button_1_key = "?";
            hotkeys = malloc(1 + strlen(sep) + strlen(button_2_key) + strlen(button_1_key));
            sprintf(hotkeys, "%s%s%s", button_1_key, sep, button_2_key);
        }
        else
        {
            hotkeys = malloc(1 + strlen(button_2_key));
            sprintf(hotkeys, "%s", button_2_key);
        }
        gtk_entry_set_text(GTK_ENTRY(items.buttons_entry), hotkeys);

        free(hotkeys);
        XCloseDisplay(display);
    }

    // Run
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}
