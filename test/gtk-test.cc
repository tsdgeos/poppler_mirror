#include <config.h>

#include <goo/gmem.h>
#include <splash/SplashTypes.h>
#include <splash/SplashBitmap.h>
#include "Object.h"
#include "SplashOutputDev.h"
#include "GfxState.h"

#include <gdk/gdk.h>

#include "PDFDoc.h"
#include "GlobalParams.h"
#include "ErrorCodes.h"
#include <poppler.h>
#include <poppler-private.h>
#include <gtk/gtk.h>
#include <cerrno>
#include <cmath>

static int requested_page = 0;
static gboolean cairo_output = FALSE;
static gboolean splash_output = FALSE;
#ifndef G_OS_WIN32
static gboolean args_are_fds = FALSE;
#endif
static const char **file_arguments = nullptr;
static const GOptionEntry options[] = { { "cairo", 'c', 0, G_OPTION_ARG_NONE, &cairo_output, "Cairo Output Device", nullptr },
                                        { "splash", 's', 0, G_OPTION_ARG_NONE, &splash_output, "Splash Output Device", nullptr },
                                        { "page", 'p', 0, G_OPTION_ARG_INT, &requested_page, "Page number", "PAGE" },
#ifndef G_OS_WIN32
                                        { "fd", 'f', 0, G_OPTION_ARG_NONE, &args_are_fds, "File descriptors", nullptr },
#endif
                                        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, static_cast<void *>(&file_arguments), nullptr, "PDF-FILES…" },
                                        {} };

static GList *view_list = nullptr;

//------------------------------------------------------------------------

#define xOutMaxRGBCube 6 // max size of RGB color cube

//------------------------------------------------------------------------
// GDKSplashOutputDev
//------------------------------------------------------------------------

class GDKSplashOutputDev : public SplashOutputDev
{
public:
    GDKSplashOutputDev(GdkScreen *screen, void (*redrawCbkA)(void *data), void *redrawCbkDataA, SplashColor sc);

    ~GDKSplashOutputDev() override;

    //----- initialization and control

    // End a page.
    void endPage() override;

    // Dump page contents to display.
    void dump() override;

    //----- update text state
    void updateFont(GfxState *state) override;

    //----- special access

    // Clear out the document (used when displaying an empty window).
    void clear();

    // Copy the rectangle (srcX, srcY, width, height) to (destX, destY)
    // in destDC.
    void redraw(int srcX, int srcY, cairo_t *cr, int destX, int destY, int width, int height);

private:
    int incrementalUpdate;
    void (*redrawCbk)(void *data);
    void *redrawCbkData;
};

typedef struct
{
    PopplerDocument *doc;
    GtkWidget *drawing_area;
    GtkWidget *spin_button;
    cairo_surface_t *surface;
    GDKSplashOutputDev *out;
} View;

//------------------------------------------------------------------------
// Constants and macros
//------------------------------------------------------------------------

#define xoutRound(x) ((int)((x) + 0.5))

//------------------------------------------------------------------------
// GDKSplashOutputDev
//------------------------------------------------------------------------

GDKSplashOutputDev::GDKSplashOutputDev(GdkScreen *screen, void (*redrawCbkA)(void *data), void *redrawCbkDataA, SplashColor sc) : SplashOutputDev(splashModeRGB8, 4, false, sc), incrementalUpdate(1)
{
    redrawCbk = redrawCbkA;
    redrawCbkData = redrawCbkDataA;
}

GDKSplashOutputDev::~GDKSplashOutputDev() = default;

void GDKSplashOutputDev::clear()
{
    startDoc(nullptr);
    startPage(0, nullptr, nullptr);
}

void GDKSplashOutputDev::endPage()
{
    SplashOutputDev::endPage();
    if (!incrementalUpdate) {
        (*redrawCbk)(redrawCbkData);
    }
}

void GDKSplashOutputDev::dump()
{
    if (incrementalUpdate && redrawCbk) {
        (*redrawCbk)(redrawCbkData);
    }
}

void GDKSplashOutputDev::updateFont(GfxState *state)
{
    SplashOutputDev::updateFont(state);
}

void GDKSplashOutputDev::redraw(int srcX, int srcY, cairo_t *cr, int destX, int destY, int width, int height)
{
    GdkPixbuf *pixbuf;
    int gdk_rowstride;

    gdk_rowstride = getBitmap()->getRowSize();
    pixbuf = gdk_pixbuf_new_from_data(getBitmap()->getDataPtr() + srcY * gdk_rowstride + srcX * 3, GDK_COLORSPACE_RGB, FALSE, 8, width, height, gdk_rowstride, nullptr, nullptr);

    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);

    g_object_unref(pixbuf);
}

static gboolean drawing_area_draw(GtkWidget *drawing_area, cairo_t *cr, View *view)
{
    GdkRectangle document;
    GdkRectangle clip;
    GdkRectangle draw;

    document.x = 0;
    document.y = 0;
    if (cairo_output) {
        document.width = cairo_image_surface_get_width(view->surface);
        document.height = cairo_image_surface_get_height(view->surface);
    } else {
        document.width = view->out->getBitmapWidth();
        document.height = view->out->getBitmapHeight();
    }

    if (!gdk_cairo_get_clip_rectangle(cr, &clip)) {
        return FALSE;
    }

    if (!gdk_rectangle_intersect(&document, &clip, &draw)) {
        return FALSE;
    }

    if (cairo_output) {
        cairo_set_source_surface(cr, view->surface, 0, 0);
        cairo_paint(cr);
    } else {
        view->out->redraw(draw.x, draw.y, cr, draw.x, draw.y, draw.width, draw.height);
    }

    return TRUE;
}

static void view_set_page(View *view, int page)
{
    int w, h;

    if (cairo_output) {
        cairo_t *cr;
        double width, height;
        PopplerPage *poppler_page;

        poppler_page = poppler_document_get_page(view->doc, page);
        poppler_page_get_size(poppler_page, &width, &height);
        w = (int)ceil(width);
        h = (int)ceil(height);

        if (view->surface) {
            cairo_surface_destroy(view->surface);
        }
        view->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

        cr = cairo_create(view->surface);
        poppler_page_render(poppler_page, cr);

        cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
        cairo_set_source_rgb(cr, 1., 1., 1.);
        cairo_paint(cr);

        cairo_destroy(cr);
        g_object_unref(poppler_page);
    } else {
        view->doc->doc->displayPage(view->out, page + 1, 72, 72, 0, false, true, true);
        w = view->out->getBitmapWidth();
        h = view->out->getBitmapHeight();
    }

    gtk_widget_set_size_request(view->drawing_area, w, h);
    gtk_widget_queue_draw(view->drawing_area);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(view->spin_button), page);
}

static void redraw_callback(void *data)
{
    View *view = (View *)data;

    gtk_widget_queue_draw(view->drawing_area);
}

static void view_free(View *view)
{
    if (G_UNLIKELY(!view)) {
        return;
    }

    g_object_unref(view->doc);
    delete view->out;
    cairo_surface_destroy(view->surface);
    g_slice_free(View, view);
}

static void destroy_window_callback(GtkWindow *window, View *view)
{
    view_list = g_list_remove(view_list, view);
    view_free(view);

    if (!view_list) {
        gtk_main_quit();
    }
}

static void page_changed_callback(GtkSpinButton *button, View *view)
{
    int page;

    page = gtk_spin_button_get_value_as_int(button);
    view_set_page(view, page);
}

static View *view_new(PopplerDocument *doc)
{
    View *view;
    GtkWidget *window;
    GtkWidget *sw;
    GtkWidget *vbox, *hbox;
    guint n_pages;
    PopplerPage *page;

    view = g_slice_new0(View);

    view->doc = doc;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(destroy_window_callback), view);

    page = poppler_document_get_page(doc, 0);
    if (page) {
        double width, height;

        poppler_page_get_size(page, &width, &height);
        gtk_window_set_default_size(GTK_WINDOW(window), (gint)width, (gint)height);
        g_object_unref(page);
    }

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    view->drawing_area = gtk_drawing_area_new();
    sw = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), view->drawing_area);
    gtk_widget_show(view->drawing_area);

    gtk_box_pack_end(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    gtk_widget_show(sw);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    n_pages = poppler_document_get_n_pages(doc);
    view->spin_button = gtk_spin_button_new_with_range(0, n_pages - 1, 1);
    g_signal_connect(view->spin_button, "value-changed", G_CALLBACK(page_changed_callback), view);

    gtk_box_pack_end(GTK_BOX(hbox), view->spin_button, FALSE, TRUE, 0);
    gtk_widget_show(view->spin_button);

    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    gtk_widget_show(window);

    if (!cairo_output) {
        SplashColor sc = { 255, 255, 255 };

        view->out = new GDKSplashOutputDev(gtk_widget_get_screen(window), redraw_callback, (void *)view, sc);
        view->out->startDoc(view->doc->doc);
    }

    g_signal_connect(view->drawing_area, "draw", G_CALLBACK(drawing_area_draw), view);

    return view;
}

int main(int argc, char *argv[])
{
    GOptionContext *ctx;

    if (argc == 1) {
        char *basename = g_path_get_basename(argv[0]);

        g_printerr("usage: %s PDF-FILES…\n", basename);
        g_free(basename);

        return -1;
    }

    ctx = g_option_context_new(nullptr);
    g_option_context_add_main_entries(ctx, options, "main");
    g_option_context_parse(ctx, &argc, &argv, nullptr);
    g_option_context_free(ctx);

    gtk_init(&argc, &argv);

    globalParams = std::make_unique<GlobalParams>();

    for (int i = 0; file_arguments[i]; i++) {
        View *view;
        GFile *file;
        PopplerDocument *doc = nullptr;
        GError *error = nullptr;
        const char *arg;

        arg = file_arguments[i];
#ifndef G_OS_WIN32
        if (args_are_fds) {
            char *end;
            gint64 v;

            errno = 0;
            end = nullptr;
            v = g_ascii_strtoll(arg, &end, 10);
            if (errno || end == arg || v == -1 || v < G_MININT || v > G_MAXINT) {
                g_set_error(&error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Failed to parse \"%s\" as file descriptor number", arg);
            } else {
                doc = poppler_document_new_from_fd(int(v), nullptr, &error);
            }
        } else
#endif /* !G_OS_WIN32 */
        {
            file = g_file_new_for_commandline_arg(arg);
            doc = poppler_document_new_from_gfile(file, nullptr, nullptr, &error);
            if (!doc) {
                gchar *uri;

                uri = g_file_get_uri(file);
                g_prefix_error(&error, "%s: ", uri);
                g_free(uri);
            }
            g_object_unref(file);
        }

        if (doc) {
            view = view_new(doc);
            view_list = g_list_prepend(view_list, view);
            view_set_page(view, CLAMP(requested_page, 0, poppler_document_get_n_pages(doc) - 1));
        } else {
            g_printerr("Error opening document: %s\n", error->message);
            g_error_free(error);
        }
    }

    if (view_list != nullptr) {
        gtk_main();
    }

    return 0;
}
