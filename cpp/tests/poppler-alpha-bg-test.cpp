/*
 * Test that render_page() supports the ignore_paper_color hint.
 *
 * Generate the test PDF with:
 *   Rscript -e 'pdf("test-alpha-bg.pdf", bg="#00008080"); grid::grid.newpage(); invisible(dev.off())'
 *
 * The PDF background is semi-transparent blue: R=0, G=0, B=128, A=128.
 * Rendering with ignore_paper_color should produce pixels of
 * (B=128, G=0, R=0, A=128) — the PDF color unchanged.
 * Rendering without the hint (default opaque white paper) should composite
 * to a fully-opaque light periwinkle (B~192, G~127, R~127, A=255).
 */

#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

static bool check_pixel(const poppler::image &img, int x, int y, unsigned char exp_b, unsigned char exp_g, unsigned char exp_r, unsigned char exp_a, int tolerance = 2)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(img.const_data() + y * img.bytes_per_row()) + x * 4;
    const bool ok = std::abs(p[0] - exp_b) <= tolerance && std::abs(p[1] - exp_g) <= tolerance && std::abs(p[2] - exp_r) <= tolerance && std::abs(p[3] - exp_a) <= tolerance;
    if (!ok) {
        std::cerr << "  FAIL at (" << x << "," << y << "): got BGRA=(" << static_cast<int>(p[0]) << "," << static_cast<int>(p[1]) << "," << static_cast<int>(p[2]) << "," << static_cast<int>(p[3]) << ")"
                  << " expected BGRA=(" << static_cast<int>(exp_b) << "," << static_cast<int>(exp_g) << "," << static_cast<int>(exp_r) << "," << static_cast<int>(exp_a) << ") tol=" << tolerance << "\n";
    }
    return ok;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " test-alpha-bg.pdf\n";
        return 1;
    }

    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(argv[1]));
    if (!doc) {
        std::cerr << "failed to open " << argv[1] << "\n";
        return 1;
    }

    std::unique_ptr<poppler::page> p(doc->create_page(0));
    if (!p) {
        std::cerr << "failed to get page 0\n";
        return 1;
    }

    const int cx = 100, cy = 100; // sample point well away from edges
    int failures = 0;

    // --- Test 1: ignore_paper_color hint ---
    // Paper contributes nothing; output should match the PDF content exactly.
    // PDF bg = #00008080 => B=128, G=0, R=0, A=128
    {
        poppler::page_renderer pr;
        pr.set_render_hint(poppler::page_renderer::ignore_paper_color);
        poppler::image img = pr.render_page(p.get(), 72.0, 72.0);
        if (!img.is_valid()) {
            std::cerr << "Test 1: render failed\n";
            ++failures;
        } else {
            std::cout << "Test 1 (ignore_paper_color hint):      ";
            if (check_pixel(img, cx, cy, 128, 0, 0, 128)) {
                std::cout << "PASS\n";
            } else {
                ++failures;
            }
        }
    }

    // --- Test 2: fully opaque white paper (the default) ---
    // Should composite to fully opaque. With as=128, ap=255:
    //   aout = 255, B = (128*128 + 127*255)/255 ~ 192, G=R ~ 127
    {
        poppler::page_renderer pr;
        poppler::image img = pr.render_page(p.get(), 72.0, 72.0);
        if (!img.is_valid()) {
            std::cerr << "Test 2: render failed\n";
            ++failures;
        } else {
            std::cout << "Test 2 (render on opaque white paper): ";
            if (check_pixel(img, cx, cy, 192, 127, 127, 255, 4)) {
                std::cout << "PASS\n";
            } else {
                ++failures;
            }
        }
    }

    std::cout << (failures == 0 ? "All tests passed.\n" : "Some tests FAILED.\n");
    return failures == 0 ? 0 : 1;
}
