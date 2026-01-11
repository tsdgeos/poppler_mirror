/*
 * Copyright (C) 2009-2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018, 2024, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2019, Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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
 \file poppler-toc.h
 */
#include "poppler-toc.h"

#include "poppler-toc-private.h"
#include "poppler-private.h"

#include "Outline.h"

using namespace poppler;

toc_private::toc_private() = default;

toc_private::~toc_private() = default;

toc *toc_private::load_from_outline(Outline *outline)
{
    if (!outline) {
        return nullptr;
    }

    const std::vector<OutlineItem *> *items = outline->getItems();
    if (!items || items->empty()) {
        return nullptr;
    }

    toc *newtoc = new toc();
    newtoc->d->root.d->is_open = true;
    newtoc->d->root.d->load_children(items);

    return newtoc;
}

toc_item_private::toc_item_private() = default;

toc_item_private::~toc_item_private()
{
    delete_all(children);
}

void toc_item_private::load(const OutlineItem *item)
{
    const std::vector<Unicode> &title_unicode = item->getTitle();
    title = detail::unicode_to_ustring(title_unicode.data(), title_unicode.size());
    is_open = item->isOpen();
}

void toc_item_private::load_children(const std::vector<OutlineItem *> *items)
{
    const int num_items = items->size();
    children.resize(num_items);
    for (int i = 0; i < num_items; ++i) {
        OutlineItem *item = (*items)[i];

        auto *new_item = new toc_item();
        new_item->d->load(item);
        children[i] = new_item;

        item->open();
        const std::vector<OutlineItem *> *item_children = item->getKids();
        if (item_children) {
            new_item->d->load_children(item_children);
        }
    }
}

/**
 \class poppler::toc poppler-toc.h "poppler/cpp/poppler-toc.h"

 Represents the TOC (Table of Contents) of a PDF %document.

 The TOC of a PDF %document is represented as a tree of items.
 */

toc::toc() : d(new toc_private()) { }

/**
 Destroys the TOC.
 */
toc::~toc()
{
    delete d;
}

/**
 Returns the "invisible item" representing the root of the TOC.

 This item is special, it has no title nor actions, it is open and its children
 are the effective root items of the TOC. This is provided as a convenience
 when iterating through the TOC.

 \returns the root "item"
 */
toc_item *toc::root() const
{
    return &d->root;
}

/**
 \class poppler::toc_item poppler-toc.h "poppler/cpp/poppler-toc.h"

 Represents an item of the TOC (Table of Contents) of a PDF %document.
 */

/**
 \typedef std::vector<toc_item *>::const_iterator poppler::toc_item::iterator

 An iterator for the children of a TOC item.
 */

toc_item::toc_item() : d(new toc_item_private()) { }

/**
 Destroys the TOC item.
 */
toc_item::~toc_item()
{
    delete d;
}

/**
 \returns the title of the TOC item
 */
ustring toc_item::title() const
{
    return d->title;
}

/**
 Returns whether the TOC item should be represented as open when showing the
 TOC.

 This is not a functional behaviour, but a visualisation hint of the item.
 Regardless of this state, the item can be expanded and collapsed freely when
 represented in a TOC view of a PDF viewer.

 \returns whether the TOC item should be open
 */
bool toc_item::is_open() const
{
    return d->is_open;
}

/**
 \returns the children of the TOC item
 */
std::vector<toc_item *> toc_item::children() const
{
    return d->children;
}

/**
 \returns an iterator to the being of the list of children of the TOC item
 */
toc_item::iterator toc_item::children_begin() const
{
    return d->children.begin();
}

/**
 \returns an iterator to the end of the list of children of the TOC item
 */
toc_item::iterator toc_item::children_end() const
{
    return d->children.end();
}
