/*
 * Copyright (C) 2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2015, Tamas Szekeres <szekerest@gmail.com>
 * Copyright (C) 2018, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2019, Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2020, Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
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

/**
 \file poppler-font.h
 */
#include "poppler-font.h"

#include "poppler-font-private.h"

#include "poppler-document-private.h"

#include "FontInfo.h"

#include <algorithm>

using namespace poppler;

/**
 \class poppler::font_info poppler-font.h "poppler/cpp/poppler-font.h"

 The information about a font used in a PDF %document.
 */

/**
 \enum poppler::font_info::type_enum

 The various types of fonts available in a PDF %document.
*/

/**
 Constructs an invalid font information.
 */
font_info::font_info() : d(new font_info_private()) { }

font_info::font_info(font_info_private &dd) : d(&dd) { }

/**
 Copy constructor.
 */
font_info::font_info(const font_info &fi) : d(new font_info_private(*fi.d)) { }

/**
 Destructor.
 */
font_info::~font_info()
{
    delete d;
}

/**
 \returns the name of the font
 */
std::string font_info::name() const
{
    return d->font_name;
}

/**
 \returns the file name of the font, in case the font is not embedded nor subset
 */
std::string font_info::file() const
{
    return d->font_file;
}

/**
 \returns whether the font is totally embedded in the %document
 */
bool font_info::is_embedded() const
{
    return d->is_embedded;
}

/**
 \returns whether there is a subset of the font embedded in the %document
 */
bool font_info::is_subset() const
{
    return d->is_subset;
}

/**
 \returns the type of the font
 */
font_info::type_enum font_info::type() const
{
    return d->type;
}

/**
 Assignment operator.
 */
font_info &font_info::operator=(const font_info &fi)
{
    if (this != &fi) {
        *d = *fi.d;
    }
    return *this;
}

/**
 \class poppler::font_iterator poppler-font.h "poppler/cpp/poppler-font.h"

 Reads the fonts in the PDF %document page by page.

 font_iterator is the way to collect the list of the fonts used in a PDF
 %document, reading them incrementally page by page.

 A typical usage of this might look like:
 \code
poppler::font_iterator *it = doc->create_font_iterator();
while (it->has_next()) {
    std::vector<poppler::font_info> fonts = it->next();
    // do domething with the fonts
}
// after we are done with the iterator, it must be deleted
delete it;
\endcode
 */

font_iterator::font_iterator(int start_page, document_private *dd) : d(new font_iterator_private(start_page, dd)) { }

/**
 Destructor.
 */
font_iterator::~font_iterator()
{
    delete d;
}

/**
 \returns the fonts of the current page and advances to the next one.
 */
std::vector<font_info> font_iterator::next()
{
    if (!has_next()) {
        return std::vector<font_info>();
    }

    ++d->current_page;

    /* FontInfoScanner::scan() receives a number how many pages to
     * be scanned from the *current page*, not from the beginning.
     * We restrict the font scanning to the current page only.
     */
    const std::vector<FontInfo *> items = d->font_info_scanner.scan(1);
    std::vector<font_info> fonts;
    fonts.reserve(items.size());
    for (FontInfo *entry : items) {
        fonts.push_back(font_info(*new font_info_private(entry)));
        delete entry;
    }
    return fonts;
}

/**
 \returns whether the iterator has more pages to advance to
*/
bool font_iterator::has_next() const
{
    return d->current_page < d->total_pages;
}

/**
 \returns the current page
*/
int font_iterator::current_page() const
{
    return d->current_page;
}
