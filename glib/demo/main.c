/*
 * Copyright (C) 2007 Carlos Garcia Campos  <carlosgc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <poppler.h>
#include <string.h>

#include "info.h"
#include "fonts.h"
#include "render.h"
#include "page.h"
#include "outline.h"
#include "links.h"
#include "forms.h"
#include "transitions.h"
#include "images.h"
#include "annots.h"
#include "attachments.h"
#include "layers.h"
#include "text.h"
#include "taggedstruct.h"
#include "find.h"
#include "print.h"
#include "selections.h"
#include "signature.h"

enum
{
    PGD_TITLE_COLUMN,
    PGD_NPAGE_COLUMN,
    PGD_WIDGET_COLUMN,
    N_COLUMNS
};

typedef struct
{
    const gchar *name;
    GtkWidget *(*create_widget)(PopplerDocument *document);
} PopplerGlibDemo;

static const PopplerGlibDemo demo_list[] = { { "Info", pgd_info_create_widget },
                                             { "Fonts", pgd_fonts_create_widget },
                                             { "Render", pgd_render_create_widget },
                                             { "Selections", pgd_selections_create_widget },
                                             { "Page Info", pgd_page_create_widget },
                                             { "Outline", pgd_outline_create_widget },
                                             { "Links", pgd_links_create_widget },
                                             { "Forms", pgd_forms_create_widget },
                                             { "Page Transitions", pgd_transitions_create_widget },
                                             { "Images", pgd_images_create_widget },
                                             { "Annotations", pgd_annots_create_widget },
                                             { "Attachments", pgd_attachments_create_widget },
                                             { "Layers", pgd_layers_create_widget },
                                             { "Text", pgd_text_create_widget },
                                             { "Tagged Structure", pgd_taggedstruct_create_widget },
                                             { "Find", pgd_find_create_widget },
                                             { "Print", pgd_print_create_widget },
                                             { "Signature", pgd_signature_create_widget } };

static void pgd_demo_changed(GtkTreeSelection *selection, GtkNotebook *notebook)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gint n_page;

        gtk_tree_model_get(model, &iter, PGD_NPAGE_COLUMN, &n_page, -1);
        gtk_notebook_set_current_page(notebook, n_page);
    }
}

static GtkWidget *pgd_demo_list_create(void)
{
    GtkWidget *treeview;
    GtkListStore *model;
    GtkCellRenderer *renderer;
    gint i;

    model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 0, "Demos", renderer, "text", PGD_TITLE_COLUMN, NULL);

    for (i = 0; i < G_N_ELEMENTS(demo_list); i++) {
        GtkTreeIter iter;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, PGD_TITLE_COLUMN, demo_list[i].name, PGD_NPAGE_COLUMN, i, -1);
    }

    g_object_unref(model);

    return treeview;
}

static GtkWidget *pgd_demo_notebook_create(PopplerDocument *document)
{
    GtkWidget *notebook;
    gint i;

    notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    for (i = 0; i < G_N_ELEMENTS(demo_list); i++) {
        GtkWidget *demo_widget;

        demo_widget = demo_list[i].create_widget(document);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), demo_widget, NULL);
        gtk_widget_show(demo_widget);
    }

    return notebook;
}

static void pgd_demo_auth_dialog_entry_changed(GtkEditable *editable, GtkDialog *dialog)
{
    const char *text;

    text = gtk_entry_get_text(GTK_ENTRY(editable));

    gtk_dialog_set_response_sensitive(dialog, GTK_RESPONSE_OK, (text != NULL && *text != '\0'));
    g_object_set_data(G_OBJECT(dialog), "pgd-password", (gpointer)text);
}

static void pgd_demo_auth_dialog_entry_activated(GtkEntry *entry, GtkDialog *dialog)
{
    gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

static GtkDialog *pgd_demo_get_auth_dialog(GFile *uri_file)
{
    GtkDialog *dialog;
    GtkWidget *content_area;
    GtkWidget *password_entry;
    GtkWidget *hbox, *main_vbox, *vbox, *icon;
    GtkWidget *table;
    GtkWidget *label;
    gchar *format, *markup, *file_name;

    dialog = GTK_DIALOG(gtk_dialog_new());
    content_area = gtk_dialog_get_content_area(dialog);

    /* Set the dialog up with HIG properties */
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_box_set_spacing(GTK_BOX(content_area), 2); /* 2 * 5 + 2 = 12 */

    gtk_window_set_title(GTK_WINDOW(dialog), "Enter password");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "dialog-password");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gtk_dialog_add_buttons(dialog, "_Cancel", GTK_RESPONSE_CANCEL, "_Unlock Document", GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);

    /* Build contents */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    icon = gtk_image_new_from_icon_name("dialog-password", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(icon, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_widget_show(icon);

    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_box_pack_start(GTK_BOX(hbox), main_vbox, TRUE, TRUE, 0);
    gtk_widget_show(main_vbox);

    label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_label_set_yalign(GTK_LABEL(label), 0.5);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    file_name = g_file_get_basename(uri_file);
    format = g_strdup_printf("<span size=\"larger\" weight=\"bold\">%s</span>\n\n%s", "Password required", "The document “%s” is locked and requires a password before it can be opened.");
    markup = g_markup_printf_escaped(format, file_name);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(format);
    g_free(markup);
    g_free(file_name);
    gtk_box_pack_start(GTK_BOX(main_vbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(main_vbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show(vbox);

    /* The table that holds the entries */
    table = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(table), 12);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    label = gtk_label_new_with_mnemonic("_Password:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_label_set_yalign(GTK_LABEL(label), 0.5);

    password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    g_signal_connect(password_entry, "changed", G_CALLBACK(pgd_demo_auth_dialog_entry_changed), dialog);
    g_signal_connect(password_entry, "activate", G_CALLBACK(pgd_demo_auth_dialog_entry_activated), dialog);

    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
    gtk_widget_show(label);

    gtk_grid_attach(GTK_GRID(table), password_entry, 1, 0, 1, 1);
    gtk_widget_set_hexpand(password_entry, TRUE);
    gtk_widget_show(password_entry);

    gtk_label_set_mnemonic_widget(GTK_LABEL(label), password_entry);

    return dialog;
}

gint main(gint argc, gchar **argv)
{
    PopplerDocument *document;
    GtkWidget *win;
    GtkWidget *hbox;
    GtkWidget *notebook;
    GtkWidget *treeview;
    GtkTreeSelection *selection;
    GFile *file;
    GTimer *timer;
    GError *error = NULL;
    GtkAccelGroup *gtk_accel;
    GClosure *closure;

    if (argc != 2) {
        g_print("Usage: poppler-glib-demo FILE\n");
        return 1;
    }

    gtk_init(&argc, &argv);

    file = g_file_new_for_commandline_arg(argv[1]);

    timer = g_timer_new();
    document = poppler_document_new_from_gfile(file, NULL, NULL, &error);
    g_timer_stop(timer);
    if (error) {
        while (g_error_matches(error, POPPLER_ERROR, POPPLER_ERROR_ENCRYPTED)) {
            GtkDialog *dialog;
            const gchar *password;

            dialog = pgd_demo_get_auth_dialog(file);
            if (gtk_dialog_run(dialog) != GTK_RESPONSE_OK) {
                g_print("Error: no password provided\n");
                g_object_unref(file);

                return 1;
            }

            g_clear_error(&error);
            password = g_object_get_data(G_OBJECT(dialog), "pgd-password");

            g_timer_start(timer);
            document = poppler_document_new_from_gfile(file, password, NULL, &error);
            g_timer_stop(timer);

            gtk_widget_destroy(GTK_WIDGET(dialog));
        }

        if (error) {
            g_print("Error: %s\n", error->message);
            g_error_free(error);
            g_object_unref(file);

            return 1;
        }
    }

    g_object_unref(file);

    g_print("Document successfully loaded in %.4f seconds\n", g_timer_elapsed(timer, NULL));
    g_timer_destroy(timer);

    /* Main window */
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(win), 1024, 768);
    gtk_window_set_title(GTK_WINDOW(win), "Poppler GLib Demo");
    g_signal_connect(G_OBJECT(win), "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    gtk_accel = gtk_accel_group_new();
    closure = g_cclosure_new(G_CALLBACK(gtk_main_quit), NULL, NULL);
    gtk_accel_group_connect(gtk_accel, gdk_keyval_from_name("q"), GDK_CONTROL_MASK, 0, closure);
    g_closure_unref(closure);
    gtk_window_add_accel_group(GTK_WINDOW(win), gtk_accel);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    treeview = pgd_demo_list_create();
    gtk_box_pack_start(GTK_BOX(hbox), treeview, FALSE, TRUE, 0);
    gtk_widget_show(treeview);

    notebook = pgd_demo_notebook_create(document);
    gtk_box_pack_start(GTK_BOX(hbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(pgd_demo_changed), (gpointer)notebook);

    gtk_container_add(GTK_CONTAINER(win), hbox);
    gtk_widget_show(hbox);

    gtk_widget_show(win);

    gtk_main();

    g_object_unref(document);

    return 0;
}
