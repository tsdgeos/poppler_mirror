#include <stdint.h>
#include <poppler.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    GError *err = NULL;
    PopplerDocument *doc;
    PopplerPage *page;
    PopplerAnnot *annot;
    PopplerRectangle bb;
    gboolean hg;
    int npages, n;

    doc = poppler_document_new_from_data(data, size, NULL, &err);
    if (doc == NULL) {
        g_error_free(err);
        return 0;
    }
    npages = poppler_document_get_n_pages(doc);
    if (npages < 1) {
        return 0;
    }

    for (n = 0; n < poppler_document_get_n_pages(doc); n++) {
        page = poppler_document_get_page(doc, n);
        if (!page) {
            continue;
        }
        hg = poppler_page_get_bounding_box(page, &bb);
        if (hg) {
            annot = poppler_annot_text_new(doc, &bb);
            poppler_page_add_annot(page, annot);
            poppler_annot_set_contents(annot, data);
            poppler_annot_markup_set_label(annot, data);
        }
        g_object_unref(page);
    }
    return 0;
}
