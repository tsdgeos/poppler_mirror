/*
 * Copyright (C) 2008 Carlos Garcia Campos  <carlosgc@gnome.org>
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

#include "attachments.h"
#include "utils.h"

enum
{
    ATTACHMENTS_NAME_COLUMN,
    ATTACHMENTS_DESCRIPTION_COLUMN,
    ATTACHMENTS_SIZE_COLUMN,
    ATTACHMENTS_CTIME_COLUMN,
    ATTACHMENTS_MTIME_COLUMN,
    ATTACHMENTS_ATTACHMENT_COLUMN,
    N_COLUMNS
};

static void pgd_attachments_fill_model(GtkListStore *model, PopplerDocument *document)
{
    GList *list, *l;

    list = poppler_document_get_attachments(document);

    for (l = list; l && l->data; l = g_list_next(l)) {
        PopplerAttachment *attachment = POPPLER_ATTACHMENT(l->data);
        GtkTreeIter iter;
        gchar *size;
        GDateTime *ctime, *mtime;
        gchar *ctime_str, *mtime_str;
        const gchar *name;
        const gchar *description;

        name = poppler_attachment_get_name(attachment);
        description = poppler_attachment_get_description(attachment);
        size = g_strdup_printf("%" G_GSIZE_FORMAT, poppler_attachment_get_size(attachment));
        ctime = poppler_attachment_get_ctime(attachment);
        ctime_str = ctime ? g_date_time_format(ctime, "%c") : nullptr;
        mtime = poppler_attachment_get_mtime(attachment);
        mtime_str = mtime ? g_date_time_format(mtime, "%c") : nullptr;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, ATTACHMENTS_NAME_COLUMN, name ? name : "Unknown", ATTACHMENTS_DESCRIPTION_COLUMN, description ? description : "Unknown", ATTACHMENTS_SIZE_COLUMN, size ? size : "Unknown", ATTACHMENTS_CTIME_COLUMN,
                           ctime_str ? ctime_str : "Unknown", ATTACHMENTS_MTIME_COLUMN, mtime_str ? mtime_str : "Unknown", ATTACHMENTS_ATTACHMENT_COLUMN, attachment, -1);

        g_free(size);
        g_free(ctime_str);
        g_free(mtime_str);

        g_object_unref(attachment);
    }

    g_list_free(list);
}

static GtkWidget *pgd_attachments_create_list(GtkTreeModel *model)
{
    GtkWidget *treeview;
    GtkCellRenderer *renderer;

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 0, "Name", renderer, "text", ATTACHMENTS_NAME_COLUMN, nullptr);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 1, "Description", renderer, "text", ATTACHMENTS_DESCRIPTION_COLUMN, nullptr);
    g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, nullptr);
    g_object_set(G_OBJECT(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), 1)), "expand", TRUE, nullptr);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 2, "Size", renderer, "text", ATTACHMENTS_SIZE_COLUMN, nullptr);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 3, "Creation Date", renderer, "text", ATTACHMENTS_CTIME_COLUMN, nullptr);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 4, "Modification Date", renderer, "text", ATTACHMENTS_MTIME_COLUMN, nullptr);
    return treeview;
}

static void pgd_attachments_save_dialog_response(GtkFileChooser *file_chooser, gint response, PopplerAttachment *attachment)
{
    gchar *filename;
    GError *error = nullptr;

    if (response != GTK_RESPONSE_ACCEPT) {
        g_object_unref(attachment);
        gtk_widget_destroy(GTK_WIDGET(file_chooser));
        return;
    }

    filename = gtk_file_chooser_get_filename(file_chooser);
    if (!poppler_attachment_save(attachment, filename, &error)) {
        g_warning("%s", error->message);
        g_error_free(error);
    }
    g_free(filename);
    g_object_unref(attachment);
    gtk_widget_destroy(GTK_WIDGET(file_chooser));
}

static void pgd_attachments_save_button_clicked(GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget *file_chooser;
    PopplerAttachment *attachment;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return;
    }

    gtk_tree_model_get(model, &iter, ATTACHMENTS_ATTACHMENT_COLUMN, &attachment, -1);

    if (!attachment) {
        return;
    }

    file_chooser = gtk_file_chooser_dialog_new("Save attachment", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview))), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), poppler_attachment_get_name(attachment));
    g_signal_connect(G_OBJECT(file_chooser), "response", G_CALLBACK(pgd_attachments_save_dialog_response), (gpointer)attachment);
    gtk_widget_show(file_chooser);
}

static gboolean attachment_save_callback(const gchar *buf, gsize count, gpointer data, GError **error)
{
    GChecksum *cs = (GChecksum *)data;

    g_checksum_update(cs, (guchar *)buf, count);

    return TRUE;
}

static void message_dialog_run(GtkWindow *parent, const gchar *message)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void pgd_attachments_validate_button_clicked(GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    const GString *checksum;
    GChecksum *cs;
    guint8 *digest;
    gsize digest_len;
    PopplerAttachment *attachment;
    gboolean valid = TRUE;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return;
    }

    gtk_tree_model_get(model, &iter, ATTACHMENTS_ATTACHMENT_COLUMN, &attachment, -1);

    if (!attachment) {
        return;
    }

    checksum = poppler_attachment_get_checksum(attachment);

    if (checksum->len == 0) {
        message_dialog_run(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview))), "Impossible to validate attachment: checksum is not available");
        g_object_unref(attachment);

        return;
    }

    cs = g_checksum_new(G_CHECKSUM_MD5);
    poppler_attachment_save_to_callback(attachment, attachment_save_callback, (gpointer)cs, nullptr);
    digest_len = g_checksum_type_get_length(G_CHECKSUM_MD5);
    digest = (guint8 *)g_malloc(digest_len);
    g_checksum_get_digest(cs, digest, &digest_len);
    g_checksum_free(cs);

    if (checksum->len == digest_len) {
        gint i;

        for (i = 0; i < digest_len; i++) {
            if ((guint8)checksum->str[i] != digest[i]) {
                valid = FALSE;
                break;
            }
        }
    }

    if (valid) {
        message_dialog_run(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview))), "Attachment is valid");
    } else {
        message_dialog_run(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(treeview))), "Attachment is not valid: the checksum does not match");
    }

    g_free(digest);
    g_object_unref(attachment);
}

GtkWidget *pgd_attachments_create_widget(PopplerDocument *document)
{
    GtkWidget *vbox;
    GtkWidget *treeview;
    GtkListStore *model;
    GtkWidget *swindow;
    GtkWidget *hbox, *button;
    gboolean has_attachments;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    swindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    has_attachments = poppler_document_has_attachments(document);
    if (has_attachments) {
        model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT);
        pgd_attachments_fill_model(model, document);
        treeview = pgd_attachments_create_list(GTK_TREE_MODEL(model));
    } else {
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        gchar *markup;

        model = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_list_store_append(model, &iter);
        markup = g_strdup_printf("<span size=\"larger\" style=\"italic\">%s</span>", "The document doesn't contain attachments");
        gtk_list_store_set(model, &iter, 0, markup, -1);
        g_free(markup);

        treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 0, "Name", renderer, "markup", 0, nullptr);
    }
    g_object_unref(model);

    gtk_container_add(GTK_CONTAINER(swindow), treeview);
    gtk_widget_show(treeview);

    gtk_box_pack_start(GTK_BOX(vbox), swindow, TRUE, TRUE, 0);
    gtk_widget_show(swindow);

    if (!has_attachments) {
        return vbox;
    }

    hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_SPREAD);

    button = gtk_button_new_with_label("Save");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_attachments_save_button_clicked), (gpointer)treeview);

    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("Validate");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_attachments_validate_button_clicked), (gpointer)treeview);

    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 6);
    gtk_widget_show(hbox);

    return vbox;
}
