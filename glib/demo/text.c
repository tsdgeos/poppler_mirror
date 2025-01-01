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

#include <string.h>

#include "text.h"
#include "utils.h"

enum
{
    TEXT_X1_COLUMN,
    TEXT_Y1_COLUMN,
    TEXT_X2_COLUMN,
    TEXT_Y2_COLUMN,
    TEXT_OFFSET_COLUMN,
    TEXT_OFFPTR_COLUMN,
    N_COLUMNS
};

typedef struct
{
    PopplerDocument *doc;

    GtkWidget *timer_label;
    GtkTextBuffer *buffer;
    GtkWidget *treeview;
    GtkListStore *model;
    GtkWidget *area_x1;
    GtkWidget *area_y1;
    GtkWidget *area_x2;
    GtkWidget *area_y2;

    /* Text attributes */
    GList *text_attrs;
    GtkWidget *font_name;
    GtkWidget *font_size;
    GtkWidget *is_underlined;
    GtkWidget *text_color;

    gint page;
    PopplerRectangle area;
} PgdTextDemo;

static void pgd_text_free(PgdTextDemo *demo)
{
    if (!demo) {
        return;
    }

    if (demo->doc) {
        g_object_unref(demo->doc);
        demo->doc = NULL;
    }

    if (demo->buffer) {
        g_object_unref(demo->buffer);
        demo->buffer = NULL;
    }

    if (demo->text_attrs) {
        poppler_page_free_text_attributes(demo->text_attrs);
        demo->text_attrs = NULL;
    }

    if (demo->model) {
        g_object_unref(demo->model);
        demo->model = NULL;
    }

    g_free(demo);
}

static void pgd_text_get_text(GtkWidget *button, PgdTextDemo *demo)
{
    PopplerPage *page;
    PopplerRectangle *recs = NULL;
    guint n_recs;
    gchar *text;
    GTimer *timer;
    gint i;

    page = poppler_document_get_page(demo->doc, demo->page);
    if (!page) {
        return;
    }

    gtk_list_store_clear(demo->model);
    if (demo->text_attrs) {
        poppler_page_free_text_attributes(demo->text_attrs);
    }
    demo->text_attrs = NULL;

    timer = g_timer_new();
    text = poppler_page_get_text_for_area(page, &demo->area);
    g_timer_stop(timer);

    if (text) {
        gchar *str;
        gdouble text_elapsed, layout_elapsed;

        text_elapsed = g_timer_elapsed(timer, NULL);

        g_timer_start(timer);
        poppler_page_get_text_layout_for_area(page, &demo->area, &recs, &n_recs);
        g_timer_stop(timer);

        layout_elapsed = g_timer_elapsed(timer, NULL);

        g_timer_start(timer);
        demo->text_attrs = poppler_page_get_text_attributes_for_area(page, &demo->area);
        g_timer_stop(timer);

        str = g_strdup_printf("<i>got %ld chars in %.4f seconds, %u layout units in %.4f seconds, text attrs in %.4f seconds</i>", g_utf8_strlen(text, -1), text_elapsed, n_recs, layout_elapsed, g_timer_elapsed(timer, NULL));
        gtk_label_set_markup(GTK_LABEL(demo->timer_label), str);
        g_free(str);
    } else {
        gtk_label_set_markup(GTK_LABEL(demo->timer_label), "<i>No text found</i>");
        n_recs = 0;
    }

    g_timer_destroy(timer);
    g_object_unref(page);

    if (text) {
        gtk_text_buffer_set_text(demo->buffer, text, strlen(text));
        g_free(text);
    }

    for (i = 0; i < n_recs; i++) {
        GtkTreeIter iter;
        gchar *x1, *y1, *x2, *y2;
        gchar *offset;

        x1 = g_strdup_printf("%.2f", recs[i].x1);
        y1 = g_strdup_printf("%.2f", recs[i].y1);
        x2 = g_strdup_printf("%.2f", recs[i].x2);
        y2 = g_strdup_printf("%.2f", recs[i].y2);

        offset = g_strdup_printf("%d", i);

        gtk_list_store_append(demo->model, &iter);
        gtk_list_store_set(demo->model, &iter, TEXT_X1_COLUMN, x1, TEXT_Y1_COLUMN, y1, TEXT_X2_COLUMN, x2, TEXT_Y2_COLUMN, y2, TEXT_OFFSET_COLUMN, offset, TEXT_OFFPTR_COLUMN, GINT_TO_POINTER(i), -1);

        g_free(x1);
        g_free(y1);
        g_free(x2);
        g_free(y2);
        g_free(offset);
    }

    g_free(recs);
}

static void pgd_text_set_text_attrs_for_offset(PgdTextDemo *demo, gint offset)
{
    GList *l;

    for (l = demo->text_attrs; l; l = g_list_next(l)) {
        PopplerTextAttributes *attrs = (PopplerTextAttributes *)l->data;

        if (offset >= attrs->start_index && offset <= attrs->end_index) {
            gchar *str;
            GdkPixbuf *pixbuf;

            gtk_label_set_text(GTK_LABEL(demo->font_name), attrs->font_name);

            str = g_strdup_printf("%.2f", attrs->font_size);
            gtk_label_set_text(GTK_LABEL(demo->font_size), str);
            g_free(str);

            gtk_label_set_text(GTK_LABEL(demo->is_underlined), attrs->is_underlined ? "Yes" : "No");

            pixbuf = pgd_pixbuf_new_for_color(&(attrs->color));
            gtk_image_set_from_pixbuf(GTK_IMAGE(demo->text_color), pixbuf);
            g_object_unref(pixbuf);
        }
    }
}

static void pgd_text_selection_changed(GtkTreeSelection *treeselection, PgdTextDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
        gpointer offset;
        GtkTextIter begin_iter, end_iter;

        gtk_tree_model_get(model, &iter, TEXT_OFFPTR_COLUMN, &offset, -1);

        gtk_text_buffer_get_iter_at_offset(demo->buffer, &begin_iter, GPOINTER_TO_INT(offset));
        end_iter = begin_iter;
        gtk_text_iter_forward_char(&end_iter);
        gtk_text_buffer_select_range(demo->buffer, &begin_iter, &end_iter);

        pgd_text_set_text_attrs_for_offset(demo, GPOINTER_TO_INT(offset));
    }
}

static void pgd_text_buffer_selection_changed(GtkTextBuffer *buffer, GParamSpec *pspec, GtkWidget *textview)
{
    gtk_widget_set_has_tooltip(textview, gtk_text_buffer_get_has_selection(buffer));
}

static gboolean pgd_text_view_query_tooltip(GtkTextView *textview, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip, PgdTextDemo *demo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(demo->treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        PopplerPage *page;
        gchar *x1, *y1, *x2, *y2;
        PopplerRectangle rect;
        gchar *text;

        gtk_tree_model_get(model, &iter, TEXT_X1_COLUMN, &x1, TEXT_Y1_COLUMN, &y1, TEXT_X2_COLUMN, &x2, TEXT_Y2_COLUMN, &y2, -1);

        rect.x1 = g_strtod(x1, NULL);
        rect.y1 = g_strtod(y1, NULL);
        rect.x2 = g_strtod(x2, NULL);
        rect.y2 = g_strtod(y2, NULL);

        g_free(x1);
        g_free(y1);
        g_free(x2);
        g_free(y2);

        page = poppler_document_get_page(demo->doc, demo->page);
        text = poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &rect);
        gtk_tooltip_set_text(tooltip, text);
        g_free(text);
        g_object_unref(page);
        return TRUE;
    } else {
        return FALSE;
    }
}

static void pgd_text_area_selector_setup(PgdTextDemo *demo)
{
    PopplerPage *page;
    gdouble width, height;

    page = poppler_document_get_page(demo->doc, demo->page);
    if (!page) {
        return;
    }

    poppler_page_get_size(page, &width, &height);

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(demo->area_x1), -10, width - 10);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(demo->area_y1), -10, height - 10);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(demo->area_x2), 0, width + 10);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(demo->area_y2), 0, height + 10);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(demo->area_x1), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(demo->area_y1), 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(demo->area_x2), width);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(demo->area_y2), height);

    g_object_unref(page);
}

static void pgd_text_area_selector_value_changed(GtkSpinButton *spinbutton, PgdTextDemo *demo)
{
    demo->area.x1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(demo->area_x1));
    demo->area.y1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(demo->area_y1));
    demo->area.x2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(demo->area_x2));
    demo->area.y2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(demo->area_y2));
}

static void pgd_text_page_selector_value_changed(GtkSpinButton *spinbutton, PgdTextDemo *demo)
{
    demo->page = (gint)gtk_spin_button_get_value(spinbutton) - 1;
}

GtkWidget *pgd_text_create_widget(PopplerDocument *document)
{
    PgdTextDemo *demo;
    GtkWidget *label;
    GtkWidget *vbox, *vbox2;
    GtkWidget *hbox, *page_selector, *area_hbox;
    GtkWidget *button;
    GtkWidget *swindow, *textview, *treeview;
    GtkTreeSelection *selection;
    GtkWidget *frame, *table;
    GtkWidget *hpaned;
    GtkCellRenderer *renderer;
    gchar *str;
    gint n_pages;
    gint row = 0;

    demo = g_new0(PgdTextDemo, 1);

    demo->doc = g_object_ref(document);

    n_pages = poppler_document_get_n_pages(document);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    label = gtk_label_new("Page:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);

    page_selector = gtk_spin_button_new_with_range(1, n_pages, 1);
    g_signal_connect(G_OBJECT(page_selector), "value-changed", G_CALLBACK(pgd_text_page_selector_value_changed), (gpointer)demo);
    gtk_box_pack_start(GTK_BOX(hbox), page_selector, FALSE, TRUE, 0);
    gtk_widget_show(page_selector);

    str = g_strdup_printf("of %d", n_pages);
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    g_free(str);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    area_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new("X1:");
    gtk_box_pack_start(GTK_BOX(area_hbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    demo->area_x1 = gtk_spin_button_new_with_range(0, 0, 0.01);
    g_signal_connect(demo->area_x1, "value-changed", G_CALLBACK(pgd_text_area_selector_value_changed), demo);
    gtk_box_pack_start(GTK_BOX(area_hbox), demo->area_x1, TRUE, TRUE, 0);
    gtk_widget_show(demo->area_x1);

    gtk_box_pack_start(GTK_BOX(hbox), area_hbox, FALSE, TRUE, 0);
    gtk_widget_show(area_hbox);

    area_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new("Y1:");
    gtk_box_pack_start(GTK_BOX(area_hbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    demo->area_y1 = gtk_spin_button_new_with_range(0, 0, 0.01);
    g_signal_connect(demo->area_y1, "value-changed", G_CALLBACK(pgd_text_area_selector_value_changed), demo);
    gtk_box_pack_start(GTK_BOX(area_hbox), demo->area_y1, TRUE, TRUE, 0);
    gtk_widget_show(demo->area_y1);

    gtk_box_pack_start(GTK_BOX(hbox), area_hbox, FALSE, TRUE, 0);
    gtk_widget_show(area_hbox);

    area_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new("X2:");
    gtk_box_pack_start(GTK_BOX(area_hbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    demo->area_x2 = gtk_spin_button_new_with_range(0, 0, 0.01);
    g_signal_connect(demo->area_x2, "value-changed", G_CALLBACK(pgd_text_area_selector_value_changed), demo);
    gtk_box_pack_start(GTK_BOX(area_hbox), demo->area_x2, TRUE, TRUE, 0);
    gtk_widget_show(demo->area_x2);

    gtk_box_pack_start(GTK_BOX(hbox), area_hbox, FALSE, TRUE, 0);
    gtk_widget_show(area_hbox);

    area_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new("Y2:");
    gtk_box_pack_start(GTK_BOX(area_hbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    demo->area_y2 = gtk_spin_button_new_with_range(0, 0, 0.01);
    g_signal_connect(demo->area_y2, "value-changed", G_CALLBACK(pgd_text_area_selector_value_changed), demo);
    gtk_box_pack_start(GTK_BOX(area_hbox), demo->area_y2, TRUE, TRUE, 0);
    gtk_widget_show(demo->area_y2);

    gtk_box_pack_start(GTK_BOX(hbox), area_hbox, FALSE, TRUE, 0);
    gtk_widget_show(area_hbox);

    pgd_text_area_selector_setup(demo);

    button = gtk_button_new_with_label("Get Text");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pgd_text_get_text), (gpointer)demo);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    demo->timer_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(demo->timer_label), "<i>No text found</i>");
    g_object_set(G_OBJECT(demo->timer_label), "xalign", 1.0, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), demo->timer_label, FALSE, TRUE, 0);
    gtk_widget_show(demo->timer_label);

    hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(hpaned), 300);

    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    demo->model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(demo->model));
    demo->treeview = treeview;

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), TEXT_X1_COLUMN, "X1", renderer, "text", TEXT_X1_COLUMN, NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), TEXT_Y1_COLUMN, "Y1", renderer, "text", TEXT_Y1_COLUMN, NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), TEXT_X2_COLUMN, "X2", renderer, "text", TEXT_X2_COLUMN, NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), TEXT_Y2_COLUMN, "Y2", renderer, "text", TEXT_Y2_COLUMN, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), TEXT_OFFSET_COLUMN, "Offset", renderer, "text", TEXT_OFFSET_COLUMN, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    g_signal_connect(selection, "changed", G_CALLBACK(pgd_text_selection_changed), (gpointer)demo);

    gtk_container_add(GTK_CONTAINER(swindow), treeview);
    gtk_widget_show(treeview);

    gtk_box_pack_start(GTK_BOX(vbox2), swindow, TRUE, TRUE, 0);
    gtk_widget_show(swindow);

    /* Text attributes */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Text Attributes</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_widget_show(label);

    table = gtk_grid_new();
    gtk_widget_set_margin_top(table, 5);
    gtk_widget_set_margin_bottom(table, 5);
    gtk_widget_set_margin_start(table, 12);
    gtk_widget_set_margin_end(table, 5);
    gtk_grid_set_column_spacing(GTK_GRID(table), 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);

    demo->font_name = gtk_label_new(NULL);
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Font Name:</b>", demo->font_name, &row);
    demo->font_size = gtk_label_new(NULL);
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Font Size:</b>", demo->font_size, &row);
    demo->is_underlined = gtk_label_new(NULL);
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Underlined:</b>", demo->is_underlined, &row);
    demo->text_color = gtk_image_new();
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Color:</b>", demo->text_color, &row);

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_widget_show(table);

    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 12);
    gtk_widget_show(frame);
    gtk_paned_add1(GTK_PANED(hpaned), vbox2);
    gtk_widget_show(vbox2);

    swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    demo->buffer = gtk_text_buffer_new(NULL);
    textview = gtk_text_view_new_with_buffer(demo->buffer);
    g_signal_connect(textview, "query-tooltip", G_CALLBACK(pgd_text_view_query_tooltip), demo);
    g_signal_connect(demo->buffer, "notify::has-selection", G_CALLBACK(pgd_text_buffer_selection_changed), textview);

    gtk_container_add(GTK_CONTAINER(swindow), textview);
    gtk_widget_show(textview);

    gtk_paned_add2(GTK_PANED(hpaned), swindow);
    gtk_widget_show(swindow);

    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
    gtk_widget_show(hpaned);

    g_object_weak_ref(G_OBJECT(vbox), (GWeakNotify)pgd_text_free, demo);

    return vbox;
}
