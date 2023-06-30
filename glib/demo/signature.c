/*
 * Copyright (C) 2022-2023 Jan-Michael Brummer <jan.brummer@tabos.org>
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
#include <string.h>

#include "signature.h"
#include "utils.h"

typedef struct
{
    PopplerDocument *doc;
    PopplerPage *page;
    GtkWidget *darea;
    cairo_surface_t *surface;
    gint num_page;
    gint redraw_idle;
    GdkPoint start;
    GdkPoint stop;
    gboolean started;
    GdkCursorType cursor;
    GtkWidget *main_box;
    gdouble scale;
} PgdSignatureDemo;

/* Render area */
static cairo_surface_t *pgd_signature_render_page(PgdSignatureDemo *demo)
{
    cairo_t *cr;
    PopplerPage *page;
    gdouble width, height;
    cairo_surface_t *surface = NULL;

    page = poppler_document_get_page(demo->doc, demo->num_page);
    if (!page) {
        return NULL;
    }

    poppler_page_get_size(page, &width, &height);

    width *= demo->scale;
    height *= demo->scale;
    gtk_widget_set_size_request(demo->darea, width, height);

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    cr = cairo_create(surface);

    if (demo->scale != 1.0) {
        cairo_scale(cr, demo->scale, demo->scale);
    }

    cairo_save(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_save(cr);
    poppler_page_render(page, cr);
    cairo_restore(cr);

    cairo_destroy(cr);
    g_object_unref(page);

    return surface;
}

static void draw_selection_rect(PgdSignatureDemo *demo, cairo_t *cr)
{
    gint pos_x1, pos_x2, pos_y1, pos_y2;
    gint x, y, w, h;

    pos_x1 = demo->start.x;
    pos_y1 = demo->start.y;
    pos_x2 = demo->stop.x;
    pos_y2 = demo->stop.y;

    x = MIN(pos_x1, pos_x2);
    y = MIN(pos_y1, pos_y2);
    w = ABS(pos_x1 - pos_x2);
    h = ABS(pos_y1 - pos_y2);

    if (w <= 0 || h <= 0) {
        return;
    }

    cairo_save(cr);

    cairo_rectangle(cr, x + 1, y + 1, w - 2, h - 2);
    cairo_set_source_rgba(cr, 0.2, 0.6, 0.8, 0.2);
    cairo_fill(cr);

    cairo_rectangle(cr, x + 0.5, y + 0.5, w - 1, h - 1);
    cairo_set_source_rgba(cr, 0.2, 0.6, 0.8, 0.35);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);

    cairo_restore(cr);
}

static gboolean pgd_signature_view_drawing_area_draw(GtkWidget *area, cairo_t *cr, PgdSignatureDemo *demo)
{
    if (demo->num_page == -1) {
        return FALSE;
    }

    if (!demo->surface) {
        demo->surface = pgd_signature_render_page(demo);
        if (!demo->surface) {
            return FALSE;
        }
    }

    cairo_set_source_surface(cr, demo->surface, 0, 0);
    cairo_paint(cr);

    if (demo->started) {
        draw_selection_rect(demo, cr);
    }

    return TRUE;
}

static gboolean pgd_signature_viewer_redraw(PgdSignatureDemo *demo)
{
    cairo_surface_destroy(demo->surface);
    demo->surface = NULL;

    gtk_widget_queue_draw(demo->darea);

    demo->redraw_idle = 0;

    return FALSE;
}

static void pgd_signature_viewer_queue_redraw(PgdSignatureDemo *demo)
{
    if (demo->redraw_idle == 0) {
        demo->redraw_idle = g_idle_add((GSourceFunc)pgd_signature_viewer_redraw, demo);
    }
}

static void pgd_signature_page_selector_value_changed(GtkSpinButton *spinbutton, PgdSignatureDemo *demo)
{
    demo->num_page = (gint)gtk_spin_button_get_value(spinbutton) - 1;
    pgd_signature_viewer_queue_redraw(demo);

    demo->page = poppler_document_get_page(demo->doc, demo->num_page);
}

static void pgd_signature_drawing_area_realize(GtkWidget *area, PgdSignatureDemo *demo)
{
    gtk_widget_add_events(area, GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

char *password_callback(const char *in)
{
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *entry;
    char *ret;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, "Enter password");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Enter password to open: %s", in);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    box = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
    entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_box_pack_end(GTK_BOX(box), entry, TRUE, TRUE, 6);
    gtk_widget_show_all(box);

    gtk_dialog_run(GTK_DIALOG(dialog));
    ret = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    gtk_widget_destroy(dialog);

    return ret;
}

static void pgd_signature_update_cursor(PgdSignatureDemo *demo, GdkCursorType cursor_type)
{
    GdkCursor *cursor = NULL;

    if (cursor_type == demo->cursor) {
        return;
    }

    if (cursor_type != GDK_LAST_CURSOR) {
        cursor = gdk_cursor_new_for_display(gtk_widget_get_display(demo->main_box), cursor_type);
    }

    demo->cursor = cursor_type;

    gdk_window_set_cursor(gtk_widget_get_window(demo->main_box), cursor);
    gdk_display_flush(gtk_widget_get_display(demo->main_box));
    if (cursor) {
        g_object_unref(cursor);
    }
}

static void pgd_signature_start_signing(GtkWidget *button, PgdSignatureDemo *demo)
{
    demo->start.x = 0;
    demo->start.y = 0;
    demo->stop.x = 0;
    demo->stop.y = 0;
    demo->started = TRUE;
    pgd_signature_update_cursor(demo, GDK_TCROSS);
}

static gboolean pgd_signature_drawing_area_button_press(GtkWidget *area, GdkEventButton *event, PgdSignatureDemo *demo)
{
    if (!demo->page || event->button != 1 || !demo->started) {
        return FALSE;
    }

    demo->start.x = event->x;
    demo->start.y = event->y;
    demo->stop = demo->start;

    pgd_signature_viewer_queue_redraw(demo);

    return TRUE;
}

static gboolean pgd_signature_drawing_area_motion_notify(GtkWidget *area, GdkEventMotion *event, PgdSignatureDemo *demo)
{
    gdouble width, height;

    if (!demo->page || demo->start.x == -1 || !demo->started) {
        return FALSE;
    }

    demo->stop.x = event->x;
    demo->stop.y = event->y;

    poppler_page_get_size(demo->page, &width, &height);
    width *= demo->scale;
    height *= demo->scale;

    /* Keep the drawing within the page */
    demo->stop.x = CLAMP(demo->stop.x, 0, width);
    demo->stop.y = CLAMP(demo->stop.y, 0, height);

    pgd_signature_viewer_queue_redraw(demo);

    return TRUE;
}

static void on_signing_done(GObject *source, GAsyncResult *result, gpointer user_data)
{
    PopplerDocument *document = POPPLER_DOCUMENT(source);
    GError *error = NULL;
    gboolean ret = poppler_document_sign_finish(document, result, &error);

    g_print("%s: result %d\n", __FUNCTION__, ret);
    if (error) {
        g_print("Error: %s", error->message);
        g_error_free(error);
    }
}

static gboolean pgd_signature_drawing_area_button_release(GtkWidget *area, GdkEventButton *event, PgdSignatureDemo *demo)
{
    if (!demo->page || event->button != 1 || !demo->started) {
        return FALSE;
    }

    demo->started = FALSE;
    pgd_signature_update_cursor(demo, GDK_LAST_CURSOR);

    /* poppler_certificate_set_nss_dir ("./glib/demo/cert"); */
    poppler_set_nss_password_callback(password_callback);
    GList *available_certificates = poppler_get_available_signing_certificates();

    if (available_certificates) {
        char *signature;
        char *signature_left;
        PopplerSigningData *data = poppler_signing_data_new();
        PopplerRectangle rect;
        PopplerCertificateInfo *certificate_info;
        time_t t;
        double width, height;

        certificate_info = available_certificates->data;
        time(&t);

        poppler_signing_data_set_certificate_info(data, certificate_info);
        poppler_signing_data_set_page(data, demo->num_page);
        poppler_signing_data_set_field_partial_name(data, g_uuid_string_random());
        poppler_signing_data_set_destination_filename(data, "test.pdf");
        poppler_signing_data_set_reason(data, "I'm the author");
        poppler_signing_data_set_location(data, "At my desk");

        poppler_page_get_size(demo->page, &width, &height);

        rect.x1 = demo->start.x > demo->stop.x ? demo->stop.x : demo->start.x;
        rect.y1 = demo->start.y > demo->stop.y ? demo->stop.y : demo->start.y;
        rect.x2 = demo->start.x > demo->stop.x ? demo->start.x : demo->stop.x;
        rect.y2 = demo->start.y > demo->stop.y ? demo->start.y : demo->stop.y;

        /* Adjust scale */
        rect.x1 /= demo->scale;
        rect.y1 /= demo->scale;
        rect.x2 /= demo->scale;
        rect.y2 /= demo->scale;

        rect.y1 = height - rect.y1;
        rect.y2 = height - rect.y2;

        poppler_signing_data_set_signature_rectangle(data, &rect);

        signature = g_strdup_printf("Digitally signed by %s\nDate: %s", poppler_certificate_info_get_subject_common_name(certificate_info), ctime(&t));
        poppler_signing_data_set_signature_text(data, signature);
        g_free(signature);

        signature_left = g_strdup_printf("%s", poppler_certificate_info_get_subject_common_name(certificate_info));
        poppler_signing_data_set_signature_text_left(data, signature_left);
        g_free(signature_left);

        poppler_document_sign(demo->doc, data, NULL, on_signing_done, NULL);
    }

    return TRUE;
}

static void pgd_signature_scale_selector_value_changed(GtkSpinButton *spinbutton, PgdSignatureDemo *demo)
{
    demo->scale = gtk_spin_button_get_value(spinbutton);
    pgd_signature_viewer_queue_redraw(demo);
}

/* Main UI */
GtkWidget *pgd_signature_create_widget(PopplerDocument *document)
{
    PgdSignatureDemo *demo;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *button;
    GtkWidget *hbox, *page_selector;
    GtkWidget *scale_hbox, *scale_selector;
    GtkWidget *swindow;
    gchar *str;
    gint n_pages;

    demo = g_new0(PgdSignatureDemo, 1);
    demo->cursor = GDK_LAST_CURSOR;
    demo->doc = g_object_ref(document);
    demo->scale = 1.0;

    n_pages = poppler_document_get_n_pages(document);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    label = gtk_label_new("Page:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);

    page_selector = gtk_spin_button_new_with_range(1, n_pages, 1);
    g_signal_connect(G_OBJECT(page_selector), "value-changed", G_CALLBACK(pgd_signature_page_selector_value_changed), (gpointer)demo);
    gtk_box_pack_start(GTK_BOX(hbox), page_selector, FALSE, TRUE, 0);
    gtk_widget_show(page_selector);

    str = g_strdup_printf("of %d", n_pages);
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    g_free(str);

    scale_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    label = gtk_label_new("Scale:");
    gtk_box_pack_start(GTK_BOX(scale_hbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    scale_selector = gtk_spin_button_new_with_range(0, 10.0, 0.1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scale_selector), 1.0);
    g_signal_connect(G_OBJECT(scale_selector), "value-changed", G_CALLBACK(pgd_signature_scale_selector_value_changed), (gpointer)demo);
    gtk_box_pack_start(GTK_BOX(scale_hbox), scale_selector, TRUE, TRUE, 0);
    gtk_widget_show(scale_selector);

    gtk_box_pack_start(GTK_BOX(hbox), scale_hbox, FALSE, TRUE, 0);
    gtk_widget_show(scale_hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    button = gtk_button_new_with_mnemonic("_Sign");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_signature_start_signing), (gpointer)demo);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_widget_show(hbox);

    /* Demo Area (Render) */
    demo->darea = gtk_drawing_area_new();
    g_signal_connect(demo->darea, "draw", G_CALLBACK(pgd_signature_view_drawing_area_draw), demo);
    g_signal_connect(demo->darea, "realize", G_CALLBACK(pgd_signature_drawing_area_realize), (gpointer)demo);
    g_signal_connect(demo->darea, "button_press_event", G_CALLBACK(pgd_signature_drawing_area_button_press), (gpointer)demo);
    g_signal_connect(demo->darea, "motion_notify_event", G_CALLBACK(pgd_signature_drawing_area_motion_notify), (gpointer)demo);
    g_signal_connect(demo->darea, "button_release_event", G_CALLBACK(pgd_signature_drawing_area_button_release), (gpointer)demo);

    swindow = gtk_scrolled_window_new(NULL, NULL);
#if GTK_CHECK_VERSION(3, 7, 8)
    gtk_container_add(GTK_CONTAINER(swindow), demo->darea);
#else
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swindow), demo->darea);
#endif
    gtk_widget_show(demo->darea);

    gtk_widget_show(swindow);

    gtk_box_pack_start(GTK_BOX(vbox), swindow, TRUE, TRUE, 0);

    demo->main_box = vbox;

    demo->num_page = 0;
    demo->page = poppler_document_get_page(demo->doc, demo->num_page);
    pgd_signature_viewer_queue_redraw(demo);

    return vbox;
}
