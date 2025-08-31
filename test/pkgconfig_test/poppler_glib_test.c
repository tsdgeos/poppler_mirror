#include <stdio.h>
#include <poppler-document.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s file.pdf\n", argv[0]);
        return 1;
    }

    GError *error = NULL;

    gchar *abs_path = g_canonicalize_filename(argv[1], NULL);

    gchar *uri = g_filename_to_uri(abs_path, NULL, &error);
    g_free(abs_path);

    if (!uri) {
        fprintf(stderr, "failed to convert path to uri: %s\n", error ? error->message : "unknown error");
        if (error) {
            g_error_free(error);
        }
        return 1;
    }

    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);

    if (!doc) {
        fprintf(stderr, "error opening file: %s\n", error ? error->message : "unknown error");
        if (error) {
            g_error_free(error);
        }
        return 1;
    }

    int n_pages = poppler_document_get_n_pages(doc);
    printf("%s has %d pages\n", argv[1], n_pages);

    g_object_unref(doc);
    return 0;
}
