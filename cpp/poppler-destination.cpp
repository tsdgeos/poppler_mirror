/*
 * Copyright (C) 2019, Masamichi Hosoda <trueroad@trueroad.jp>
 * Copyright (C) 2019 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2022, Oliver Sander <oliver.sander@tu-dresden.de>
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
 \file poppler-destination.h
 */
#include "poppler-destination.h"

#include "poppler-destination-private.h"

#include "PDFDoc.h"
#include "Link.h"

#include <utility>

using namespace poppler;

destination_private::destination_private(const LinkDest *ld, PDFDoc *doc) : pdf_doc(doc)
{
    if (!ld) {
        type = destination::unknown;
        return;
    }

    switch (ld->getKind()) {
    case ::destXYZ:
        type = destination::xyz;
        break;
    case ::destFit:
        type = destination::fit;
        break;
    case ::destFitH:
        type = destination::fit_h;
        break;
    case ::destFitV:
        type = destination::fit_v;
        break;
    case ::destFitR:
        type = destination::fit_r;
        break;
    case ::destFitB:
        type = destination::fit_b;
        break;
    case ::destFitBH:
        type = destination::fit_b_h;
        break;
    case ::destFitBV:
        type = destination::fit_b_v;
        break;
    default:
        type = destination::unknown;
        break;
    }

    if (!ld->isPageRef()) {
        // The page number has been resolved.
        page_number_unresolved = false;
        page_number = ld->getPageNum();
    } else if (doc) {
        // It is necessary to resolve the page number by its accessor.
        page_number_unresolved = true;
        page_ref = ld->getPageRef();
    } else {
        // The page number cannot be resolved because there is no PDFDoc.
        page_number_unresolved = false;
        page_number = 0;
    }

    left = ld->getLeft();
    bottom = ld->getBottom();
    right = ld->getRight();
    top = ld->getTop();
    zoom = ld->getZoom();
    change_left = ld->getChangeLeft();
    change_top = ld->getChangeTop();
    change_zoom = ld->getChangeZoom();
}

/**
 \class poppler::destination poppler-destination.h "poppler/cpp/poppler-destination.h"

 The information about a destination used in a PDF %document.
 */

/**
 \enum poppler::destination::type_enum

 The various types of destinations available in a PDF %document.
*/
/**
 \var poppler::destination::type_enum poppler::destination::unknown

 unknown destination
*/
/**
 \var poppler::destination::type_enum poppler::destination::xyz

 go to page with coordinates (left, top) positioned at the upper-left
 corner of the window and the contents of the page magnified
 by the factor zoom
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit

 go to page with its contents magnified just enough to fit the entire page
 within the window both horizontally and vertically
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_h

 go to page with the vertical coordinate top positioned at the top edge
 of the window and the contents of the page magnified just enough to fit
 the entire width of the page within the window
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_v

 go to page with the horizontal coordinate left positioned at the left edge
 of the window and the contents of the page magnified just enough to fit
 the entire height of the page within the window
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_r

 go to page with its contents magnified just enough to fit the rectangle
 specified by the coordinates left, bottom, right, and top entirely
 within the window both horizontally and vertically
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_b

 go to page with its contents magnified just enough to fit its bounding box
 entirely within the window both horizontally and vertically
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_b_h

 go to page with the vertical coordinate top positioned at the top edge
 of the window and the contents of the page magnified just enough to fit
 the entire width of its bounding box within the window
*/
/**
 \var poppler::destination::type_enum poppler::destination::fit_b_v

 go to page with the horizontal coordinate left positioned at the left edge
 of the window and the contents of the page magnified just enough to fit
 the entire height of its bounding box within the window
*/

destination::destination(destination_private *dd) : d(dd) { }

/**
 Move constructor.
 */
destination::destination(destination &&other) noexcept
{
    *this = std::move(other);
}

/**
 \returns the type of the destination
 */
destination::type_enum destination::type() const
{
    return d->type;
}

/**
 \note It is necessary not to destruct parent poppler::document
 before calling this function for the first time.

 \returns the page number of the destination
 */
int destination::page_number() const
{
    if (d->page_number_unresolved) {
        d->page_number_unresolved = false;
        d->page_number = d->pdf_doc->findPage(d->page_ref);
    }

    return d->page_number;
}

/**
 \returns the left coordinate of the destination
 */
double destination::left() const
{
    return d->left;
}

/**
 \returns the bottom coordinate of the destination
 */
double destination::bottom() const
{
    return d->bottom;
}

/**
 \returns the right coordinate of the destination
 */
double destination::right() const
{
    return d->right;
}

/**
 \returns the top coordinate of the destination
 */
double destination::top() const
{
    return d->top;
}

/**
 \returns the scale factor of the destination
 */
double destination::zoom() const
{
    return d->zoom;
}

/**
 \returns whether left coordinate should be changed
 */
bool destination::is_change_left() const
{
    return d->change_left;
}

/**
 \returns whether top coordinate should be changed
 */
bool destination::is_change_top() const
{
    return d->change_top;
}

/**
 \returns whether scale factor should be changed
 */
bool destination::is_change_zoom() const
{
    return d->change_zoom;
}

/**
 Move assignment operator.
 */
destination &destination::operator=(destination &&other) noexcept = default;

/**
 Destructor.
 */
destination::~destination() = default;
