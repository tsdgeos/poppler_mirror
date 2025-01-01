/*
 * Copyright (C) 2008 Inigo Martinez <inigomartinez@gmail.com>
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

#include "annots.h"
#include "utils.h"

#define STAMP_CUSTOM_IMAGE "Custom image"

enum
{
    ANNOTS_TYPE_COLUMN,
    ANNOTS_COLOR_COLUMN,
    ANNOTS_FLAG_INVISIBLE_COLUMN,
    ANNOTS_FLAG_HIDDEN_COLUMN,
    ANNOTS_FLAG_PRINT_COLUMN,
    ANNOTS_COLUMN,
    N_COLUMNS
};

enum
{
    SELECTED_TYPE_COLUMN,
    SELECTED_LABEL_COLUMN,
    SELECTED_N_COLUMNS
};

typedef struct
{
    const guint type;
    const gchar *label;
} Annotations;

static const Annotations supported_annots[] = {
    { POPPLER_ANNOT_TEXT, "Text" },           { POPPLER_ANNOT_FREE_TEXT, "Free Text" }, { POPPLER_ANNOT_LINE, "Line" },         { POPPLER_ANNOT_SQUARE, "Square" },         { POPPLER_ANNOT_CIRCLE, "Circle" },
    { POPPLER_ANNOT_HIGHLIGHT, "Highlight" }, { POPPLER_ANNOT_UNDERLINE, "Underline" }, { POPPLER_ANNOT_SQUIGGLY, "Squiggly" }, { POPPLER_ANNOT_STRIKE_OUT, "Strike Out" }, { POPPLER_ANNOT_STAMP, "Stamp" },
};

static const char *stamp_types[] = { [POPPLER_ANNOT_STAMP_ICON_UNKNOWN] = "Unknown",
                                     [POPPLER_ANNOT_STAMP_ICON_APPROVED] = "APPROVED",
                                     [POPPLER_ANNOT_STAMP_ICON_AS_IS] = "AS_IS",
                                     [POPPLER_ANNOT_STAMP_ICON_CONFIDENTIAL] = "CONFIDENTIAL",
                                     [POPPLER_ANNOT_STAMP_ICON_FINAL] = "FINAL",
                                     [POPPLER_ANNOT_STAMP_ICON_EXPERIMENTAL] = "EXPERIMENTAL",
                                     [POPPLER_ANNOT_STAMP_ICON_EXPIRED] = "EXPIRED",
                                     [POPPLER_ANNOT_STAMP_ICON_NOT_APPROVED] = "NOT_APPROVED",
                                     [POPPLER_ANNOT_STAMP_ICON_NOT_FOR_PUBLIC_RELEASE] = "NOT_FOR_PUBLIC_RELEASE",
                                     [POPPLER_ANNOT_STAMP_ICON_SOLD] = "SOLD",
                                     [POPPLER_ANNOT_STAMP_ICON_DEPARTMENTAL] = "DEPARTMENTAL",
                                     [POPPLER_ANNOT_STAMP_ICON_FOR_COMMENT] = "FOR_COMMENT",
                                     [POPPLER_ANNOT_STAMP_ICON_FOR_PUBLIC_RELEASE] = "FOR_PUBLIC_RELEASE",
                                     [POPPLER_ANNOT_STAMP_ICON_TOP_SECRET] = "TOP_SECRET",
                                     [POPPLER_ANNOT_STAMP_ICON_NONE] = "None" };

typedef enum
{
    MODE_NORMAL, /* Regular use as pointer in the page */
    MODE_ADD, /* To add simple annotations */
    MODE_EDIT, /* To move/edit an annotation */
    MODE_DRAWING /* To add annotations that require mouse interactions */
} ModeType;

typedef struct
{
    PopplerDocument *doc;
    PopplerPage *page;
    PopplerAnnot *active_annot;

    GtkWidget *tree_view;
    GtkListStore *model;
    GtkWidget *darea;
    GtkWidget *annot_view;
    GtkWidget *timer_label;
    GtkWidget *remove_button;
    GtkWidget *type_selector;
    GtkWidget *stamp_selector;
    GtkWidget *main_box;

    gint num_page;
    gint annot_type;
    char *custom_image_filename;
    ModeType mode;

    cairo_surface_t *surface;
    GdkRGBA annot_color;

    GdkPoint start;
    GdkPoint stop;
    GdkCursorType cursor;
    guint annotations_idle;
} PgdAnnotsDemo;

static void pgd_annots_viewer_queue_redraw(PgdAnnotsDemo *demo);

static void pgd_annots_free(PgdAnnotsDemo *demo)
{
    if (!demo) {
        return;
    }

    if (demo->annotations_idle > 0) {
        g_source_remove(demo->annotations_idle);
        demo->annotations_idle = 0;
    }

    if (demo->doc) {
        g_object_unref(demo->doc);
        demo->doc = NULL;
    }

    if (demo->page) {
        g_object_unref(demo->page);
        demo->page = NULL;
    }

    if (demo->model) {
        g_object_unref(demo->model);
        demo->model = NULL;
    }

    g_free(demo);
}

static GtkWidget *pgd_annot_view_new(void)
{
    GtkWidget *frame, *label;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Annotation Properties</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_widget_show(label);

    return frame;
}

const gchar *get_annot_type(PopplerAnnot *poppler_annot)
{
    switch (poppler_annot_get_annot_type(poppler_annot)) {
    case POPPLER_ANNOT_TEXT:
        return "Text";
    case POPPLER_ANNOT_LINK:
        return "Link";
    case POPPLER_ANNOT_FREE_TEXT:
        return "Free Text";
    case POPPLER_ANNOT_LINE:
        return "Line";
    case POPPLER_ANNOT_SQUARE:
        return "Square";
    case POPPLER_ANNOT_CIRCLE:
        return "Circle";
    case POPPLER_ANNOT_POLYGON:
        return "Polygon";
    case POPPLER_ANNOT_POLY_LINE:
        return "Poly Line";
    case POPPLER_ANNOT_HIGHLIGHT:
        return "Highlight";
    case POPPLER_ANNOT_UNDERLINE:
        return "Underline";
    case POPPLER_ANNOT_SQUIGGLY:
        return "Squiggly";
    case POPPLER_ANNOT_STRIKE_OUT:
        return "Strike Out";
    case POPPLER_ANNOT_STAMP:
        return "Stamp";
    case POPPLER_ANNOT_CARET:
        return "Caret";
    case POPPLER_ANNOT_INK:
        return "Ink";
    case POPPLER_ANNOT_POPUP:
        return "Popup";
    case POPPLER_ANNOT_FILE_ATTACHMENT:
        return "File Attachment";
    case POPPLER_ANNOT_SOUND:
        return "Sound";
    case POPPLER_ANNOT_MOVIE:
        return "Movie";
    case POPPLER_ANNOT_WIDGET:
        return "Widget";
    case POPPLER_ANNOT_SCREEN:
        return "Screen";
    case POPPLER_ANNOT_PRINTER_MARK:
        return "Printer Mark";
    case POPPLER_ANNOT_TRAP_NET:
        return "Trap Net";
    case POPPLER_ANNOT_WATERMARK:
        return "Watermark";
    case POPPLER_ANNOT_3D:
        return "3D";
    default:
        break;
    }

    return "Unknown";
}

GdkPixbuf *get_annot_color(PopplerAnnot *poppler_annot)
{
    PopplerColor *poppler_color;

    if ((poppler_color = poppler_annot_get_color(poppler_annot))) {
        GdkPixbuf *pixbuf_tmp, *pixbuf;

        pixbuf_tmp = pgd_pixbuf_new_for_color(poppler_color);
        pixbuf = gdk_pixbuf_scale_simple(pixbuf_tmp, 16, 16, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf_tmp);

        g_free(poppler_color);

        return pixbuf;
    }

    return NULL;
}

gchar *get_markup_date(PopplerAnnotMarkup *poppler_annot)
{
    GDate *date;
    struct tm t;
    time_t timet;

    date = poppler_annot_markup_get_date(poppler_annot);
    if (!date) {
        return NULL;
    }

    g_date_to_struct_tm(date, &t);
    g_date_free(date);

    timet = mktime(&t);
    return timet == (time_t)-1 ? NULL : pgd_format_date(timet);
}

const gchar *get_markup_reply_to(PopplerAnnotMarkup *poppler_annot)
{
    switch (poppler_annot_markup_get_reply_to(poppler_annot)) {
    case POPPLER_ANNOT_MARKUP_REPLY_TYPE_R:
        return "Type R";
    case POPPLER_ANNOT_MARKUP_REPLY_TYPE_GROUP:
        return "Type Group";
    default:
        break;
    }

    return "Unknown";
}

const gchar *get_markup_external_data(PopplerAnnotMarkup *poppler_annot)
{
    switch (poppler_annot_markup_get_external_data(poppler_annot)) {
    case POPPLER_ANNOT_EXTERNAL_DATA_MARKUP_3D:
        return "Markup 3D";
    default:
        break;
    }

    return "Unknown";
}

const gchar *get_text_state(PopplerAnnotText *poppler_annot)
{
    switch (poppler_annot_text_get_state(poppler_annot)) {
    case POPPLER_ANNOT_TEXT_STATE_MARKED:
        return "Marked";
    case POPPLER_ANNOT_TEXT_STATE_UNMARKED:
        return "Unmarked";
    case POPPLER_ANNOT_TEXT_STATE_ACCEPTED:
        return "Accepted";
    case POPPLER_ANNOT_TEXT_STATE_REJECTED:
        return "Rejected";
    case POPPLER_ANNOT_TEXT_STATE_CANCELLED:
        return "Cancelled";
    case POPPLER_ANNOT_TEXT_STATE_COMPLETED:
        return "Completed";
    case POPPLER_ANNOT_TEXT_STATE_NONE:
        return "None";
    case POPPLER_ANNOT_TEXT_STATE_UNKNOWN:
        return "Unknown";
    default:
        break;
    }

    return "Unknown";
}

const gchar *get_free_text_quadding(PopplerAnnotFreeText *poppler_annot)
{
    switch (poppler_annot_free_text_get_quadding(poppler_annot)) {
    case POPPLER_ANNOT_FREE_TEXT_QUADDING_LEFT_JUSTIFIED:
        return "Left Justified";
    case POPPLER_ANNOT_FREE_TEXT_QUADDING_CENTERED:
        return "Centered";
    case POPPLER_ANNOT_FREE_TEXT_QUADDING_RIGHT_JUSTIFIED:
        return "Right Justified";
    default:
        break;
    }

    return "Unknown";
}

gchar *get_free_text_callout_line(PopplerAnnotFreeText *poppler_annot)
{
    PopplerAnnotCalloutLine *callout;
    gchar *text;

    if ((callout = poppler_annot_free_text_get_callout_line(poppler_annot))) {
        text = g_strdup_printf("%f,%f,%f,%f", callout->x1, callout->y1, callout->x2, callout->y2);
        if (callout->multiline) {
            text = g_strdup_printf("%s,%f,%f", text, callout->x3, callout->y3);
        }

        return text;
    }

    return NULL;
}

static void pgd_annots_update_cursor(PgdAnnotsDemo *demo, GdkCursorType cursor_type)
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

static void pgd_annots_start_add_annot(GtkWidget *button, PgdAnnotsDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(demo->type_selector), &iter);
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(demo->type_selector));
    gtk_tree_model_get(model, &iter, SELECTED_TYPE_COLUMN, &(demo->annot_type), -1);

    demo->mode = MODE_ADD;
    pgd_annots_update_cursor(demo, GDK_TCROSS);
}

static void pgd_annots_remove_annot(GtkWidget *button, PgdAnnotsDemo *demo)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(demo->tree_view));

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        PopplerAnnot *annot;

        gtk_tree_model_get(model, &iter, ANNOTS_COLUMN, &annot, -1);
        poppler_page_remove_annot(demo->page, annot);
        g_object_unref(annot);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

        pgd_annots_viewer_queue_redraw(demo);
    }
}

static void pgd_annot_view_set_annot_markup(GtkWidget *table, PopplerAnnotMarkup *markup, gint *row)
{
    gchar *text;
    PopplerRectangle rect;

    text = poppler_annot_markup_get_label(markup);
    pgd_table_add_property(GTK_GRID(table), "<b>Label:</b>", text, row);
    g_free(text);

    if (poppler_annot_markup_has_popup(markup)) {
        pgd_table_add_property(GTK_GRID(table), "<b>Popup is open:</b>", poppler_annot_markup_get_popup_is_open(markup) ? "Yes" : "No", row);

        poppler_annot_markup_get_popup_rectangle(markup, &rect);
        text = g_strdup_printf("X1: %.2f, Y1: %.2f, X2: %.2f, Y2: %.2f", rect.x1, rect.y1, rect.x2, rect.y2);
        pgd_table_add_property(GTK_GRID(table), "<b>Popup Rectangle:</b>", text, row);
        g_free(text);
    }

    text = g_strdup_printf("%f", poppler_annot_markup_get_opacity(markup));
    pgd_table_add_property(GTK_GRID(table), "<b>Opacity:</b>", text, row);
    g_free(text);

    text = get_markup_date(markup);
    pgd_table_add_property(GTK_GRID(table), "<b>Date:</b>", text, row);
    g_free(text);

    text = poppler_annot_markup_get_subject(markup);
    pgd_table_add_property(GTK_GRID(table), "<b>Subject:</b>", text, row);
    g_free(text);

    pgd_table_add_property(GTK_GRID(table), "<b>Reply To:</b>", get_markup_reply_to(markup), row);

    pgd_table_add_property(GTK_GRID(table), "<b>External Data:</b>", get_markup_external_data(markup), row);
}

static void pgd_annot_view_set_annot_text(GtkWidget *table, PopplerAnnotText *annot, gint *row)
{
    gchar *text;

    pgd_table_add_property(GTK_GRID(table), "<b>Is open:</b>", poppler_annot_text_get_is_open(annot) ? "Yes" : "No", row);

    text = poppler_annot_text_get_icon(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Icon:</b>", text, row);
    g_free(text);

    pgd_table_add_property(GTK_GRID(table), "<b>State:</b>", get_text_state(annot), row);
}

static void pgd_annot_color_changed(GtkButton *button, GParamSpec *pspec, PgdAnnotsDemo *demo)
{
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &demo->annot_color);
}

static void pgd_annot_view_set_annot_text_markup(GtkWidget *table, PopplerAnnotTextMarkup *annot, gint *row)
{
    gint i;
    gchar *text = NULL;
    gchar *prev_text = NULL;
    GArray *quads_array = NULL;
    PopplerQuadrilateral quadrilateral;

    quads_array = poppler_annot_text_markup_get_quadrilaterals(annot);

    prev_text = g_strdup("");
    for (i = 0; i < quads_array->len; i++) {
        quadrilateral = g_array_index(quads_array, PopplerQuadrilateral, i);

        text = g_strdup_printf("%s%2d:(%.2f,%.2f) (%.2f,%.2f)\n"
                               "    (%.2f,%.2f) (%.2f,%.2f)\n",
                               prev_text, i + 1, quadrilateral.p1.x, quadrilateral.p1.y, quadrilateral.p2.x, quadrilateral.p2.y, quadrilateral.p3.x, quadrilateral.p3.y, quadrilateral.p4.x, quadrilateral.p4.y);
        g_free(prev_text);
        prev_text = text;
    }

    text = g_strchomp(text);
    pgd_table_add_property(GTK_GRID(table), "<b>Quadrilaterals:</b>", text, row);

    g_array_free(quads_array, TRUE);
    g_free(text);
}

static void pgd_annot_view_set_annot_free_text(GtkWidget *table, PopplerAnnotFreeText *annot, gint *row)
{
    gchar *text;

    pgd_table_add_property(GTK_GRID(table), "<b>Quadding:</b>", get_free_text_quadding(annot), row);

    text = get_free_text_callout_line(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Callout:</b>", text, row);
    g_free(text);

    PopplerColor *color = poppler_annot_get_color(POPPLER_ANNOT(annot));

    if (color) {
        text = g_strdup_printf("(%d, %d, %d)", color->red, color->green, color->blue);
    } else {
        text = g_strdup("null");
    }
    pgd_table_add_property(GTK_GRID(table), "<b>Color:</b>", text, row);
    g_free(text);

    PopplerFontDescription *font_desc = poppler_annot_free_text_get_font_desc(annot);
    PangoFontDescription *p = pango_font_description_new();
    pango_font_description_set_family(p, font_desc->font_name);
    pango_font_description_set_stretch(p, (PangoStretch)font_desc->stretch);
    pango_font_description_set_weight(p, (PangoWeight)font_desc->weight);
    pango_font_description_set_style(p, (PangoStyle)font_desc->style);
    pango_font_description_set_size(p, font_desc->size_pt * PANGO_SCALE);
    text = pango_font_description_to_string(p);
    pgd_table_add_property(GTK_GRID(table), "<b>Font:</b>", text, row);
    g_free(text);
}

static void pgd_annot_view_set_annot_stamp(GtkWidget *table, PopplerAnnotStamp *annot, gint *row)
{
    PopplerAnnotStampIcon icon;

    icon = poppler_annot_stamp_get_icon(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Icon Name:</b>", stamp_types[icon], row);
}

static void pgd_annots_file_attachment_save_dialog_response(GtkFileChooser *file_chooser, gint response, PopplerAttachment *attachment)
{
    gchar *filename;
    GError *error = NULL;

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

static void pgd_annot_save_file_attachment_button_clicked(GtkButton *button, PopplerAnnotFileAttachment *annot)
{
    GtkWidget *file_chooser;
    PopplerAttachment *attachment;

    attachment = poppler_annot_file_attachment_get_attachment(annot);
    if (!attachment) {
        return;
    }

    file_chooser = gtk_file_chooser_dialog_new("Save attachment", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), poppler_attachment_get_name(attachment));
    g_signal_connect(G_OBJECT(file_chooser), "response", G_CALLBACK(pgd_annots_file_attachment_save_dialog_response), (gpointer)attachment);
    gtk_widget_show(file_chooser);
}

static void pgd_annot_view_set_annot_file_attachment(GtkWidget *table, PopplerAnnotFileAttachment *annot, gint *row)
{
    GtkWidget *button;
    gchar *text;

    text = poppler_annot_file_attachment_get_name(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Attachment Name:</b>", text, row);
    g_free(text);

    button = gtk_button_new_with_label("Save Attachment");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_annot_save_file_attachment_button_clicked), (gpointer)annot);
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>File Attachment:</b>", button, row);
    gtk_widget_show(button);
}

static void pgd_annot_view_set_annot_movie(GtkWidget *table, PopplerAnnotMovie *annot, gint *row)
{
    GtkWidget *movie_view;
    gchar *text;

    text = poppler_annot_movie_get_title(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Movie Title:</b>", text, row);
    g_free(text);

    movie_view = pgd_movie_view_new();
    pgd_movie_view_set_movie(movie_view, poppler_annot_movie_get_movie(annot));
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Movie:</b>", movie_view, row);
    gtk_widget_show(movie_view);
}

static void pgd_annot_view_set_annot_screen(GtkWidget *table, PopplerAnnotScreen *annot, gint *row)
{
    GtkWidget *action_view;

    action_view = pgd_action_view_new(NULL);
    pgd_action_view_set_action(action_view, poppler_annot_screen_get_action(annot));
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Action:</b>", action_view, row);
    gtk_widget_show(action_view);
}

static void pgd_annot_view_set_annot(PgdAnnotsDemo *demo, PopplerAnnot *annot)
{
    GtkWidget *table;
    gint row = 0;
    gchar *text;
    time_t timet;
    PopplerRectangle rect;

    table = gtk_bin_get_child(GTK_BIN(demo->annot_view));
    if (table) {
        gtk_container_remove(GTK_CONTAINER(demo->annot_view), table);
    }

    if (!annot) {
        return;
    }

    table = gtk_grid_new();
    gtk_widget_set_margin_top(table, 5);
    gtk_widget_set_margin_bottom(table, 5);
    gtk_widget_set_margin_start(table, 8);
    gtk_widget_set_margin_end(table, 5);
    gtk_grid_set_column_spacing(GTK_GRID(table), 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);

    text = poppler_annot_get_contents(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Contents:</b>", text, &row);
    g_free(text);

    text = poppler_annot_get_name(annot);
    pgd_table_add_property(GTK_GRID(table), "<b>Name:</b>", text, &row);
    g_free(text);

    text = poppler_annot_get_modified(annot);
    if (poppler_date_parse(text, &timet)) {
        g_free(text);
        text = pgd_format_date(timet);
    }
    pgd_table_add_property(GTK_GRID(table), "<b>Modified:</b>", text, &row);
    g_free(text);

    poppler_annot_get_rectangle(annot, &rect);
    text = g_strdup_printf("(%.2f;%.2f) (%.2f;%.2f)", rect.x1, rect.y1, rect.x2, rect.y2);
    pgd_table_add_property(GTK_GRID(table), "<b>Coords:</b>", text, &row);
    g_free(text);

    double border_width;
    if (poppler_annot_get_border_width(POPPLER_ANNOT(annot), &border_width)) {
        text = g_strdup_printf("%f", border_width);
    } else {
        text = g_strdup("unset");
    }
    pgd_table_add_property(GTK_GRID(table), "<b>Border Width:</b>", text, &row);
    g_free(text);

    if (POPPLER_IS_ANNOT_MARKUP(annot)) {
        pgd_annot_view_set_annot_markup(table, POPPLER_ANNOT_MARKUP(annot), &row);
    }

    switch (poppler_annot_get_annot_type(annot)) {
    case POPPLER_ANNOT_TEXT:
        pgd_annot_view_set_annot_text(table, POPPLER_ANNOT_TEXT(annot), &row);
        break;
    case POPPLER_ANNOT_HIGHLIGHT:
    case POPPLER_ANNOT_UNDERLINE:
    case POPPLER_ANNOT_SQUIGGLY:
    case POPPLER_ANNOT_STRIKE_OUT:
        pgd_annot_view_set_annot_text_markup(table, POPPLER_ANNOT_TEXT_MARKUP(annot), &row);
        break;
    case POPPLER_ANNOT_FREE_TEXT:
        pgd_annot_view_set_annot_free_text(table, POPPLER_ANNOT_FREE_TEXT(annot), &row);
        break;
    case POPPLER_ANNOT_FILE_ATTACHMENT:
        pgd_annot_view_set_annot_file_attachment(table, POPPLER_ANNOT_FILE_ATTACHMENT(annot), &row);
        break;
    case POPPLER_ANNOT_MOVIE:
        pgd_annot_view_set_annot_movie(table, POPPLER_ANNOT_MOVIE(annot), &row);
        break;
    case POPPLER_ANNOT_SCREEN:
        pgd_annot_view_set_annot_screen(table, POPPLER_ANNOT_SCREEN(annot), &row);
        break;
    case POPPLER_ANNOT_STAMP:
        pgd_annot_view_set_annot_stamp(table, POPPLER_ANNOT_STAMP(annot), &row);
        break;
    default:
        break;
    }

    gtk_container_add(GTK_CONTAINER(demo->annot_view), table);
    gtk_widget_show(table);
}

static void pgd_annots_add_annot_to_model(PgdAnnotsDemo *demo, PopplerAnnot *annot, PopplerRectangle area, gboolean selected)
{
    GtkTreeIter iter;
    GdkPixbuf *pixbuf;
    PopplerAnnotFlag flags;

    pixbuf = get_annot_color(annot);
    flags = poppler_annot_get_flags(annot);

    gtk_list_store_append(demo->model, &iter);
    gtk_list_store_set(demo->model, &iter, ANNOTS_TYPE_COLUMN, get_annot_type(annot), ANNOTS_COLOR_COLUMN, pixbuf, ANNOTS_FLAG_INVISIBLE_COLUMN, (flags & POPPLER_ANNOT_FLAG_INVISIBLE), ANNOTS_FLAG_HIDDEN_COLUMN,
                       (flags & POPPLER_ANNOT_FLAG_HIDDEN), ANNOTS_FLAG_PRINT_COLUMN, (flags & POPPLER_ANNOT_FLAG_PRINT), ANNOTS_COLUMN, annot, -1);

    if (selected) {
        GtkTreePath *path;

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(demo->model), &iter);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(demo->tree_view), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }

    if (pixbuf) {
        g_object_unref(pixbuf);
    }
}

static void pgd_annots_get_annots(PgdAnnotsDemo *demo)
{
    GList *mapping, *l;
    gint n_fields;
    GTimer *timer;

    gtk_list_store_clear(demo->model);
    pgd_annot_view_set_annot(demo, NULL);

    if (demo->page) {
        g_object_unref(demo->page);
        demo->page = NULL;
    }

    demo->page = poppler_document_get_page(demo->doc, demo->num_page);
    if (!demo->page) {
        return;
    }

    timer = g_timer_new();
    mapping = poppler_page_get_annot_mapping(demo->page);
    g_timer_stop(timer);

    n_fields = g_list_length(mapping);
    if (n_fields > 0) {
        gchar *str;

        str = g_strdup_printf("<i>%d annotations found in %.4f seconds</i>", n_fields, g_timer_elapsed(timer, NULL));
        gtk_label_set_markup(GTK_LABEL(demo->timer_label), str);
        g_free(str);
    } else {
        gtk_label_set_markup(GTK_LABEL(demo->timer_label), "<i>No annotations found</i>");
    }

    g_timer_destroy(timer);

    for (l = mapping; l; l = g_list_next(l)) {
        PopplerAnnotMapping *amapping;

        amapping = (PopplerAnnotMapping *)l->data;
        pgd_annots_add_annot_to_model(demo, amapping->annot, amapping->area, FALSE);
    }

    poppler_page_free_annot_mapping(mapping);
}

static void pgd_annots_page_selector_value_changed(GtkSpinButton *spinbutton, PgdAnnotsDemo *demo)
{
    demo->num_page = (gint)gtk_spin_button_get_value(spinbutton) - 1;
    pgd_annots_viewer_queue_redraw(demo);
    pgd_annots_get_annots(demo);
}

static void pgd_annots_selection_changed(GtkTreeSelection *treeselection, PgdAnnotsDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
        PopplerAnnot *annot;

        gtk_tree_model_get(model, &iter, ANNOTS_COLUMN, &annot, -1);
        pgd_annot_view_set_annot(demo, annot);
        g_object_unref(annot);

        gtk_widget_set_sensitive(demo->remove_button, TRUE);
    } else {
        pgd_annot_view_set_annot(demo, NULL);
        gtk_widget_set_sensitive(demo->remove_button, FALSE);
    }
}

static void pgd_annots_flags_toggled(GtkCellRendererToggle *renderer, gchar *path_str, PgdAnnotsDemo *demo, gint column, PopplerAnnotFlag flag_bit)
{
    GtkTreeIter iter;
    gboolean fixed;
    PopplerAnnot *annot;
    PopplerAnnotFlag flags;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreeModel *model = GTK_TREE_MODEL(demo->model);

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, column, &fixed, ANNOTS_COLUMN, &annot, -1);

    fixed ^= 1;
    flags = poppler_annot_get_flags(annot);

    if (fixed) {
        flags |= flag_bit;
    } else {
        flags &= ~flag_bit;
    }

    poppler_annot_set_flags(annot, flags);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, fixed, -1);

    pgd_annot_view_set_annot(demo, annot);
    gtk_tree_path_free(path);

    pgd_annots_viewer_queue_redraw(demo);
}

static void pgd_annots_hidden_flag_toggled(GtkCellRendererToggle *renderer, gchar *path_str, PgdAnnotsDemo *demo)
{
    pgd_annots_flags_toggled(renderer, path_str, demo, ANNOTS_FLAG_HIDDEN_COLUMN, POPPLER_ANNOT_FLAG_HIDDEN);
}

static void pgd_annots_print_flag_toggled(GtkCellRendererToggle *renderer, gchar *path_str, PgdAnnotsDemo *demo)
{
    pgd_annots_flags_toggled(renderer, path_str, demo, ANNOTS_FLAG_PRINT_COLUMN, POPPLER_ANNOT_FLAG_PRINT);
}

static void pgd_annots_invisible_flag_toggled(GtkCellRendererToggle *renderer, gchar *path_str, PgdAnnotsDemo *demo)
{
    pgd_annots_flags_toggled(renderer, path_str, demo, ANNOTS_FLAG_INVISIBLE_COLUMN, POPPLER_ANNOT_FLAG_INVISIBLE);
}

static inline void pgd_annots_set_poppler_quad_from_rectangle(PopplerQuadrilateral *quad, PopplerRectangle *rect)
{
    quad->p1.x = rect->x1;
    quad->p1.y = rect->y1;
    quad->p2.x = rect->x2;
    quad->p2.y = rect->y1;
    quad->p3.x = rect->x1;
    quad->p3.y = rect->y2;
    quad->p4.x = rect->x2;
    quad->p4.y = rect->y2;
}

static GArray *pgd_annots_create_quads_array_for_rectangle(PopplerRectangle *rect)
{
    GArray *quads_array;
    PopplerQuadrilateral *quad;

    quads_array = g_array_sized_new(FALSE, FALSE, sizeof(PopplerQuadrilateral), 1);
    g_array_set_size(quads_array, 1);

    quad = &g_array_index(quads_array, PopplerQuadrilateral, 0);
    pgd_annots_set_poppler_quad_from_rectangle(quad, rect);

    return quads_array;
}

static PopplerAnnotStampIcon get_icon_from_stamp_text(gchar *icon_text)
{
    int i;

    for (i = 1; i < G_N_ELEMENTS(stamp_types) - 1; i++) {
        if (strcmp(stamp_types[i], icon_text) == 0) {
            return (PopplerAnnotStampIcon)i;
        }
    }

    return POPPLER_ANNOT_STAMP_ICON_UNKNOWN;
}

static void pgd_annots_add_annot(PgdAnnotsDemo *demo)
{
    PopplerRectangle rect;
    PopplerColor color;
    PopplerAnnot *annot;
    gdouble height;

    g_assert(demo->mode == MODE_ADD);

    poppler_page_get_size(demo->page, NULL, &height);

    rect.x1 = demo->start.x;
    rect.y1 = height - demo->start.y;
    rect.x2 = demo->stop.x;
    rect.y2 = height - demo->stop.y;

    color.red = CLAMP((guint)(demo->annot_color.red * 65535), 0, 65535);
    color.green = CLAMP((guint)(demo->annot_color.green * 65535), 0, 65535);
    color.blue = CLAMP((guint)(demo->annot_color.blue * 65535), 0, 65535);

    switch (demo->annot_type) {
    case POPPLER_ANNOT_TEXT:
        annot = poppler_annot_text_new(demo->doc, &rect);

        break;
    case POPPLER_ANNOT_FREE_TEXT:
        annot = poppler_annot_free_text_new(demo->doc, &rect);
        poppler_annot_set_contents(annot, "Free Text");
        PopplerFontDescription *font_desc = poppler_font_description_new("DejaVu Sans");
        font_desc->size_pt = 17;
        font_desc->style = POPPLER_STYLE_OBLIQUE;
        font_desc->weight = POPPLER_WEIGHT_BOLD;
        font_desc->stretch = POPPLER_STRETCH_CONDENSED;
        poppler_annot_free_text_set_font_desc(POPPLER_ANNOT_FREE_TEXT(annot), font_desc);
        poppler_font_description_free(font_desc);
        break;
    case POPPLER_ANNOT_LINE: {
        PopplerPoint start, end;

        start.x = rect.x1;
        start.y = rect.y1;
        end.x = rect.x2;
        end.y = rect.y2;

        annot = poppler_annot_line_new(demo->doc, &rect, &start, &end);
    } break;
    case POPPLER_ANNOT_SQUARE:
        annot = poppler_annot_square_new(demo->doc, &rect);
        break;
    case POPPLER_ANNOT_CIRCLE:
        annot = poppler_annot_circle_new(demo->doc, &rect);
        break;
    case POPPLER_ANNOT_HIGHLIGHT: {
        GArray *quads_array;

        quads_array = pgd_annots_create_quads_array_for_rectangle(&rect);
        annot = poppler_annot_text_markup_new_highlight(demo->doc, &rect, quads_array);
        g_array_free(quads_array, TRUE);
    } break;
    case POPPLER_ANNOT_UNDERLINE: {
        GArray *quads_array;

        quads_array = pgd_annots_create_quads_array_for_rectangle(&rect);
        annot = poppler_annot_text_markup_new_underline(demo->doc, &rect, quads_array);
        g_array_free(quads_array, TRUE);
    } break;
    case POPPLER_ANNOT_SQUIGGLY: {
        GArray *quads_array;

        quads_array = pgd_annots_create_quads_array_for_rectangle(&rect);
        annot = poppler_annot_text_markup_new_squiggly(demo->doc, &rect, quads_array);
        g_array_free(quads_array, TRUE);
    } break;
    case POPPLER_ANNOT_STRIKE_OUT: {
        GArray *quads_array;

        quads_array = pgd_annots_create_quads_array_for_rectangle(&rect);
        annot = poppler_annot_text_markup_new_strikeout(demo->doc, &rect, quads_array);
        g_array_free(quads_array, TRUE);
    } break;
    case POPPLER_ANNOT_STAMP: {
        annot = poppler_annot_stamp_new(demo->doc, &rect);
        gchar *stamp_type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(demo->stamp_selector));
        GError *error = NULL;

        if (strcmp(stamp_type, STAMP_CUSTOM_IMAGE) == 0 && demo->custom_image_filename) {
            cairo_surface_t *img = cairo_image_surface_create_from_png(demo->custom_image_filename);
            if (cairo_surface_status(img) == CAIRO_STATUS_SUCCESS) {
                poppler_annot_stamp_set_custom_image(POPPLER_ANNOT_STAMP(annot), img, &error);
                if (error) {
                    g_warning("%s", error->message);
                    g_error_free(error);
                }
            }
            cairo_surface_destroy(img);
        } else {
            poppler_annot_stamp_set_icon(POPPLER_ANNOT_STAMP(annot), get_icon_from_stamp_text(stamp_type));
        }
        g_free(stamp_type);
    } break;
    default:
        g_assert_not_reached();
    }

    demo->active_annot = annot;

    if (demo->annot_type != POPPLER_ANNOT_STAMP) {
        poppler_annot_set_color(annot, &color);
    }
    poppler_page_add_annot(demo->page, annot);
    pgd_annots_add_annot_to_model(demo, annot, rect, TRUE);
    g_object_unref(annot);
}

static void pgd_annots_finish_add_annot(PgdAnnotsDemo *demo)
{
    g_assert(demo->mode == MODE_ADD || demo->mode == MODE_DRAWING);

    demo->mode = MODE_NORMAL;
    demo->start.x = -1;
    pgd_annots_update_cursor(demo, GDK_LAST_CURSOR);
    pgd_annots_viewer_queue_redraw(demo);

    gtk_label_set_text(GTK_LABEL(demo->timer_label), NULL);
}

static void pgd_annots_update_selected_text(PgdAnnotsDemo *demo)
{
    PopplerRectangle doc_area, *rects = NULL, *r = NULL;
    gdouble width, height;
    GArray *quads_array = NULL;
    guint n_rects;
    gint i, lines = 0;
    GList *l_rects = NULL, *list;

    poppler_page_get_size(demo->page, &width, &height);

    doc_area.x1 = demo->start.x;
    doc_area.y1 = demo->start.y;
    doc_area.x2 = demo->stop.x;
    doc_area.y2 = demo->stop.y;

    if (!poppler_page_get_text_layout_for_area(demo->page, &doc_area, &rects, &n_rects)) {
        return;
    }

    r = g_slice_new(PopplerRectangle);
    r->x1 = G_MAXDOUBLE;
    r->y1 = G_MAXDOUBLE;
    r->x2 = G_MINDOUBLE;
    r->y2 = G_MINDOUBLE;

    for (i = 0; i < n_rects; i++) {
        /* Check if the rectangle belongs to the same line.
           On a new line, start a new target rectangle.
           On the same line, make an union of rectangles at
           the same line */
        if (ABS(r->y2 - rects[i].y2) > 0.0001) {
            if (i > 0) {
                l_rects = g_list_append(l_rects, r);
            }
            r = g_slice_new(PopplerRectangle);
            r->x1 = rects[i].x1;
            r->y1 = rects[i].y1;
            r->x2 = rects[i].x2;
            r->y2 = rects[i].y2;
            lines++;
        } else {
            r->x1 = MIN(r->x1, rects[i].x1);
            r->y1 = MIN(r->y1, rects[i].y1);
            r->x2 = MAX(r->x2, rects[i].x2);
            r->y2 = MAX(r->y2, rects[i].y2);
        }
    }

    l_rects = g_list_append(l_rects, r);
    l_rects = g_list_reverse(l_rects);

    quads_array = g_array_sized_new(TRUE, TRUE, sizeof(PopplerQuadrilateral), lines);
    g_array_set_size(quads_array, lines);

    for (list = l_rects, i = 0; list; list = list->next, i++) {
        PopplerQuadrilateral *quad;

        quad = &g_array_index(quads_array, PopplerQuadrilateral, i);
        r = (PopplerRectangle *)list->data;
        quad->p1.x = r->x1;
        quad->p1.y = height - r->y1;
        quad->p2.x = r->x2;
        quad->p2.y = height - r->y1;
        quad->p3.x = r->x1;
        quad->p3.y = height - r->y2;
        quad->p4.x = r->x2;
        quad->p4.y = height - r->y2;
        g_slice_free(PopplerRectangle, r);
    }

    poppler_annot_text_markup_set_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(demo->active_annot), quads_array);
    g_array_free(quads_array, TRUE);
    g_free(rects);
    g_list_free(l_rects);
}

/* Render area */
static cairo_surface_t *pgd_annots_render_page(PgdAnnotsDemo *demo)
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
    gtk_widget_set_size_request(demo->darea, width, height);

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    cr = cairo_create(surface);

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

static gboolean pgd_annots_view_drawing_area_draw(GtkWidget *area, cairo_t *cr, PgdAnnotsDemo *demo)
{
    if (demo->num_page == -1) {
        return FALSE;
    }

    if (!demo->surface) {
        demo->surface = pgd_annots_render_page(demo);
        if (!demo->surface) {
            return FALSE;
        }
    }

    cairo_set_source_surface(cr, demo->surface, 0, 0);
    cairo_paint(cr);

    return TRUE;
}

static gboolean pgd_annots_viewer_redraw(PgdAnnotsDemo *demo)
{
    cairo_surface_destroy(demo->surface);
    demo->surface = NULL;

    gtk_widget_queue_draw(demo->darea);

    demo->annotations_idle = 0;

    return FALSE;
}

static void pgd_annots_viewer_queue_redraw(PgdAnnotsDemo *demo)
{
    if (demo->annotations_idle == 0) {
        demo->annotations_idle = g_idle_add((GSourceFunc)pgd_annots_viewer_redraw, demo);
    }
}

static void pgd_annots_drawing_area_realize(GtkWidget *area, PgdAnnotsDemo *demo)
{
    gtk_widget_add_events(area, GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

static gboolean pgd_annots_drawing_area_button_press(GtkWidget *area, GdkEventButton *event, PgdAnnotsDemo *demo)
{
    if (!demo->page || demo->mode != MODE_ADD || event->button != 1) {
        return FALSE;
    }

    demo->start.x = event->x;
    demo->start.y = event->y;
    demo->stop = demo->start;

    pgd_annots_add_annot(demo);
    pgd_annots_viewer_queue_redraw(demo);
    demo->mode = MODE_DRAWING;

    return TRUE;
}

static void choose_custom_image(PgdAnnotsDemo *demo)
{
    GtkFileChooser *chooser_dialog;
    gint response;
    const gchar *chooser_dir = "/usr/share/pixmaps";
    const gchar *pics_dir;
    GtkFileFilter *filter;

    chooser_dialog = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Select PNG Image", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "gtk-cancel", GTK_RESPONSE_CANCEL, "gtk-open", GTK_RESPONSE_ACCEPT, NULL));
    gtk_window_set_modal(GTK_WINDOW(chooser_dialog), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(chooser_dialog), GTK_RESPONSE_ACCEPT);

    gtk_file_chooser_add_shortcut_folder(chooser_dialog, chooser_dir, NULL);
    pics_dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    if (pics_dir != NULL) {
        gtk_file_chooser_add_shortcut_folder(chooser_dialog, pics_dir, NULL);
    }

    if (!g_file_test(chooser_dir, G_FILE_TEST_IS_DIR)) {
        chooser_dir = g_get_home_dir();
    }

    gtk_file_chooser_set_current_folder(chooser_dialog, chooser_dir);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PNG images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_chooser_add_filter(chooser_dialog, filter);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All Files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser_dialog, filter);

    response = gtk_dialog_run(GTK_DIALOG(chooser_dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        if (demo->custom_image_filename) {
            g_free(demo->custom_image_filename);
        }

        demo->custom_image_filename = gtk_file_chooser_get_filename(chooser_dialog);
    } else {
        if (demo->custom_image_filename) {
            g_free(demo->custom_image_filename);
            demo->custom_image_filename = NULL;
        }
    }

    gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

static void stamp_selector_changed(GtkComboBox *combo_box, PgdAnnotsDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *active;

    model = gtk_combo_box_get_model(combo_box);
    gtk_combo_box_get_active_iter(combo_box, &iter);
    gtk_tree_model_get(model, &iter, 0, &active, -1);
    if (strcmp(active, STAMP_CUSTOM_IMAGE) == 0) {
        choose_custom_image(demo);
    }
}

static void type_selector_changed(GtkComboBox *combo_box, PgdAnnotsDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    int active;

    model = gtk_combo_box_get_model(combo_box);
    gtk_combo_box_get_active_iter(combo_box, &iter);
    gtk_tree_model_get(model, &iter, SELECTED_TYPE_COLUMN, &active, -1);
    gtk_widget_set_sensitive(demo->stamp_selector, active == POPPLER_ANNOT_STAMP);
}

static gboolean pgd_annots_drawing_area_motion_notify(GtkWidget *area, GdkEventMotion *event, PgdAnnotsDemo *demo)
{
    PopplerRectangle rect;
    PopplerPoint start, end;
    gdouble width, height;

    if (!demo->page || demo->mode != MODE_DRAWING || demo->start.x == -1) {
        return FALSE;
    }

    demo->stop.x = event->x;
    demo->stop.y = event->y;

    poppler_page_get_size(demo->page, &width, &height);

    /* Keep the drawing within the page */
    demo->stop.x = CLAMP(demo->stop.x, 0, width);
    demo->stop.y = CLAMP(demo->stop.y, 0, height);

    rect.x1 = start.x = demo->start.x;
    rect.y1 = start.y = height - demo->start.y;
    rect.x2 = end.x = demo->stop.x;
    rect.y2 = end.y = height - demo->stop.y;

    poppler_annot_set_rectangle(demo->active_annot, &rect);

    if (demo->annot_type == POPPLER_ANNOT_LINE) {
        poppler_annot_line_set_vertices(POPPLER_ANNOT_LINE(demo->active_annot), &start, &end);
    }

    if (POPPLER_IS_ANNOT_TEXT_MARKUP(demo->active_annot)) {
        pgd_annots_update_selected_text(demo);
    }

    pgd_annot_view_set_annot(demo, demo->active_annot);
    pgd_annots_viewer_queue_redraw(demo);

    return TRUE;
}

static gboolean pgd_annots_drawing_area_button_release(GtkWidget *area, GdkEventButton *event, PgdAnnotsDemo *demo)
{
    if (!demo->page || demo->mode != MODE_DRAWING || event->button != 1) {
        return FALSE;
    }

    pgd_annots_finish_add_annot(demo);

    return TRUE;
}

/* Main UI */
GtkWidget *pgd_annots_create_widget(PopplerDocument *document)
{
    PgdAnnotsDemo *demo;
    GtkWidget *label;
    GtkWidget *vbox, *vbox2;
    GtkWidget *button;
    GtkWidget *hbox, *hbox2, *page_selector;
    GtkWidget *hpaned;
    GtkWidget *swindow, *treeview;
    GtkTreeSelection *selection;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *model;
    GtkTreeIter iter;
    gchar *str;
    gint n_pages;
    gint i;

    demo = g_new0(PgdAnnotsDemo, 1);

    demo->doc = g_object_ref(document);
    demo->cursor = GDK_LAST_CURSOR;
    demo->mode = MODE_NORMAL;

    n_pages = poppler_document_get_n_pages(document);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    label = gtk_label_new("Page:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);

    page_selector = gtk_spin_button_new_with_range(1, n_pages, 1);
    g_signal_connect(G_OBJECT(page_selector), "value-changed", G_CALLBACK(pgd_annots_page_selector_value_changed), (gpointer)demo);
    gtk_box_pack_start(GTK_BOX(hbox), page_selector, FALSE, TRUE, 0);
    gtk_widget_show(page_selector);

    str = g_strdup_printf("of %d", n_pages);
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    g_free(str);

    demo->remove_button = gtk_button_new_with_mnemonic("_Remove");
    gtk_widget_set_sensitive(demo->remove_button, FALSE);
    g_signal_connect(G_OBJECT(demo->remove_button), "clicked", G_CALLBACK(pgd_annots_remove_annot), (gpointer)demo);
    gtk_box_pack_end(GTK_BOX(hbox), demo->remove_button, FALSE, FALSE, 6);
    gtk_widget_show(demo->remove_button);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 0);

    button = gtk_button_new_with_mnemonic("_Add");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_annots_start_add_annot), (gpointer)demo);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    model = gtk_list_store_new(SELECTED_N_COLUMNS, G_TYPE_INT, G_TYPE_STRING);

    for (i = 0; i < G_N_ELEMENTS(supported_annots); i++) {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, SELECTED_TYPE_COLUMN, supported_annots[i].type, SELECTED_LABEL_COLUMN, supported_annots[i].label, -1);
    }

    demo->type_selector = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    g_signal_connect(demo->type_selector, "changed", G_CALLBACK(type_selector_changed), (gpointer)demo);
    g_object_unref(model);

    demo->stamp_selector = gtk_combo_box_text_new();
    for (i = 1; i < G_N_ELEMENTS(stamp_types) - 1; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(demo->stamp_selector), stamp_types[i]);
    }
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(demo->stamp_selector), STAMP_CUSTOM_IMAGE);

    gtk_combo_box_set_active(GTK_COMBO_BOX(demo->stamp_selector), 0);
    g_signal_connect(demo->stamp_selector, "changed", G_CALLBACK(stamp_selector_changed), (gpointer)demo);
    gtk_widget_set_sensitive(demo->stamp_selector, FALSE);
    label = gtk_label_new("Stamp type: ");
    gtk_widget_set_sensitive(label, FALSE);
    g_object_bind_property(demo->stamp_selector, "sensitive", label, "sensitive", 0);
    gtk_box_pack_end(GTK_BOX(hbox2), demo->stamp_selector, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
    gtk_widget_show_all(hbox2);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(demo->type_selector), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(demo->type_selector), renderer, "text", SELECTED_LABEL_COLUMN, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(demo->type_selector), 0);
    gtk_box_pack_end(GTK_BOX(hbox), demo->type_selector, FALSE, FALSE, 0);
    gtk_widget_show(demo->type_selector);

    button = gtk_color_button_new();
    demo->annot_color.red = 65535;
    demo->annot_color.alpha = 1.0;
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button), &demo->annot_color);
    g_signal_connect(button, "notify::color", G_CALLBACK(pgd_annot_color_changed), (gpointer)demo);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
    gtk_widget_show(button);

    gtk_widget_show(hbox);
    gtk_widget_show(hbox2);

    demo->timer_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(demo->timer_label), "<i>No annotations found</i>");
    g_object_set(G_OBJECT(demo->timer_label), "xalign", 1.0, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), demo->timer_label, FALSE, TRUE, 0);
    gtk_widget_show(demo->timer_label);

    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    demo->annot_view = pgd_annot_view_new();

    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    demo->model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_OBJECT);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(demo->model));
    demo->tree_view = treeview;

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, "Type");
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", ANNOTS_COLOR_COLUMN);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", ANNOTS_TYPE_COLUMN);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(pgd_annots_invisible_flag_toggled), (gpointer)demo);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), ANNOTS_FLAG_INVISIBLE_COLUMN, "Invisible", renderer, "active", ANNOTS_FLAG_INVISIBLE_COLUMN, NULL);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(pgd_annots_hidden_flag_toggled), (gpointer)demo);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), ANNOTS_FLAG_HIDDEN_COLUMN, "Hidden", renderer, "active", ANNOTS_FLAG_HIDDEN_COLUMN, NULL);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(pgd_annots_print_flag_toggled), (gpointer)demo);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), ANNOTS_FLAG_PRINT_COLUMN, "Print", renderer, "active", ANNOTS_FLAG_PRINT_COLUMN, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(pgd_annots_selection_changed), (gpointer)demo);

    /* Annotation's list */
    gtk_container_add(GTK_CONTAINER(swindow), treeview);
    gtk_widget_show(treeview);

    gtk_box_pack_start(GTK_BOX(vbox2), swindow, TRUE, TRUE, 0);
    gtk_widget_show(swindow);

    /* Annotation Properties */
    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(swindow), demo->annot_view);
    gtk_widget_show(demo->annot_view);
    gtk_widget_show(swindow);

    gtk_box_pack_start(GTK_BOX(vbox2), swindow, TRUE, TRUE, 6);
    gtk_widget_show(swindow);

    gtk_paned_add1(GTK_PANED(hpaned), vbox2);
    gtk_widget_show(vbox2);

    /* Demo Area (Render) */
    demo->darea = gtk_drawing_area_new();
    g_signal_connect(demo->darea, "draw", G_CALLBACK(pgd_annots_view_drawing_area_draw), demo);
    g_signal_connect(demo->darea, "realize", G_CALLBACK(pgd_annots_drawing_area_realize), (gpointer)demo);
    g_signal_connect(demo->darea, "button_press_event", G_CALLBACK(pgd_annots_drawing_area_button_press), (gpointer)demo);
    g_signal_connect(demo->darea, "motion_notify_event", G_CALLBACK(pgd_annots_drawing_area_motion_notify), (gpointer)demo);
    g_signal_connect(demo->darea, "button_release_event", G_CALLBACK(pgd_annots_drawing_area_button_release), (gpointer)demo);

    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swindow), demo->darea);
    gtk_widget_show(demo->darea);

    gtk_paned_add2(GTK_PANED(hpaned), swindow);
    gtk_widget_show(swindow);

    gtk_paned_set_position(GTK_PANED(hpaned), 300);

    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
    gtk_widget_show(hpaned);

    g_object_weak_ref(G_OBJECT(vbox), (GWeakNotify)pgd_annots_free, demo);

    pgd_annots_viewer_queue_redraw(demo);
    pgd_annots_get_annots(demo);

    demo->main_box = vbox;

    return vbox;
}
