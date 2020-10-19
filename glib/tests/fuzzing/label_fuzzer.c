#include <stdint.h>
#include <poppler.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    GError *err = NULL;
    PopplerDocument *doc;
    PopplerPage *page;
    int npages;

    doc = poppler_document_new_from_data(data, size, NULL, &err);
    if (doc == NULL) {
        g_error_free(err);
        return 0;
    }

    npages = poppler_document_get_n_pages(doc);
    if (npages < 1) {
        return 0;
    }

    for (int n = 0; n < npages; n++) {
        page = poppler_document_get_page_by_label(doc, data);
        if (!page) {
            continue;
        }
        g_object_unref(page);
    }
    return 0;
}
