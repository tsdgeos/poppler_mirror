#include <stdint.h>
#include <poppler.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include "fuzzer_temp_file.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    GError *err = NULL;
    PopplerDocument *doc;
    PopplerPage *page;
    PopplerRectangle bb;
    gdouble width, height;
    gboolean hg;
    int npages, n;

    cairo_surface_t *surface;
    cairo_t *cr;

    doc = poppler_document_new_from_data(data, size, NULL, &err);
    if (doc == NULL) {
        g_error_free(err);
        return 0;
    }

    npages = poppler_document_get_n_pages(doc);
    if (npages < 1) {
        g_object_unref(doc);
        return 0;
    }

    char *tmpfile = fuzzer_get_tmpfile(data, size);
    surface = cairo_pdf_surface_create(tmpfile, 1.0, 1.0);

    for (n = 0; n < poppler_document_get_n_pages(doc); n++) {
        page = poppler_document_get_page(doc, n);
        if (!page) {
            continue;
        }
        poppler_page_get_size(page, &width, &height);
        cairo_pdf_surface_set_size(surface, width, height);
        hg = poppler_page_get_bounding_box(page, &bb);

        cr = cairo_create(surface);
        poppler_page_render_for_printing(page, cr);
        if (hg) {
            cairo_set_source_rgb(cr, 1.6, 1.6, 1.6);
            cairo_rectangle(cr, bb.x1, bb.y1, bb.x2 - bb.x1, bb.y2 - bb.y1);
            cairo_stroke(cr);
        }
        cairo_destroy(cr);
        cairo_surface_show_page(surface);

        g_object_unref(page);
    }

    g_object_unref(doc);
    cairo_surface_destroy(surface);
    fuzzer_release_tmpfile(tmpfile);
    return 0;
}
