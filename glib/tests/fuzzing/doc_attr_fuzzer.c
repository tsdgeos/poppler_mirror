#include <stdint.h>
#include <poppler.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    GError *err = NULL;
    PopplerDocument *doc;
    PopplerPage *page;
    int npages, n;

    doc = poppler_document_new_from_data(data, size, NULL, &err);
    if (doc == NULL) {
        g_error_free(err);
        return 0;
    }
    poppler_document_set_author(doc, data);
    poppler_document_set_creator(doc, data);
    poppler_document_set_keywords(doc, data);
    poppler_document_set_producer(doc, data);
    poppler_document_set_subject(doc, data);
    poppler_document_set_title(doc, data);
    return 0;
}
