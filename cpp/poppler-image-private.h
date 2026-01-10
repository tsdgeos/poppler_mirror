/*
 * Copyright (C) 2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018, 2022, Albert Astals Cid <aacid@kde.org>
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

#ifndef POPPLER_IMAGE_PRIVATE_H
#define POPPLER_IMAGE_PRIVATE_H

#include "poppler-image.h"

namespace poppler {

class image_private
{
public:
    image_private(int iwidth, int iheight, image::format_enum iformat);
    ~image_private();

    image_private(const image_private &) = delete;
    image_private &operator=(const image_private &) = delete;

    static image_private *create_data(int width, int height, image::format_enum format);
    static image_private *create_data(char *data, int width, int height, image::format_enum format);

    int ref = 1;
    char *data = nullptr;
    int width;
    int height;
    int bytes_per_row = 0;
    int bytes_num = 0;
    image::format_enum format;
    bool own_data : 1 = true;
};

}

#endif
