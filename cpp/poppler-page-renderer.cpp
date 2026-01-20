/*
 * Copyright (C) 2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2015 William Bader <williambader@hotmail.com>
 * Copyright (C) 2018, Zsombor Hollay-Horvath <hollay.horvath@gmail.com>
 * Copyright (C) 2019, Julián Unrrein <junrrein@gmail.com>
 * Copyright (C) 2020, 2026, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2021, Hubert Figuiere <hub@figuiere.net>
 * Copyright (C) 2026, g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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
 \file poppler-page-renderer.h
 */
#include "poppler-page-renderer.h"

#include "poppler-document-private.h"
#include "poppler-page-private.h"
#include "poppler-image.h"

#include <config.h>
#include <poppler-config.h>

#include "PDFDoc.h"
#include "SplashOutputDev.h"
#include "splash/SplashBitmap.h"

using namespace poppler;

class poppler::page_renderer_private
{
public:
    page_renderer_private() = default;

    static bool conv_color_mode(image::format_enum mode, SplashColorMode &splash_mode);
    static bool conv_line_mode(page_renderer::line_mode_enum mode, SplashThinLineMode &splash_mode);

    argb paper_color = 0xffffffff;
    unsigned int hints = 0;
    image::format_enum image_format = image::format_enum::format_argb32;
    page_renderer::line_mode_enum line_mode = page_renderer::line_mode_enum::line_default;
};

bool page_renderer_private::conv_color_mode(image::format_enum mode, SplashColorMode &splash_mode)
{
    switch (mode) {
    case image::format_enum::format_mono:
        splash_mode = splashModeMono1;
        break;
    case image::format_enum::format_gray8:
        splash_mode = splashModeMono8;
        break;
    case image::format_enum::format_rgb24:
        splash_mode = splashModeRGB8;
        break;
    case image::format_enum::format_bgr24:
        splash_mode = splashModeBGR8;
        break;
    case image::format_enum::format_argb32:
        splash_mode = splashModeXBGR8;
        break;
    default:
        return false;
    }
    return true;
}

bool page_renderer_private::conv_line_mode(page_renderer::line_mode_enum mode, SplashThinLineMode &splash_mode)
{
    switch (mode) {
    case page_renderer::line_mode_enum::line_default:
        splash_mode = splashThinLineDefault;
        break;
    case page_renderer::line_mode_enum::line_solid:
        splash_mode = splashThinLineSolid;
        break;
    case page_renderer::line_mode_enum::line_shape:
        splash_mode = splashThinLineShape;
        break;
    default:
        return false;
    }
    return true;
}

/**
 \class poppler::page_renderer poppler-page-renderer.h "poppler/cpp/poppler-renderer.h"

 Simple way to render a page of a PDF %document.

 \since 0.16
 */

/**
 \enum poppler::page_renderer::render_hint

 A flag of an option taken into account when rendering
*/

/**
 Constructs a new %page renderer.
 */
page_renderer::page_renderer() : d(new page_renderer_private()) { }

/**
 Destructor.
 */
page_renderer::~page_renderer()
{
    delete d;
}

/**
 The color used for the "paper" of the pages.

 The default color is opaque solid white (0xffffffff).

 \returns the paper color
 */
argb page_renderer::paper_color() const
{
    return d->paper_color;
}

/**
 Set a new color for the "paper".

 \param c the new color
 */
void page_renderer::set_paper_color(argb c)
{
    d->paper_color = c;
}

/**
 The hints used when rendering.

 By default no hint is set.

 \returns the render hints set
 */
unsigned int page_renderer::render_hints() const
{
    return d->hints;
}

/**
 Enable or disable a single render %hint.

 \param hint the hint to modify
 \param on whether enable it or not
 */
void page_renderer::set_render_hint(page_renderer::render_hint hint, bool on)
{
    if (on) {
        d->hints |= hint;
    } else {
        d->hints &= ~(int)hint;
    }
}

/**
 Set new render %hints at once.

 \param hints the new set of render hints
 */
void page_renderer::set_render_hints(unsigned int hints)
{
    d->hints = hints;
}

/**
 The image format used when rendering.

 By default ARGB32 is set.

 \returns the image format

 \since 0.65
 */
image::format_enum page_renderer::image_format() const
{
    return d->image_format;
}

/**
 Set new image format used when rendering.

 \param format the new image format

 \since 0.65
 */
void page_renderer::set_image_format(image::format_enum format)
{
    d->image_format = format;
}

/**
 The line mode used when rendering.

 By default default mode is set.

 \returns the line mode

 \since 0.65
 */
page_renderer::line_mode_enum page_renderer::line_mode() const
{
    return d->line_mode;
}

/**
 Set new line mode used when rendering.

 \param mode the new line mode

 \since 0.65
 */
void page_renderer::set_line_mode(page_renderer::line_mode_enum mode)
{
    d->line_mode = mode;
}

/**
 Render the specified page.

 This functions renders the specified page on an image following the specified
 parameters, returning it.

 \param p the page to render
 \param xres the X resolution, in dot per inch (DPI)
 \param yres the Y resolution, in dot per inch (DPI)
 \param x the X top-right coordinate, in pixels
 \param y the Y top-right coordinate, in pixels
 \param w the width in pixels of the area to render
 \param h the height in pixels of the area to render
 \param rotate the rotation to apply when rendering the page

 \returns the rendered image, or a null one in case of errors

 \see can_render
 */
image page_renderer::render_page(const page *p, double xres, double yres, int x, int y, int w, int h, rotation_enum rotate) const
{
    if (!p) {
        return image();
    }

    page_private *pp = page_private::get(p);
    PDFDoc *pdfdoc = pp->doc->doc.get();

    SplashColorMode colorMode;
    SplashThinLineMode lineMode;

    if (!poppler::page_renderer_private::conv_color_mode(d->image_format, colorMode) || !poppler::page_renderer_private::conv_line_mode(d->line_mode, lineMode)) {
        return image();
    }

    SplashColor bgColor;
    bgColor[0] = d->paper_color & 0xff;
    bgColor[1] = (d->paper_color >> 8) & 0xff;
    bgColor[2] = (d->paper_color >> 16) & 0xff;
    SplashOutputDev splashOutputDev(colorMode, 4, bgColor, true, lineMode);
    splashOutputDev.setFontAntialias((d->hints & text_antialiasing) != 0);
    splashOutputDev.setVectorAntialias((d->hints & antialiasing) != 0);
    splashOutputDev.setFreeTypeHinting((d->hints & text_hinting) != 0, false);
    splashOutputDev.startDoc(pdfdoc);
    pdfdoc->displayPageSlice(&splashOutputDev, pp->index + 1, xres, yres, int(rotate) * 90, false, true, false, x, y, w, h, nullptr, nullptr, nullptr, nullptr, true);

    SplashBitmap *bitmap = splashOutputDev.getBitmap();
    const int bw = bitmap->getWidth();
    const int bh = bitmap->getHeight();

    SplashColorPtr data_ptr = bitmap->getDataPtr();

    const image img(reinterpret_cast<char *>(data_ptr), bw, bh, d->image_format);
    return img.copy();
}

/**
 Rendering capability test.

 page_renderer can render only if a render backend ('Splash') is compiled in
 Poppler.

 \returns whether page_renderer can render
 */
bool page_renderer::can_render()
{
    return true;
}
