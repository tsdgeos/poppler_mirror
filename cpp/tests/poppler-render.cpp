/*
 * Copyright (C) 2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2017, 2019, 2026, Albert Astals Cid <aacid@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

#include "parseargs.h"

bool show_help = false;
bool show_formats = false;
char out_filename[4096];
int doc_page = 0;
bool read_to_memory = false;

static const ArgDesc the_args[] = { { .arg = "-f", .kind = argFlag, .val = &show_formats, .size = 0, .usage = "show supported output image formats" },
                                    { .arg = "--page", .kind = argInt, .val = &doc_page, .size = 0, .usage = "select page to render" },
                                    { .arg = "-o", .kind = argString, .val = &out_filename, .size = sizeof(out_filename), .usage = "output filename for the resulting PNG image" },
                                    { .arg = "-m", .kind = argFlag, .val = &read_to_memory, .size = 0, .usage = "reds file into memory first" },
                                    { .arg = "-h", .kind = argFlag, .val = &show_help, .size = 0, .usage = "print usage information" },
                                    { .arg = "--help", .kind = argFlag, .val = &show_help, .size = 0, .usage = "print usage information" },
                                    { .arg = nullptr, .kind = argFlag, .val = nullptr, .size = 0, .usage = nullptr } };

static void error(const std::string &msg)
{
    std::cerr << "Error: " << msg << std::endl;
    std::cerr << "Exiting..." << std::endl;
    exit(1);
}

int main(int argc, char *argv[])
{
    if (!parseArgs(the_args, &argc, argv) || (argc < 2 && !show_formats) || show_help) {
        printUsage(argv[0], "DOCUMENT", the_args);
        exit(1);
    }

    if (show_formats) {
        const std::vector<std::string> formats = poppler::image::supported_image_formats();
        std::cout << "Supported image formats:" << std::endl;
        for (const std::string &format : formats) {
            std::cout << "  " << format << std::endl;
        }
        exit(0);
    }

    if (!out_filename[0]) {
        error("missing output filename (-o)");
    }

    if (!poppler::page_renderer::can_render()) {
        error("renderer compiled without Splash support");
    }

    const std::string file_name(argv[1]);

    std::unique_ptr<poppler::document> doc;
    if (read_to_memory) {
        std::ifstream file(file_name, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        file.read(buffer.data(), size);
        doc = std::unique_ptr<poppler::document>(poppler::document::load_from_data(&buffer));
    } else {
        doc = std::unique_ptr<poppler::document>(poppler::document::load_from_file(file_name));
    }
    if (!doc) {
        error("loading error");
    }
    if (doc->is_locked()) {
        error("encrypted document");
    }

    if (doc_page < 0 || doc_page >= doc->pages()) {
        error("specified page number out of page count");
    }
    std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
    if (!p) {
        error("NULL page");
    }

    poppler::page_renderer pr;
    pr.set_render_hint(poppler::page_renderer::antialiasing, true);
    pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    poppler::image img = pr.render_page(p.get());
    if (!img.is_valid()) {
        error("rendering failed");
    }

    if (!img.save(out_filename, "png")) {
        error("saving to file failed");
    }

    return 0;
}
