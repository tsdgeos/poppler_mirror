/*
 * Copyright (C) 2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
 * Copyright (C) 2014, Hans-Peter Deifel <hpdeifel@gmx.de>
 * Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
 * Copyright (C) 2018, 2020, Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
 * Copyright (C) 2018, 2020 Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2018, 2020, 2024 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2018, Zsombor Hollay-Horvath <hollay.horvath@gmail.com>
 * Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#ifndef POPPLER_PRIVATE_H
#define POPPLER_PRIVATE_H

#include "poppler-global.h"
#include "poppler-rectangle.h"
#include "poppler-page.h" // to use text_box::writing_mode_enum

#include "Error.h"
#include "CharTypes.h"

#include <cstdarg>

class GooString;
class PDFRectangle;

#define PSTR(str) (const_cast<char *>(str))

namespace poppler {

namespace detail {

extern debug_func user_debug_function;
extern void *debug_closure;
void error_function(ErrorCategory category, Goffset pos, const char *msg);

rectf pdfrectangle_to_rectf(const PDFRectangle &pdfrect);

ustring unicode_GooString_to_ustring(const GooString *str);
ustring unicode_to_ustring(const Unicode *u, int length);
std::unique_ptr<GooString> ustring_to_unicode_GooString(const ustring &str);

}

template<typename ConstIterator>
void delete_all(ConstIterator it, ConstIterator end)
{
    while (it != end) {
        delete *it++;
    }
}

template<typename Collection>
void delete_all(const Collection &c)
{
    delete_all(c.begin(), c.end());
}

class font_info;
struct text_box_font_info_data
{
    ~text_box_font_info_data();

    double font_size;
    std::vector<text_box::writing_mode_enum> wmodes;

    /*
     * a duplication of the font_info_cache created by the
     * poppler::font_iterator and owned by the poppler::page
     * object. Its lifetime might differ from that of text_box
     * object (think about collecting all text_box objects
     * from all pages), so we have to duplicate it into all
     * text_box instances.
     */
    std::vector<font_info> font_info_cache;

    /*
     * a std::vector from the glyph index in the owner
     * text_box to the font_info index in font_info_cache.
     * The "-1" means no corresponding fonts found in the
     * cache.
     */
    std::vector<int> glyph_to_cache_index;
};

class font_info;
struct text_box_data
{
    ~text_box_data();

    ustring text;
    rectf bbox;
    int rotation;
    std::vector<rectf> char_bboxes;
    bool has_space_after;

    std::unique_ptr<text_box_font_info_data> text_box_font;
};

}

#endif
