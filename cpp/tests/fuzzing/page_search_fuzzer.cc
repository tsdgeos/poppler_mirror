#include <cstdint>

#include <poppler-global.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

static void dummy_error_function(const std::string &, void *) { }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    poppler::set_debug_error_function(dummy_error_function, nullptr);

    poppler::document *doc = poppler::document::load_from_raw_data((const char *)data, size);
    if (!doc || doc->is_locked()) {
        delete doc;
        return 0;
    }

    poppler::page_renderer r;
    for (int i = 0; i < doc->pages(); i++) {
        poppler::page *p = doc->create_page(i);
        if (!p) {
            continue;
        }
        poppler::rectf rect = p->page_rect();
        poppler::ustring text = poppler::ustring::from_utf8((const char*)data, size);
        p->search(text, rect, poppler::page::search_from_top, poppler::case_insensitive, poppler::rotate_0);
        r.render_page(p);
        delete p;
    }

    delete doc;
    return 0;
}
