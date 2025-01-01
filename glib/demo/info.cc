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

#include "config.h"
#include "info.h"
#include "utils.h"

static void pgd_info_add_permissions(GtkGrid *table, PopplerPermissions permissions, gint *row)
{
    GtkWidget *label, *hbox, *vbox;
    GtkWidget *checkbox;

    label = gtk_label_new(nullptr);
    g_object_set(G_OBJECT(label), "xalign", 0.0, NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Permissions:</b>");
    gtk_grid_attach(GTK_GRID(table), label, 0, *row, 1, 1);
    gtk_widget_show(label);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    checkbox = gtk_check_button_new_with_label("Print");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_PRINT));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Copy");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_COPY));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Modify");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_MODIFY));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Add notes");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_ADD_NOTES));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Fill forms");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_FILL_FORM));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    checkbox = gtk_check_button_new_with_label("Extract contents");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_EXTRACT_CONTENTS));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Assemble");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_ASSEMBLE));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    checkbox = gtk_check_button_new_with_label("Print at high resolution");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), (permissions & POPPLER_PERMISSIONS_OK_TO_PRINT_HIGH_RESOLUTION));
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
    gtk_widget_show(checkbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    gtk_grid_attach(GTK_GRID(table), vbox, 1, *row, 1, 1);
    gtk_widget_show(vbox);

    *row += 1;
}

static void pgd_info_add_metadata(GtkGrid *table, const gchar *metadata, gint *row)
{
    GtkWidget *label;
    GtkWidget *textview, *swindow;
    GtkTextBuffer *buffer;

    label = gtk_label_new(nullptr);
    g_object_set(G_OBJECT(label), "xalign", 0.0, NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Metadata:</b>");
    gtk_grid_attach(GTK_GRID(table), label, 0, *row, 1, 1);
    gtk_widget_show(label);

    swindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (metadata) {
        gtk_text_buffer_set_text(buffer, metadata, -1);
    }

    gtk_container_add(GTK_CONTAINER(swindow), textview);
    gtk_widget_show(textview);

    gtk_grid_attach(GTK_GRID(table), swindow, 1, *row, 1, 1);
    gtk_widget_set_hexpand(swindow, TRUE);
    gtk_widget_set_vexpand(swindow, TRUE);
    gtk_widget_show(swindow);

    *row += 1;
}

GtkWidget *pgd_info_create_widget(PopplerDocument *document)
{
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *frame, *table;
    gchar *str;
    gchar *title, *format, *author, *subject;
    gchar *keywords, *creator, *producer;
    gchar *metadata;
    gchar *perm_id;
    gchar *up_id;
    gboolean linearized;
    GDateTime *creation_date, *mod_date;
    GEnumValue *enum_value;
    PopplerBackend backend;
    PopplerPageLayout layout;
    PopplerPageMode mode;
    PopplerPermissions permissions;
    PopplerViewerPreferences view_prefs;
    gint row = 0;

    g_object_get(document, "title", &title, "format", &format, "author", &author, "subject", &subject, "keywords", &keywords, "creation-datetime", &creation_date, "mod-datetime", &mod_date, "creator", &creator, "producer", &producer,
                 "linearized", &linearized, "page-mode", &mode, "page-layout", &layout, "permissions", &permissions, "viewer-preferences", &view_prefs, "metadata", &metadata, NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    backend = poppler_get_backend();
    enum_value = g_enum_get_value((GEnumClass *)g_type_class_ref(POPPLER_TYPE_BACKEND), backend);
    str = g_strdup_printf("<span weight='bold' size='larger'>Poppler %s (%s)</span>", poppler_get_version(), enum_value->value_name);
    label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 12);
    gtk_widget_show(label);

    frame = gtk_frame_new(nullptr);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Document properties</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_widget_show(label);

    table = gtk_grid_new();
    gtk_widget_set_margin_top(table, 5);
    gtk_widget_set_margin_bottom(table, 5);
    gtk_widget_set_margin_start(table, 12);
    gtk_widget_set_margin_end(table, 5);
    gtk_grid_set_column_spacing(GTK_GRID(table), 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);

    pgd_table_add_property(GTK_GRID(table), "<b>Format:</b>", format, &row);
    g_free(format);

    pgd_table_add_property(GTK_GRID(table), "<b>Title:</b>", title, &row);
    g_free(title);

    pgd_table_add_property(GTK_GRID(table), "<b>Author:</b>", author, &row);
    g_free(author);

    pgd_table_add_property(GTK_GRID(table), "<b>Subject:</b>", subject, &row);
    g_free(subject);

    pgd_table_add_property(GTK_GRID(table), "<b>Keywords:</b>", keywords, &row);
    g_free(keywords);

    pgd_table_add_property(GTK_GRID(table), "<b>Creator:</b>", creator, &row);
    g_free(creator);

    pgd_table_add_property(GTK_GRID(table), "<b>Producer:</b>", producer, &row);
    g_free(producer);

    pgd_table_add_property(GTK_GRID(table), "<b>Linearized:</b>", linearized ? "Yes" : "No", &row);

    str = g_date_time_format(creation_date, "%c");
    pgd_table_add_property(GTK_GRID(table), "<b>Creation Date:</b>", str, &row);
    g_clear_pointer(&creation_date, g_date_time_unref);
    g_free(str);

    str = g_date_time_format(mod_date, "%c");
    pgd_table_add_property(GTK_GRID(table), "<b>Modification Date:</b>", str, &row);
    g_clear_pointer(&mod_date, g_date_time_unref);
    g_free(str);

    enum_value = g_enum_get_value((GEnumClass *)g_type_class_peek(POPPLER_TYPE_PAGE_MODE), mode);
    pgd_table_add_property(GTK_GRID(table), "<b>Page Mode:</b>", enum_value->value_name, &row);

    enum_value = g_enum_get_value((GEnumClass *)g_type_class_peek(POPPLER_TYPE_PAGE_LAYOUT), layout);
    pgd_table_add_property(GTK_GRID(table), "<b>Page Layout:</b>", enum_value->value_name, &row);

    if (poppler_document_get_id(document, &perm_id, &up_id)) {
        str = g_strndup(perm_id, 32);
        g_free(perm_id);
        pgd_table_add_property(GTK_GRID(table), "<b>Permanent ID:</b>", str, &row);
        g_free(str);
        str = g_strndup(up_id, 32);
        g_free(up_id);
        pgd_table_add_property(GTK_GRID(table), "<b>Update ID:</b>", str, &row);
        g_free(str);
    }

    pgd_info_add_permissions(GTK_GRID(table), permissions, &row);

    pgd_info_add_metadata(GTK_GRID(table), metadata, &row);
    g_free(metadata);

    /* TODO: view_prefs */

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_widget_show(table);

    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    gtk_widget_show(frame);

    return vbox;
}
