#include <cstdint>
#include <poppler-document.h>
#include <poppler-global.h>
#include "fuzzer_init.h"
#include <poppler-page.h>
#include <poppler-page-renderer.h>
#include <GlobalParams.h>

#include "fuzzer_temp_file.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    initialize_poppler_data_dir();
    GlobalParamsIniter globalParamsIniter(nullptr);
    globalParams->setErrQuiet(true);

    char *tmpfile = fuzzer_get_tmpfile(data, size);
    std::string fname(tmpfile);
    poppler::document *doc = poppler::document::load_from_file(fname);
    if (!doc || doc->is_locked()) {
        delete doc;
        fuzzer_release_tmpfile(tmpfile);
        return 0;
    }

    poppler::page_renderer r;
    for (int i = 0; i < doc->pages(); i++) {
        poppler::page *p = doc->create_page(i);
        if (!p) {
            continue;
        }
        r.render_page(p);
        delete p;
    }

    delete doc;
    fuzzer_release_tmpfile(tmpfile);
    return 0;
}
