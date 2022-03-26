/*
 * testing program for the get_text function
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <poppler.h>

/*
 * main
 */
int main(int argc, char *argv[])
{
    GFile *infile;
    PopplerDocument *doc;
    PopplerPage *page;
    PopplerRectangle *areas = NULL;
    guint n_glyph_areas, n_utf8_chars;
    int npages, n;
    char *text;
    GError *err = NULL;

    /* open file */

    infile = g_file_new_for_path(TESTDATADIR "/unittestcases/WithActualText.pdf");
    if (!infile) {
        exit(EXIT_FAILURE);
    }

    doc = poppler_document_new_from_gfile(infile, NULL, NULL, &err);
    if (doc == NULL) {
        g_printerr("error opening pdf file: %s\n", err->message);
        g_error_free(err);
        exit(EXIT_FAILURE);
    }

    /* pages */

    npages = poppler_document_get_n_pages(doc);
    if (npages < 1) {
        g_printerr("no page in document\n");
        exit(EXIT_FAILURE);
    }

    /* check text */

    n = 0;
    page = poppler_document_get_page(doc, n);
    text = poppler_page_get_text(page);
    g_print("%s\n", text);
    g_assert_cmpstr(text, ==, "The slow brown fox jumps over the black dog.");

    /* Cleanup vars for next test */
    g_clear_object(&page);
    g_clear_object(&doc);
    g_clear_object(&infile);
    g_clear_pointer(&text, g_free);

    /* Test for consistency between utf8 characters returned by poppler_page_get_text()
     * and glyph layout areas returned by poppler_page_get_text_layout(). Issue #1100 */
    g_print("Consistency test between poppler_page_get_text() and poppler_page_get_text_layout()\n");
    g_print("Issue #1100 \n");
    infile = g_file_new_for_path(TESTDATADIR "/unittestcases/searchAcrossLines.pdf");
    if (!infile) {
        exit(EXIT_FAILURE);
    }

    doc = poppler_document_new_from_gfile(infile, NULL, NULL, &err);
    if (doc == NULL) {
        g_printerr("error opening pdf file: %s\n", err->message);
        g_error_free(err);
        exit(EXIT_FAILURE);
    }

    page = poppler_document_get_page(doc, 0);
    if (page == NULL || !POPPLER_IS_PAGE(page)) {
        g_print("error opening pdf page\n");
        exit(EXIT_FAILURE);
    }

    text = poppler_page_get_text(page);
    n_utf8_chars = (guint)g_utf8_strlen(text, -1);
    poppler_page_get_text_layout(page, &areas, &n_glyph_areas);
    g_assert_cmpuint(n_glyph_areas, ==, n_utf8_chars);
    g_print("Test: OK ('layout glyph areas' match amount of 'utf8 characters')\n");

    /* Cleanup vars for next test */
    g_clear_object(&page);
    g_clear_object(&doc);
    g_clear_object(&infile);
    g_clear_pointer(&areas, g_free);
    g_clear_pointer(&text, g_free);

    return EXIT_SUCCESS;
}
