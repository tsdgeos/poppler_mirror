/*
 * pdfdrawbb.c
 *
 * draw the bounding box of each page
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <poppler.h>
#include <cairo.h>
#include <cairo-pdf.h>

/*
 * add suffix to a pdf filename
 */
char *pdfaddsuffix(char *infile, char *suffix)
{
    char *basename;
    char *outfile;
    char *pos;

    basename = g_path_get_basename(infile);

    outfile = malloc(strlen(infile) + strlen(suffix) + 10);
    strcpy(outfile, basename);
    g_free(basename);

    pos = strrchr(outfile, '.');
    if (pos != NULL && (!strcmp(pos, ".pdf") || !strcmp(pos, ".PDF"))) {
        *pos = '\0';
    }

    strcat(outfile, "-");
    strcat(outfile, suffix);
    strcat(outfile, ".pdf");
    return outfile;
}

/*
 * main
 */
int main(int argc, char *argv[])
{
    int opt;
    gboolean usage = FALSE;
    char *infilename, *outfilename;

    GError *err = NULL;
    GFile *infile;
    PopplerDocument *doc;
    PopplerPage *page;
    int npages, n;
    PopplerRectangle bb;
    gboolean hg;

    gdouble width, height;
    cairo_surface_t *surface;
    cairo_t *cr;

    /* arguments */

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
        case 'h':
            usage = TRUE;
            break;
        }
    }

    if (!usage && argc - 1 < optind) {
        g_print("input file name missing\n");
        usage = TRUE;
    }
    if (usage) {
        g_print("usage:\n");
        g_print("\tpdfdrawbb");
        g_print("[-h] file.pdf\n");
        g_print("\t\t-h\t\tthis help\n");
        exit(EXIT_FAILURE);
    }
    infilename = argv[optind];
    if (!infilename) {
        exit(EXIT_FAILURE);
    }
    outfilename = pdfaddsuffix(argv[optind], "bb");

    /* open file */

    infile = g_file_new_for_path(infilename);
    if (infile == NULL) {
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
        g_print("no page in document\n");
        exit(EXIT_FAILURE);
    }

    /* copy to destination */

    surface = cairo_pdf_surface_create(outfilename, 1.0, 1.0);

    g_print("infile: %s\n", infilename);
    g_print("outfile: %s\n", outfilename);

    for (n = 0; n < npages; n++) {
        g_print("page %d:\n", n);
        page = poppler_document_get_page(doc, n);
        poppler_page_get_size(page, &width, &height);
        cairo_pdf_surface_set_size(surface, width, height);

        hg = poppler_page_get_bounding_box(page, &bb);
        if (hg) {
            g_print("bounding box %g,%g - %g,%g", bb.x1, bb.y1, bb.x2, bb.y2);
        }
        g_print("\n");

        cr = cairo_create(surface);
        poppler_page_render_for_printing(page, cr);
        if (hg) {
            cairo_set_source_rgb(cr, 0.6, 0.6, 1.0);
            cairo_rectangle(cr, bb.x1, bb.y1, bb.x2 - bb.x1, bb.y2 - bb.y1);
            cairo_stroke(cr);
        }
        cairo_destroy(cr);
        cairo_surface_show_page(surface);

        g_object_unref(page);
    }

    cairo_surface_destroy(surface);

    return EXIT_SUCCESS;
}
