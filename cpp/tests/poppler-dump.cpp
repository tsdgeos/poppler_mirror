/*
 * Copyright (C) 2009-2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2017-2019, 2022, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2017, Jason Alan Palmer <jalanpalmer@gmail.com>
 * Copyright (C) 2018, 2020, Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
 * Copyright (C) 2019, Masamichi Hosoda <trueroad@trueroad.jp>
 * Copyright (C) 2020, Jiri Jakes <freedesktop@jirijakes.eu>
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

#include <goo/glibc.h>
#include <poppler-destination.h>
#include <poppler-document.h>
#include <poppler-embedded-file.h>
#include <poppler-font.h>
#include <poppler-page.h>
#include <poppler-toc.h>
#include <poppler-version.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "parseargs.h"

#include "config.h"

static const int out_width = 30;

bool show_all = false;
bool show_info = false;
bool show_perm = false;
bool show_metadata = false;
bool show_toc = false;
bool show_fonts = false;
bool show_embedded_files = false;
bool show_pages = false;
bool show_destinations = false;
bool show_help = false;
bool show_version = false;
char show_text[32];
bool show_text_list = false;
bool show_text_list_with_font = false;
poppler::page::text_layout_enum show_text_layout = poppler::page::physical_layout;

static const ArgDesc the_args[] = { { "--show-all", argFlag, &show_all, 0, "show all the available information" },
                                    { "--show-info", argFlag, &show_info, 0, "show general document information" },
                                    { "--show-perm", argFlag, &show_perm, 0, "show document permissions" },
                                    { "--show-metadata", argFlag, &show_metadata, 0, "show document metadata" },
                                    { "--show-toc", argFlag, &show_toc, 0, "show the TOC" },
                                    { "--show-fonts", argFlag, &show_fonts, 0, "show the document fonts" },
                                    { "--show-embedded-files", argFlag, &show_embedded_files, 0, "show the document-level embedded files" },
                                    { "--show-pages", argFlag, &show_pages, 0, "show pages information" },
                                    { "--show-destinations", argFlag, &show_destinations, 0, "show named destinations" },
                                    { "--show-text", argString, &show_text, sizeof(show_text), "show text (physical|raw|none) extracted from all pages" },
                                    { "--show-text-list", argFlag, &show_text_list, 0, "show text list (experimental)" },
                                    { "--show-text-list-with-font", argFlag, &show_text_list_with_font, 0, "show text list with font info (experimental)" },
                                    { "-h", argFlag, &show_help, 0, "print usage information" },
                                    { "--help", argFlag, &show_help, 0, "print usage information" },
                                    { "--version", argFlag, &show_version, 0, "print poppler version" },
                                    { nullptr, argFlag, nullptr, 0, nullptr } };

static void error(const std::string &msg)
{
    std::cerr << "Error: " << msg << std::endl;
    std::cerr << "Exiting..." << std::endl;
    exit(1);
}

static std::ostream &operator<<(std::ostream &stream, const poppler::ustring &str)
{
    const poppler::byte_array ba = str.to_utf8();
    for (const char c : ba) {
        stream << c;
    }
    return stream;
}

static std::string out_date(std::time_t date)
{
    if (date != std::time_t(-1)) {
        struct tm time;
        gmtime_r(&date, &time);
        struct tm *t = &time;
        char buf[32];
        strftime(buf, sizeof(buf) - 1, "%d/%m/%Y %H:%M:%S", t);
        return std::string(buf);
    }
    return std::string("n/a");
}

static std::string out_size(int size)
{
    if (size >= 0) {
        std::ostringstream ss;
        ss << size;
        return ss.str();
    }
    return std::string("n/a");
}

static char charToHex(int x)
{
    return x < 10 ? x + '0' : x - 10 + 'a';
}

static std::string out_hex_string(const poppler::byte_array &str)
{
    std::string ret(str.size() * 2, '\0');
    const char *str_p = &str[0];
    for (unsigned int i = 0; i < str.size(); ++i, ++str_p) {
        ret[i * 2] = charToHex((*str_p & 0xf0) >> 4);
        ret[i * 2 + 1] = charToHex(*str_p & 0xf);
    }
    return ret;
}

static std::string out_page_orientation(poppler::page::orientation_enum o)
{
    switch (o) {
    case poppler::page::landscape:
        return "landscape (90)";
    case poppler::page::portrait:
        return "portrait (0)";
    case poppler::page::seascape:
        return "seascape (270)";
    case poppler::page::upside_down:
        return "upside_downs (180)";
    };
    return "<unknown orientation>";
}

static std::string out_font_info_type(poppler::font_info::type_enum t)
{
#define OUT_FONT_TYPE(thetype)                                                                                                                                                                                                                 \
    case poppler::font_info::thetype:                                                                                                                                                                                                          \
        return #thetype
    switch (t) {
        OUT_FONT_TYPE(unknown);
        OUT_FONT_TYPE(type1);
        OUT_FONT_TYPE(type1c);
        OUT_FONT_TYPE(type1c_ot);
        OUT_FONT_TYPE(type3);
        OUT_FONT_TYPE(truetype);
        OUT_FONT_TYPE(truetype_ot);
        OUT_FONT_TYPE(cid_type0);
        OUT_FONT_TYPE(cid_type0c);
        OUT_FONT_TYPE(cid_type0c_ot);
        OUT_FONT_TYPE(cid_truetype);
        OUT_FONT_TYPE(cid_truetype_ot);
    }
    return "<unknown font type>";
#undef OUT_FONT_TYPE
}

static void print_info(poppler::document *doc)
{
    std::cout << "Document information:" << std::endl;
    int major = 0, minor = 0;
    doc->get_pdf_version(&major, &minor);
    std::cout << std::setw(out_width) << "PDF version"
              << ": " << major << "." << minor << std::endl;
    std::string permanent_id, update_id;
    if (doc->get_pdf_id(&permanent_id, &update_id)) {
        std::cout << std::setw(out_width) << "PDF IDs"
                  << ": P: " << permanent_id << " - U: " << update_id << std::endl;
    } else {
        std::cout << std::setw(out_width) << "PDF IDs"
                  << ": <none>" << std::endl;
    }
    const std::vector<std::string> keys = doc->info_keys();
    std::vector<std::string>::const_iterator key_it = keys.begin(), key_end = keys.end();
    for (; key_it != key_end; ++key_it) {
        std::cout << std::setw(out_width) << *key_it << ": " << doc->info_key(*key_it) << std::endl;
    }
    std::cout << std::setw(out_width) << "Date (creation)"
              << ": " << out_date(doc->info_date_t("CreationDate")) << std::endl;
    std::cout << std::setw(out_width) << "Date (modification)"
              << ": " << out_date(doc->info_date_t("ModDate")) << std::endl;
    std::cout << std::setw(out_width) << "Number of pages"
              << ": " << doc->pages() << std::endl;
    std::cout << std::setw(out_width) << "Linearized"
              << ": " << doc->is_linearized() << std::endl;
    std::cout << std::setw(out_width) << "Encrypted"
              << ": " << doc->is_encrypted() << std::endl;
    std::cout << std::endl;
}

static void print_perm(poppler::document *doc)
{
    std::cout << "Document permissions:" << std::endl;
#define OUT_PERM(theperm) std::cout << std::setw(out_width) << #theperm << ": " << doc->has_permission(poppler::perm_##theperm) << std::endl
    OUT_PERM(print);
    OUT_PERM(change);
    OUT_PERM(copy);
    OUT_PERM(add_notes);
    OUT_PERM(fill_forms);
    OUT_PERM(accessibility);
    OUT_PERM(assemble);
    OUT_PERM(print_high_resolution);
    std::cout << std::endl;
#undef OUT_PERM
}

static void print_metadata(poppler::document *doc)
{
    std::cout << std::setw(out_width) << "Metadata"
              << ":" << std::endl
              << doc->metadata() << std::endl;
    std::cout << std::endl;
}

static void print_toc_item(poppler::toc_item *item, int indent)
{
    std::cout << std::setw(indent * 2) << " "
              << "+ " << item->title() << " (" << item->is_open() << ")" << std::endl;
    poppler::toc_item::iterator it = item->children_begin(), it_end = item->children_end();
    for (; it != it_end; ++it) {
        print_toc_item(*it, indent + 1);
    }
}

static void print_toc(poppler::toc *doctoc)
{
    std::cout << "Document TOC:" << std::endl;
    if (doctoc) {
        print_toc_item(doctoc->root(), 0);
    } else {
        std::cout << "<no TOC>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_fonts(poppler::document *doc)
{
    std::cout << "Document fonts:" << std::endl;
    std::vector<poppler::font_info> fl = doc->fonts();
    if (!fl.empty()) {
        std::vector<poppler::font_info>::const_iterator it = fl.begin(), it_end = fl.end();
        const std::ios_base::fmtflags f = std::cout.flags();
        std::left(std::cout);
        for (; it != it_end; ++it) {
            std::cout << " " << std::setw(out_width + 10) << it->name() << " " << std::setw(15) << out_font_info_type(it->type()) << " " << std::setw(5) << it->is_embedded() << " " << std::setw(5) << it->is_subset() << " " << it->file()
                      << std::endl;
        }
        std::cout.flags(f);
    } else {
        std::cout << "<no fonts>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_embedded_files(poppler::document *doc)
{
    std::cout << "Document embedded files:" << std::endl;
    std::vector<poppler::embedded_file *> ef = doc->embedded_files();
    if (!ef.empty()) {
        std::vector<poppler::embedded_file *>::const_iterator it = ef.begin(), it_end = ef.end();
        const std::ios_base::fmtflags flags = std::cout.flags();
        std::left(std::cout);
        for (; it != it_end; ++it) {
            poppler::embedded_file *f = *it;
            std::cout << " " << std::setw(out_width + 10) << f->name() << " " << std::setw(10) << out_size(f->size()) << " " << std::setw(20) << out_date(f->creation_date_t()) << " " << std::setw(20) << out_date(f->modification_date_t())
                      << std::endl
                      << "     ";
            if (f->description().empty()) {
                std::cout << "<no description>";
            } else {
                std::cout << f->description();
            }
            std::cout << std::endl
                      << "     " << std::setw(35) << (f->checksum().empty() ? std::string("<no checksum>") : out_hex_string(f->checksum())) << " " << (f->mime_type().empty() ? std::string("<no mime type>") : f->mime_type()) << std::endl;
        }
        std::cout.flags(flags);
    } else {
        std::cout << "<no embedded files>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_page(poppler::page *p)
{
    if (p) {
        std::cout << std::setw(out_width) << "Rect"
                  << ": " << p->page_rect() << std::endl;
        std::cout << std::setw(out_width) << "Label"
                  << ": " << p->label() << std::endl;
        std::cout << std::setw(out_width) << "Duration"
                  << ": " << p->duration() << std::endl;
        std::cout << std::setw(out_width) << "Orientation"
                  << ": " << out_page_orientation(p->orientation()) << std::endl;
    } else {
        std::cout << std::setw(out_width) << "Broken Page. Could not be parsed" << std::endl;
    }
    std::cout << std::endl;
}

static void print_destination(const poppler::destination *d)
{
    if (d) {
        std::cout << std::setw(out_width) << "Type"
                  << ": ";
        switch (d->type()) {
        case poppler::destination::unknown:
            std::cout << "unknown" << std::endl;
            break;
        case poppler::destination::xyz:
            std::cout << "xyz" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Left"
                      << ": " << d->left() << std::endl
                      << std::setw(out_width) << "Top"
                      << ": " << d->top() << std::endl
                      << std::setw(out_width) << "Zoom"
                      << ": " << d->zoom() << std::endl;
            break;
        case poppler::destination::fit:
            std::cout << "fit" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl;
            break;
        case poppler::destination::fit_h:
            std::cout << "fit_h" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Top"
                      << ": " << d->top() << std::endl;
            break;
        case poppler::destination::fit_v:
            std::cout << "fit_v" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Left"
                      << ": " << d->left() << std::endl;
            break;
        case poppler::destination::fit_r:
            std::cout << "fit_r" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Left"
                      << ": " << d->left() << std::endl
                      << std::setw(out_width) << "Bottom"
                      << ": " << d->bottom() << std::endl
                      << std::setw(out_width) << "Right"
                      << ": " << d->right() << std::endl
                      << std::setw(out_width) << "Top"
                      << ": " << d->top() << std::endl;
            break;
        case poppler::destination::fit_b:
            std::cout << "fit_b" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl;
            break;
        case poppler::destination::fit_b_h:
            std::cout << "fit_b_h" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Top"
                      << ": " << d->top() << std::endl;
            break;
        case poppler::destination::fit_b_v:
            std::cout << "fit_b_v" << std::endl
                      << std::setw(out_width) << "Page"
                      << ": " << d->page_number() << std::endl
                      << std::setw(out_width) << "Left"
                      << ": " << d->left() << std::endl;
            break;
        default:
            std::cout << "error" << std::endl;
            break;
        }
    }
    std::cout << std::endl;
}

static void print_page_text(poppler::page *p)
{
    if (p) {
        std::cout << p->text(poppler::rectf(), show_text_layout) << std::endl;
    } else {
        std::cout << std::setw(out_width) << "Broken Page. Could not be parsed" << std::endl;
    }
    std::cout << std::endl;
}

static void print_page_text_list(poppler::page *p, int opt_flag = 0)
{
    if (!p) {
        std::cout << std::setw(out_width) << "Broken Page. Could not be parsed" << std::endl;
        std::cout << std::endl;
        return;
    }
    auto text_list = p->text_list(opt_flag);

    std::cout << "---" << std::endl;
    for (const poppler::text_box &text : text_list) {
        poppler::rectf bbox = text.bbox();
        poppler::ustring ustr = text.text();
        int wmode = text.get_wmode();
        double font_size = text.get_font_size();
        std::string font_name = text.get_font_name();
        std::cout << "[" << ustr << "] @ ";
        std::cout << "( x=" << bbox.x() << " y=" << bbox.y() << " w=" << bbox.width() << " h=" << bbox.height() << " )";
        if (text.has_font_info()) {
            std::cout << "( fontname=" << font_name << " fontsize=" << font_size << " wmode=" << wmode << " )";
        }
        std::cout << std::endl;
    }
    std::cout << "---" << std::endl;
}

int main(int argc, char *argv[])
{
    if (!parseArgs(the_args, &argc, argv) || argc < 2 || show_help) {
        printUsage(argv[0], "DOCUMENT", the_args);
        exit(1);
    }

    if (show_text[0]) {
        if (!memcmp(show_text, "physical", 9)) {
            show_text_layout = poppler::page::physical_layout;
        } else if (!memcmp(show_text, "raw", 4)) {
            show_text_layout = poppler::page::raw_order_layout;
        } else if (!memcmp(show_text, "none", 5)) {
            show_text_layout = poppler::page::non_raw_non_physical_layout;
        } else {
            error(std::string("unrecognized text mode: '") + show_text + "'");
        }
    }

    std::string file_name(argv[1]);

    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(file_name));
    if (!doc) {
        error("loading error");
    }
    if (doc->is_locked()) {
        error("encrypted document");
    }

    std::cout.setf(std::ios_base::boolalpha);

    if (show_all) {
        show_info = true;
        show_perm = true;
        show_metadata = true;
        show_toc = true;
        show_fonts = true;
        show_embedded_files = true;
        show_pages = true;
    }

    if (show_version) {
        std::cout << std::setw(out_width) << "Compiled"
                  << ": poppler-cpp " << POPPLER_VERSION << std::endl
                  << std::setw(out_width) << "Running"
                  << ": poppler-cpp " << poppler::version_string() << std::endl;
    }
    if (show_info) {
        print_info(doc.get());
    }
    if (show_perm) {
        print_perm(doc.get());
    }
    if (show_metadata) {
        print_metadata(doc.get());
    }
    if (show_toc) {
        std::unique_ptr<poppler::toc> doctoc(doc->create_toc());
        print_toc(doctoc.get());
    }
    if (show_fonts) {
        print_fonts(doc.get());
    }
    if (show_embedded_files) {
        print_embedded_files(doc.get());
    }
    if (show_pages) {
        const int pages = doc->pages();
        for (int i = 0; i < pages; ++i) {
            std::cout << "Page " << (i + 1) << "/" << pages << ":" << std::endl;
            std::unique_ptr<poppler::page> p(doc->create_page(i));
            print_page(p.get());
        }
    }
    if (show_destinations) {
        auto map = doc->create_destination_map();
        for (const auto &pair : map) {
            std::string s = pair.first;
            for (auto &c : s) {
                if (c < 0x20 || c > 0x7e) {
                    c = '.';
                }
            }
            std::cout << "Named destination \"" << s << "\":" << std::endl;
            print_destination(&pair.second);
        }
    }
    if (show_text[0]) {
        const int pages = doc->pages();
        for (int i = 0; i < pages; ++i) {
            std::cout << "Page " << (i + 1) << "/" << pages << ":" << std::endl;
            std::unique_ptr<poppler::page> p(doc->create_page(i));
            print_page_text(p.get());
        }
    }
    if (show_text_list || show_text_list_with_font) {
        const int pages = doc->pages();
        for (int i = 0; i < pages; ++i) {
            std::cout << "Page " << (i + 1) << "/" << pages << ":" << std::endl;
            std::unique_ptr<poppler::page> p(doc->create_page(i));
            if (show_text_list_with_font) {
                print_page_text_list(p.get(), poppler::page::text_list_include_font);
            } else {
                print_page_text_list(p.get(), 0);
            }
        }
    }

    return 0;
}
