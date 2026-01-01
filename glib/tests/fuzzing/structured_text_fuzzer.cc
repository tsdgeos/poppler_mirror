// Fuzzer for PDF structured text parser

#include <stdint.h>
#include <stdlib.h>
#include <poppler.h>

// Recursive function to exercise structure tree parsing
static void exercise_structure_tree(PopplerStructureElementIter *iter)
{
    if (!iter) {
        return;
    }

    do {
        PopplerStructureElement *element = poppler_structure_element_iter_get_element(iter);
        if (!element) {
            continue;
        }

        // Exercise all getter functions
        (void)poppler_structure_element_get_kind(element);

        // These return gchar* that should be freed to prevent memory leaks
        gchar *id = poppler_structure_element_get_id(element);
        g_free(id);

        gchar *title = poppler_structure_element_get_title(element);
        g_free(title);

        gchar *language = poppler_structure_element_get_language(element);
        g_free(language);

        gchar *abbreviation = poppler_structure_element_get_abbreviation(element);
        g_free(abbreviation);

        gchar *alt_text = poppler_structure_element_get_alt_text(element);
        g_free(alt_text);

        gchar *actual_text = poppler_structure_element_get_actual_text(element);
        g_free(actual_text);

        // Check if it's content and test text extraction
        if (poppler_structure_element_is_content(element)) {
            // Test non-recursive mode
            gchar *text = poppler_structure_element_get_text(element, POPPLER_STRUCTURE_GET_TEXT_NONE);
            g_free(text);

            // Test recursive mode
            text = poppler_structure_element_get_text(element, POPPLER_STRUCTURE_GET_TEXT_RECURSIVE);
            g_free(text);
        }

        // Recurse into children
        PopplerStructureElementIter *child_iter = poppler_structure_element_iter_get_child(iter);
        if (child_iter) {
            exercise_structure_tree(child_iter);
            poppler_structure_element_iter_free(child_iter);
        }

        g_object_unref(element);

    } while (poppler_structure_element_iter_next(iter));
}

// Fuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    GError *err = NULL;

    // Load document from fuzzer input
    GBytes *bytes = g_bytes_new_static(data, size);
    PopplerDocument *doc = poppler_document_new_from_bytes(bytes, NULL, &err);
    g_bytes_unref(bytes);

    if (doc == NULL) {
        if (err) {
            g_error_free(err);
        }
        return 0;
    }

    // Try to create iterator - if it fails, no structure tree exists
    PopplerStructureElementIter *root_iter = poppler_structure_element_iter_new(doc);
    if (root_iter) {
        exercise_structure_tree(root_iter);
        poppler_structure_element_iter_free(root_iter);
    }

    g_object_unref(doc);
    return 0;
}
