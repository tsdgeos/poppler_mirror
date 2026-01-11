/*
 * Copyright (C) 2009, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2019, Oliver Sander <oliver.sander@tu-dresden.de>
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

#ifndef POPPLER_TOC_PRIVATE_H
#define POPPLER_TOC_PRIVATE_H

#include "poppler-global.h"
#include "poppler-toc.h"

#include <vector>

class Outline;
class OutlineItem;

namespace poppler {

class toc_private
{
public:
    toc_private();
    ~toc_private();

    static toc *load_from_outline(Outline *outline);

    toc_item root;
};

class toc_item_private
{
public:
    toc_item_private();
    ~toc_item_private();

    toc_item_private(const toc_item_private &) = delete;
    toc_item_private &operator=(const toc_item_private &) = delete;

    void load(const OutlineItem *item);
    void load_children(const std::vector<OutlineItem *> *items);

    std::vector<toc_item *> children;
    ustring title;
    bool is_open = false;
};

}

#endif
