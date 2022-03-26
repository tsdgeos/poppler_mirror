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
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"

void pgd_table_add_property_with_custom_widget(GtkGrid *table, const gchar *markup, GtkWidget *widget, gint *row)
{
    GtkWidget *label;

    label = gtk_label_new(NULL);
    g_object_set(G_OBJECT(label), "xalign", 0.0, "yalign", 0.0, NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_grid_attach(GTK_GRID(table), label, 0, *row, 1, 1);
    gtk_widget_show(label);

    gtk_grid_attach(GTK_GRID(table), widget, 1, *row, 1, 1);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_widget_show(widget);

    *row += 1;
}

void pgd_table_add_property_with_value_widget(GtkGrid *table, const gchar *markup, GtkWidget **value_widget, const gchar *value, gint *row)
{
    GtkWidget *label;

    *value_widget = label = gtk_label_new(value);
    g_object_set(G_OBJECT(label), "xalign", 0.0, "selectable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    pgd_table_add_property_with_custom_widget(table, markup, label, row);
}

void pgd_table_add_property(GtkGrid *table, const gchar *markup, const gchar *value, gint *row)
{
    GtkWidget *label;

    pgd_table_add_property_with_value_widget(table, markup, &label, value, row);
}

GtkWidget *pgd_action_view_new(PopplerDocument *document)
{
    GtkWidget *frame, *label;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Action Properties</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_widget_show(label);

    g_object_set_data(G_OBJECT(frame), "document", document);

    return frame;
}

static void pgd_action_view_add_destination(GtkWidget *action_view, GtkGrid *table, PopplerDest *dest, gboolean remote, gint *row)
{
    PopplerDocument *document;
    GEnumValue *enum_value;
    gchar *str;

    pgd_table_add_property(table, "<b>Type:</b>", "Destination", row);

    enum_value = g_enum_get_value((GEnumClass *)g_type_class_ref(POPPLER_TYPE_DEST_TYPE), dest->type);
    pgd_table_add_property(table, "<b>Destination Type:</b>", enum_value->value_name, row);

    document = g_object_get_data(G_OBJECT(action_view), "document");

    if (dest->type != POPPLER_DEST_NAMED) {
        str = NULL;

        if (document && !remote) {
            PopplerPage *poppler_page;
            gchar *page_label;

            poppler_page = poppler_document_get_page(document, MAX(0, dest->page_num - 1));

            g_object_get(G_OBJECT(poppler_page), "label", &page_label, NULL);
            if (page_label) {
                str = g_strdup_printf("%d (%s)", dest->page_num, page_label);
                g_free(page_label);
            }
        }

        if (!str) {
            str = g_strdup_printf("%d", dest->page_num);
        }
        pgd_table_add_property(table, "<b>Page:</b>", str, row);
        g_free(str);

        str = g_strdup_printf("%.2f", dest->left);
        pgd_table_add_property(table, "<b>Left:</b>", str, row);
        g_free(str);

        str = g_strdup_printf("%.2f", dest->right);
        pgd_table_add_property(table, "<b>Right:</b>", str, row);
        g_free(str);

        str = g_strdup_printf("%.2f", dest->top);
        pgd_table_add_property(table, "<b>Top:</b>", str, row);
        g_free(str);

        str = g_strdup_printf("%.2f", dest->bottom);
        pgd_table_add_property(table, "<b>Bottom:</b>", str, row);
        g_free(str);

        str = g_strdup_printf("%.2f", dest->zoom);
        pgd_table_add_property(table, "<b>Zoom:</b>", str, row);
        g_free(str);
    } else {
        if (document && !remote) {
            PopplerDest *new_dest;

            new_dest = poppler_document_find_dest(document, dest->named_dest);
            if (new_dest) {
                GtkWidget *new_table;
                gint new_row = 0;

                new_table = gtk_grid_new();
                gtk_widget_set_margin_top(new_table, 5);
                gtk_widget_set_margin_bottom(new_table, 5);
#if GTK_CHECK_VERSION(3, 12, 0)
                gtk_widget_set_margin_start(new_table, 12);
                gtk_widget_set_margin_end(new_table, 5);
#else
                gtk_widget_set_margin_left(new_table, 12);
                gtk_widget_set_margin_right(new_table, 5);
#endif
                gtk_grid_set_column_spacing(GTK_GRID(new_table), 6);
                gtk_grid_set_row_spacing(GTK_GRID(new_table), 6);

                pgd_action_view_add_destination(action_view, GTK_GRID(new_table), new_dest, FALSE, &new_row);
                poppler_dest_free(new_dest);

                gtk_grid_attach(GTK_GRID(table), new_table, 0, *row, 1, 1);
                gtk_widget_show(new_table);

                *row += 1;
            }
        }
    }
}

static const gchar *get_movie_op(PopplerActionMovieOperation op)
{
    switch (op) {
    case POPPLER_ACTION_MOVIE_PLAY:
        return "Play";
    case POPPLER_ACTION_MOVIE_PAUSE:
        return "Pause";
    case POPPLER_ACTION_MOVIE_RESUME:
        return "Resume";
    case POPPLER_ACTION_MOVIE_STOP:
        return "Stop";
    }
    return NULL;
}

static void free_tmp_file(GFile *tmp_file)
{

    g_file_delete(tmp_file, NULL, NULL);
    g_object_unref(tmp_file);
}

static gboolean save_helper(const gchar *buf, gsize count, gpointer data, GError **error)
{
    gint fd = GPOINTER_TO_INT(data);

    return write(fd, buf, count) == count;
}

static void pgd_action_view_play_rendition(GtkWidget *button, PopplerMedia *media)
{
    GFile *file = NULL;
    gchar *uri;

    if (poppler_media_is_embedded(media)) {
        gint fd;
        gchar *tmp_filename = NULL;

        fd = g_file_open_tmp(NULL, &tmp_filename, NULL);
        if (fd != -1) {
            if (poppler_media_save_to_callback(media, save_helper, GINT_TO_POINTER(fd), NULL)) {
                file = g_file_new_for_path(tmp_filename);
                g_object_set_data_full(G_OBJECT(media), "tmp-file", g_object_ref(file), (GDestroyNotify)free_tmp_file);
            } else {
                g_free(tmp_filename);
            }
            close(fd);
        } else if (tmp_filename) {
            g_free(tmp_filename);
        }

    } else {
        const gchar *filename;

        filename = poppler_media_get_filename(media);
        if (g_path_is_absolute(filename)) {
            file = g_file_new_for_path(filename);
        } else if (g_strrstr(filename, "://")) {
            file = g_file_new_for_uri(filename);
        } else {
            gchar *cwd;
            gchar *path;

            // FIXME: relative to doc uri, not cwd
            cwd = g_get_current_dir();
            path = g_build_filename(cwd, filename, NULL);
            g_free(cwd);

            file = g_file_new_for_path(path);
            g_free(path);
        }
    }

    if (file) {
        uri = g_file_get_uri(file);
        g_object_unref(file);
        if (uri) {
#if GTK_CHECK_VERSION(3, 22, 0)
            GtkWidget *toplevel;

            toplevel = gtk_widget_get_toplevel(button);
            gtk_show_uri_on_window(gtk_widget_is_toplevel(toplevel) ? GTK_WINDOW(toplevel) : NULL, uri, GDK_CURRENT_TIME, NULL);
#else
            gtk_show_uri(gtk_widget_get_screen(button), uri, GDK_CURRENT_TIME, NULL);
#endif
            g_free(uri);
        }
    }
}

static void pgd_action_view_do_action_layer(GtkWidget *button, GList *state_list)
{
    GList *l, *m;

    for (l = state_list; l; l = g_list_next(l)) {
        PopplerActionLayer *action_layer = (PopplerActionLayer *)l->data;

        for (m = action_layer->layers; m; m = g_list_next(m)) {
            PopplerLayer *layer = (PopplerLayer *)m->data;

            switch (action_layer->action) {
            case POPPLER_ACTION_LAYER_ON:
                poppler_layer_show(layer);
                break;
            case POPPLER_ACTION_LAYER_OFF:
                poppler_layer_hide(layer);
                break;
            case POPPLER_ACTION_LAYER_TOGGLE:
                if (poppler_layer_is_visible(layer)) {
                    poppler_layer_hide(layer);
                } else {
                    poppler_layer_show(layer);
                }
                break;
            }
        }
    }
}

void pgd_action_view_set_action(GtkWidget *action_view, PopplerAction *action)
{
    GtkWidget *table;
    gint row = 0;

    table = gtk_bin_get_child(GTK_BIN(action_view));
    if (table) {
        gtk_container_remove(GTK_CONTAINER(action_view), table);
    }

    if (!action) {
        return;
    }

    table = gtk_grid_new();
    gtk_widget_set_margin_top(table, 5);
    gtk_widget_set_margin_bottom(table, 5);
#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_widget_set_margin_start(table, 12);
    gtk_widget_set_margin_end(table, 5);
#else
    gtk_widget_set_margin_left(table, 12);
    gtk_widget_set_margin_right(table, 5);
#endif
    gtk_grid_set_column_spacing(GTK_GRID(table), 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);

    pgd_table_add_property(GTK_GRID(table), "<b>Title:</b>", action->any.title, &row);

    switch (action->type) {
    case POPPLER_ACTION_UNKNOWN:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Unknown", &row);
        break;
    case POPPLER_ACTION_NONE:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "None", &row);
        break;
    case POPPLER_ACTION_GOTO_DEST:
        pgd_action_view_add_destination(action_view, GTK_GRID(table), action->goto_dest.dest, FALSE, &row);
        break;
    case POPPLER_ACTION_GOTO_REMOTE:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Remote Destination", &row);
        pgd_table_add_property(GTK_GRID(table), "<b>Filename:</b>", action->goto_remote.file_name, &row);
        pgd_action_view_add_destination(action_view, GTK_GRID(table), action->goto_remote.dest, TRUE, &row);
        break;
    case POPPLER_ACTION_LAUNCH:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Launch", &row);
        pgd_table_add_property(GTK_GRID(table), "<b>Filename:</b>", action->launch.file_name, &row);
        pgd_table_add_property(GTK_GRID(table), "<b>Params:</b>", action->launch.params, &row);
        break;
    case POPPLER_ACTION_URI:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "External URI", &row);
        pgd_table_add_property(GTK_GRID(table), "<b>URI</b>", action->uri.uri, &row);
        break;
    case POPPLER_ACTION_NAMED:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Named Action", &row);
        pgd_table_add_property(GTK_GRID(table), "<b>Name:</b>", action->named.named_dest, &row);
        break;
    case POPPLER_ACTION_MOVIE: {
        GtkWidget *movie_view = pgd_movie_view_new();

        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Movie", &row);
        pgd_table_add_property(GTK_GRID(table), "<b>Operation:</b>", get_movie_op(action->movie.operation), &row);
        pgd_movie_view_set_movie(movie_view, action->movie.movie);
        pgd_table_add_property_with_custom_widget(GTK_GRID(table), "<b>Movie:</b>", movie_view, &row);
    } break;
    case POPPLER_ACTION_RENDITION: {
        gchar *text;

        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "Rendition", &row);
        text = g_strdup_printf("%d", action->rendition.op);
        pgd_table_add_property(GTK_GRID(table), "<b>Operation:</b>", text, &row);
        g_free(text);
        if (action->rendition.media) {
            gboolean embedded = poppler_media_is_embedded(action->rendition.media);
            GtkWidget *button;

            pgd_table_add_property(GTK_GRID(table), "<b>Embedded:</b>", embedded ? "Yes" : "No", &row);
            if (embedded) {
                const gchar *mime_type = poppler_media_get_mime_type(action->rendition.media);
                pgd_table_add_property(GTK_GRID(table), "<b>Mime type:</b>", mime_type ? mime_type : "", &row);
            } else {
                pgd_table_add_property(GTK_GRID(table), "<b>Filename:</b>", poppler_media_get_filename(action->rendition.media), &row);
            }

            button = gtk_button_new_with_mnemonic("_Play");
            g_signal_connect(button, "clicked", G_CALLBACK(pgd_action_view_play_rendition), action->rendition.media);
            pgd_table_add_property_with_custom_widget(GTK_GRID(table), NULL, button, &row);
            gtk_widget_show(button);
        }
    } break;
    case POPPLER_ACTION_OCG_STATE: {
        GList *l;
        GtkWidget *button;

        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "OCGState", &row);

        for (l = action->ocg_state.state_list; l; l = g_list_next(l)) {
            PopplerActionLayer *action_layer = (PopplerActionLayer *)l->data;
            gchar *text = NULL;
            gint n_layers = g_list_length(action_layer->layers);

            switch (action_layer->action) {
            case POPPLER_ACTION_LAYER_ON:
                text = g_strdup_printf("%d layers On", n_layers);
                break;
            case POPPLER_ACTION_LAYER_OFF:
                text = g_strdup_printf("%d layers Off", n_layers);
                break;
            case POPPLER_ACTION_LAYER_TOGGLE:
                text = g_strdup_printf("%d layers Toggle", n_layers);
                break;
            }
            pgd_table_add_property(GTK_GRID(table), "<b>Action:</b>", text, &row);
            g_free(text);
        }

        button = gtk_button_new_with_label("Do action");
        g_signal_connect(button, "clicked", G_CALLBACK(pgd_action_view_do_action_layer), action->ocg_state.state_list);
        pgd_table_add_property_with_custom_widget(GTK_GRID(table), NULL, button, &row);
        gtk_widget_show(button);
    } break;
    case POPPLER_ACTION_JAVASCRIPT: {
        GtkTextBuffer *buffer;
        GtkWidget *textview;
        GtkWidget *swindow;

        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "JavaScript", &row);

        buffer = gtk_text_buffer_new(NULL);
        if (action->javascript.script) {
            gtk_text_buffer_set_text(buffer, action->javascript.script, -1);
        }

        textview = gtk_text_view_new_with_buffer(buffer);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
        g_object_unref(buffer);

        swindow = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(swindow), textview);
        gtk_widget_show(textview);

        pgd_table_add_property_with_custom_widget(GTK_GRID(table), NULL, swindow, &row);
        gtk_widget_show(swindow);
    } break;
    case POPPLER_ACTION_RESET_FORM:
        pgd_table_add_property(GTK_GRID(table), "<b>Type:</b>", "ResetForm", &row);
        break;
    default:
        g_assert_not_reached();
    }

    gtk_container_add(GTK_CONTAINER(action_view), table);
    gtk_widget_show(table);
}

gchar *pgd_format_date(time_t utime)
{
    GDateTime *dt = NULL;
    gchar *s = NULL;

    if (utime == 0) {
        return NULL;
    }
    dt = g_date_time_new_from_unix_local(utime);
    if (dt == NULL) {
        return NULL;
    }
    s = g_date_time_format(dt, "%c");
    g_date_time_unref(dt);

    return s;
}

GtkWidget *pgd_movie_view_new(void)
{
    GtkWidget *frame, *label;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Movie Properties</b>");
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_widget_show(label);

    return frame;
}

static void pgd_movie_view_play_movie(GtkWidget *button, PopplerMovie *movie)
{
    const gchar *filename;
    GFile *file;
    gchar *uri;

    filename = poppler_movie_get_filename(movie);
    if (g_path_is_absolute(filename)) {
        file = g_file_new_for_path(filename);
    } else if (g_strrstr(filename, "://")) {
        file = g_file_new_for_uri(filename);
    } else {
        gchar *cwd;
        gchar *path;

        // FIXME: relative to doc uri, not cwd
        cwd = g_get_current_dir();
        path = g_build_filename(cwd, filename, NULL);
        g_free(cwd);

        file = g_file_new_for_path(path);
        g_free(path);
    }

    uri = g_file_get_uri(file);
    g_object_unref(file);
    if (uri) {
#if GTK_CHECK_VERSION(3, 22, 0)
        GtkWidget *toplevel;

        toplevel = gtk_widget_get_toplevel(button);
        gtk_show_uri_on_window(gtk_widget_is_toplevel(toplevel) ? GTK_WINDOW(toplevel) : NULL, uri, GDK_CURRENT_TIME, NULL);
#else
        gtk_show_uri(gtk_widget_get_screen(button), uri, GDK_CURRENT_TIME, NULL);
#endif
        g_free(uri);
    }
}

void pgd_movie_view_set_movie(GtkWidget *movie_view, PopplerMovie *movie)
{
    GtkWidget *table;
    GtkWidget *button;
    GEnumValue *enum_value;
    gint width, height;
    gint row = 0;

    table = gtk_bin_get_child(GTK_BIN(movie_view));
    if (table) {
        gtk_container_remove(GTK_CONTAINER(movie_view), table);
    }

    if (!movie) {
        return;
    }

    table = gtk_grid_new();
    gtk_widget_set_margin_top(table, 5);
    gtk_widget_set_margin_bottom(table, 5);
#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_widget_set_margin_start(table, 12);
    gtk_widget_set_margin_end(table, 5);
#else
    gtk_widget_set_margin_left(table, 12);
    gtk_widget_set_margin_right(table, 5);
#endif
    gtk_grid_set_column_spacing(GTK_GRID(table), 6);
    gtk_grid_set_row_spacing(GTK_GRID(table), 6);

    pgd_table_add_property(GTK_GRID(table), "<b>Filename:</b>", poppler_movie_get_filename(movie), &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Need Poster:</b>", poppler_movie_need_poster(movie) ? "Yes" : "No", &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Show Controls:</b>", poppler_movie_show_controls(movie) ? "Yes" : "No", &row);
    enum_value = g_enum_get_value((GEnumClass *)g_type_class_ref(POPPLER_TYPE_MOVIE_PLAY_MODE), poppler_movie_get_play_mode(movie));
    pgd_table_add_property(GTK_GRID(table), "<b>Play Mode:</b>", enum_value->value_name, &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Synchronous Play:</b>", poppler_movie_is_synchronous(movie) ? "Yes" : "No", &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Volume:</b>", g_strdup_printf("%g", poppler_movie_get_volume(movie)), &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Rate:</b>", g_strdup_printf("%g", poppler_movie_get_rate(movie)), &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Start:</b>", g_strdup_printf("%g s", poppler_movie_get_start(movie) / 1e9), &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Duration:</b>", g_strdup_printf("%g s", poppler_movie_get_duration(movie) / 1e9), &row);
    pgd_table_add_property(GTK_GRID(table), "<b>Rotation Angle:</b>", g_strdup_printf("%u", poppler_movie_get_rotation_angle(movie)), &row);
    poppler_movie_get_aspect(movie, &width, &height);
    pgd_table_add_property(GTK_GRID(table), "<b>Aspect:</b>", g_strdup_printf("%dx%d", width, height), &row);

    button = gtk_button_new_with_mnemonic("_Play");
    g_signal_connect(button, "clicked", G_CALLBACK(pgd_movie_view_play_movie), movie);
    pgd_table_add_property_with_custom_widget(GTK_GRID(table), NULL, button, &row);
    gtk_widget_show(button);

    gtk_container_add(GTK_CONTAINER(movie_view), table);
    gtk_widget_show(table);
}

GdkPixbuf *pgd_pixbuf_new_for_color(PopplerColor *poppler_color)
{
    GdkPixbuf *pixbuf;
    gint num, x;
    guchar *pixels;

    if (!poppler_color) {
        return NULL;
    }

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 64, 16);

    pixels = gdk_pixbuf_get_pixels(pixbuf);
    num = gdk_pixbuf_get_width(pixbuf) * gdk_pixbuf_get_height(pixbuf);

    for (x = 0; x < num; x++) {
        pixels[0] = poppler_color->red;
        pixels[1] = poppler_color->green;
        pixels[2] = poppler_color->blue;
        pixels += 3;
    }

    return pixbuf;
}
