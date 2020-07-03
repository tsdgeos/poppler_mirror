/*
 * Copyright (C) 2019, Masamichi Hosoda <trueroad@trueroad.jp>
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

#ifndef POPPLER_DESTINATION_PRIVATE_H
#define POPPLER_DESTINATION_PRIVATE_H

#include "poppler-global.h"
#include "poppler-destination.h"

#include "Object.h"

class PDFDoc;
class LinkDest;

namespace poppler {

class destination_private
{
public:
    destination_private(const LinkDest *ld, PDFDoc *doc);

    destination::type_enum type;
    bool page_number_unresolved;
    union {
        Ref page_ref;
        int page_number;
    };
    double left, bottom;
    double right, top;
    double zoom;
    bool change_left : 1, change_top : 1;
    bool change_zoom : 1;

    PDFDoc *pdf_doc;
};

}

#endif
