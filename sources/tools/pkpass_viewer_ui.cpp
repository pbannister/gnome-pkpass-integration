//
// pkpass-viewer: show one pass in a window.
//
// This is the graphical half of the handler.  It is built only where the GTK4
// development files are present; without them `pkpass-integrate handle` prints
// the equivalent command-line report instead, and the rest of the project is
// unaffected.
//
// The window uses the desktop theme rather than the pass's own colours: the
// colours are carried by `pkpass-render`, and mixing a per-pass stylesheet into
// the desktop theme produced unreadable text for light passes on a dark theme.
//
#include "pkpass/pkpass_reader.h"
#include "util/file_io.h"
#include "version/version.h"
#include "zip/zip_reader.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <unistd.h>

#include <cstdio>
#include <string>

namespace {

struct state_o {
    std::string s_path;
    std::string s_logo_path;
    std::string s_message;
};

void remove_if_present(const std::string &s_path) {
    if (!s_path.empty()) {
        ::g_remove(s_path.c_str());
    }
}

GtkWidget *make_label(const std::string &s_text, bool b_bold, bool b_wrap, const char *s_class) {
    GtkWidget *p_label = gtk_label_new(s_text.c_str());
    gtk_label_set_xalign(GTK_LABEL(p_label), 0.0F);
    if (b_wrap) {
        gtk_label_set_wrap(GTK_LABEL(p_label), TRUE);
    }
    gtk_label_set_selectable(GTK_LABEL(p_label), TRUE);
    if (nullptr != s_class) {
        gtk_widget_add_css_class(p_label, s_class);
    }
    if (b_bold) {
        gtk_widget_add_css_class(p_label, "heading");
    }
    return p_label;
}

// One field: its label above its value, inside the column that group occupies.
GtkWidget *make_field(const gnome_pkpass::pkpass::field_o &o_field) {
    GtkWidget *p_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    if (!o_field.s_label.empty()) {
        gtk_box_append(GTK_BOX(p_box), make_label(o_field.s_label, false, true, "dim-label"));
    }
    const std::string s_value =
        o_field.s_attributed_value.empty() ? o_field.s_value : o_field.s_attributed_value;
    gtk_box_append(GTK_BOX(p_box), make_label(s_value, false, true, nullptr));
    return p_box;
}

GtkWidget *make_field_row(const gnome_pkpass::pkpass::field_group_o &o_group) {
    GtkWidget *p_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_set_homogeneous(GTK_BOX(p_row), TRUE);
    for (const gnome_pkpass::pkpass::field_o &o_field : o_group.o_fields) {
        gtk_box_append(GTK_BOX(p_row), make_field(o_field));
    }
    return p_row;
}

const gnome_pkpass::pkpass::field_group_o *group_named(const gnome_pkpass::pkpass::pass_o &o_pass,
                                                       const char *s_name) {
    for (const gnome_pkpass::pkpass::field_group_o &o_group : o_pass.o_field_groups) {
        if (o_group.s_name == s_name) {
            return &o_group;
        }
    }
    return nullptr;
}

void add_group(GtkWidget *p_content, const gnome_pkpass::pkpass::pass_o &o_pass,
               const char *s_name) {
    const gnome_pkpass::pkpass::field_group_o *p_group = group_named(o_pass, s_name);
    if (nullptr == p_group || p_group->o_fields.empty()) {
        return;
    }
    gtk_box_append(GTK_BOX(p_content), make_field_row(*p_group));
}

const char *preferred_logo(const gnome_pkpass::zip::archive_c &o_archive) {
    for (const char *s_name : {"logo@2x.png", "logo.png", "logo@3x.png"}) {
        if (o_archive.contains(s_name)) {
            return s_name;
        }
    }
    return nullptr;
}

// Write the pass logo beside the window as a PNG, because GtkPicture reads a
// file; nothing else can present a PNG from memory without a pixbuf loader.
bool write_logo(const std::string &s_path, std::string &s_logo_path) {
    gnome_pkpass::zip::archive_c o_archive;
    std::string s_error;
    if (!o_archive.load_file(s_path, s_error)) {
        return false;
    }
    const char *s_name = preferred_logo(o_archive);
    if (nullptr == s_name) {
        return false;
    }
    std::string s_image;
    if (!o_archive.extract(s_name, s_image, s_error)) {
        return false;
    }

    gchar *s_temporary = nullptr;
    gint i_descriptor = g_file_open_tmp("pkpass-viewer-XXXXXX.png", &s_temporary, nullptr);
    if (0 > i_descriptor) {
        return false;
    }
    ::close(i_descriptor);
    s_logo_path = s_temporary;
    g_free(s_temporary);

    return g_file_set_contents(s_logo_path.c_str(), s_image.data(),
                               static_cast<gssize>(s_image.size()), nullptr);
}

void on_copy_message(GtkWidget *p_widget, gpointer p_data) {
    (void)p_widget;
    state_o *p_state = static_cast<state_o *>(p_data);
    GdkDisplay *p_display = gdk_display_get_default();
    if (nullptr == p_display) {
        return;
    }
    gdk_clipboard_set_text(gdk_display_get_clipboard(p_display), p_state->s_message.c_str());
}

void on_toggle_back(GtkToggleButton *p_button, gpointer p_data) {
    GtkWidget *p_back = GTK_WIDGET(p_data);
    gtk_widget_set_visible(p_back, gtk_toggle_button_get_active(p_button));
}

GtkWidget *build_window(GtkApplication *p_application, state_o *p_state) {
    gnome_pkpass::pkpass::pass_o o_pass;
    std::string s_error;

    GtkWidget *p_window = gtk_application_window_new(p_application);
    if (!gnome_pkpass::pkpass::read_pass(p_state->s_path, o_pass, s_error)) {
        gtk_window_set_title(GTK_WINDOW(p_window), "Pass");
        gtk_window_set_default_size(GTK_WINDOW(p_window), 420, 200);
        gtk_window_set_child(GTK_WINDOW(p_window), make_label(s_error, false, true, nullptr));
        return p_window;
    }

    const std::string s_title =
        o_pass.s_description.empty() ? "Apple Wallet pass" : o_pass.s_description;
    gtk_window_set_title(GTK_WINDOW(p_window), s_title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(p_window), 460, 640);

    GtkWidget *p_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(p_content, 20);
    gtk_widget_set_margin_end(p_content, 20);
    gtk_widget_set_margin_top(p_content, 16);
    gtk_widget_set_margin_bottom(p_content, 16);

    if (write_logo(p_state->s_path, p_state->s_logo_path)) {
        GtkWidget *p_picture = gtk_picture_new_for_filename(p_state->s_logo_path.c_str());
        gtk_picture_set_can_shrink(GTK_PICTURE(p_picture), TRUE);
        gtk_widget_set_halign(p_picture, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(p_content), p_picture);
    }
    if (!o_pass.s_logo_text.empty()) {
        GtkWidget *p_logo_text = make_label(o_pass.s_logo_text, true, true, "title-2");
        gtk_widget_set_halign(p_logo_text, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(p_content), p_logo_text);
    }

    add_group(p_content, o_pass, "headerFields");
    add_group(p_content, o_pass, "primaryFields");
    add_group(p_content, o_pass, "secondaryFields");
    add_group(p_content, o_pass, "auxiliaryFields");

    if (!o_pass.o_barcodes.empty()) {
        const gnome_pkpass::pkpass::barcode_o &o_barcode = o_pass.o_barcodes.front();
        p_state->s_message = o_barcode.s_message;
        gtk_box_append(GTK_BOX(p_content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_box_append(GTK_BOX(p_content),
                       make_label(gnome_pkpass::pkpass::barcode_format_label(o_barcode.s_format) +
                                      " barcode (not drawn)",
                                  false, true, "dim-label"));
        if (!o_barcode.s_alt_text.empty()) {
            gtk_box_append(GTK_BOX(p_content), make_label(o_barcode.s_alt_text, true, true, nullptr));
        }
        gtk_box_append(GTK_BOX(p_content),
                       make_label(o_barcode.s_message, false, true, "monospace"));
        GtkWidget *p_copy = gtk_button_new_with_label("Copy barcode message");
        g_signal_connect(p_copy, "clicked", G_CALLBACK(on_copy_message), p_state);
        gtk_box_append(GTK_BOX(p_content), p_copy);
    }

    const gnome_pkpass::pkpass::field_group_o *p_back = group_named(o_pass, "backFields");
    if (nullptr != p_back && !p_back->o_fields.empty()) {
        GtkWidget *p_toggle = gtk_toggle_button_new_with_label("Back of pass");
        gtk_box_append(GTK_BOX(p_content), p_toggle);

        GtkWidget *p_back_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_visible(p_back_box, FALSE);
        for (const gnome_pkpass::pkpass::field_o &o_field : p_back->o_fields) {
            gtk_box_append(GTK_BOX(p_back_box), make_field(o_field));
        }
        gtk_box_append(GTK_BOX(p_content), p_back_box);
        g_signal_connect(p_toggle, "toggled", G_CALLBACK(on_toggle_back), p_back_box);
    }

    GtkWidget *p_scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(p_scroller), GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(p_scroller), p_content);
    gtk_window_set_child(GTK_WINDOW(p_window), p_scroller);
    return p_window;
}

void on_activate(GtkApplication *p_application, gpointer p_data) {
    state_o *p_state = static_cast<state_o *>(p_data);
    GtkWidget *p_window = build_window(p_application, p_state);
    gtk_window_present(GTK_WINDOW(p_window));
}

void on_shutdown(GApplication *p_application, gpointer p_data) {
    (void)p_application;
    state_o *p_state = static_cast<state_o *>(p_data);
    remove_if_present(p_state->s_logo_path);
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    if (2 > i_argument_count) {
        std::fprintf(stderr, "usage: pkpass-viewer FILE.pkpass\n");
        return 2;
    }
    for (int i_argument = 1; i_argument < i_argument_count; ++i_argument) {
        const std::string s_argument = o_arguments[i_argument];
        if ("--version" == s_argument) {
            std::printf("pkpass-viewer %s (build %d)\n",
                        gnome_pkpass::version::version_string(),
                        gnome_pkpass::version::version_build_counter());
            return 0;
        }
    }

    state_o o_state;
    o_state.s_path = o_arguments[i_argument_count - 1];

    GtkApplication *p_application = gtk_application_new(
        nullptr, static_cast<GApplicationFlags>(0));
    g_signal_connect(p_application, "activate", G_CALLBACK(on_activate), &o_state);
    g_signal_connect(p_application, "shutdown", G_CALLBACK(on_shutdown), &o_state);
    const int i_status = g_application_run(G_APPLICATION(p_application), 0, nullptr);
    g_object_unref(p_application);
    return i_status;
}
