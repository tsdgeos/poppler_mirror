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
int main(int argc, char *argv[]) {
	GFile *infile;
	PopplerDocument *doc;
	PopplerPage *page;
	int npages, n;
	char *text;
	GError *err = NULL;

				/* open file */

	infile = g_file_new_for_path
		(TESTDATADIR "/unittestcases/WithActualText.pdf");
	if (! infile)
		exit(EXIT_FAILURE);

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
	g_object_unref(page);

	return EXIT_SUCCESS;
}

