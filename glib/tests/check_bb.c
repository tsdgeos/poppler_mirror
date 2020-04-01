/*
 * testing program for the boundingbox function
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <poppler.h>

/*
 * compare floating-point coordinates
 */
int equal(double a, double b) {
	return fabs(a - b) < 0.01;
}

/*
 * main
 */
int main(int argc, char *argv[]) {
	GFile *infile;
	PopplerDocument *doc;
	PopplerPage *page;
	int npages, n;
	gboolean hg;
	PopplerRectangle bb, correct;
	GError *err = NULL;
	int argx;

				/* open file */

	g_print("file: %s\n", argv[1]);
	infile = g_file_new_for_path(argv[1]);
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

				/* check the bounding box */

	argx = 2;
	for (n = 0; n < poppler_document_get_n_pages(doc); n++) {
		g_print("    page: %d\n", n + 1);

		page = poppler_document_get_page(doc, n);
		hg = poppler_page_get_bounding_box(page, &bb);
		if (! hg) {
			g_printerr("no graphics in page\n");
			exit(EXIT_FAILURE);
		}
		g_print("        bounding box: %g,%g - %g,%g\n",
			bb.x1, bb.y1, bb.x2, bb.y2);

		if (argc - argx < 4) {
			g_print("not enough arguments\n");
			exit(EXIT_FAILURE);
		}
		correct.x1 = atof(argv[argx++]);
		correct.y1 = atof(argv[argx++]);
		correct.x2 = atof(argv[argx++]);
		correct.y2 = atof(argv[argx++]);
		g_print("        correct:      %g,%g - %g,%g\n",
			correct.x1, correct.y1, correct.x2, correct.y2);
		if (! equal(bb.x1, correct.x1) ||
		    ! equal(bb.x2, correct.x2) ||
		    ! equal(bb.y1, correct.y1) ||
		    ! equal(bb.x2, correct.x2)) {
			g_print("bounding box differs from expected\n");
			exit(EXIT_FAILURE);
		}

		g_object_unref(page);
	}

	return EXIT_SUCCESS;
}

