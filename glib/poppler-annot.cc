/* poppler-annot.cc: glib interface to poppler
 *
 * Copyright (C) 2007 Inigo Martinez <inigomartinez@gmail.com>
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2013 German Poo-Caamano <gpoo@gnome.org>
 * Copyright (C) 2025, 2026 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2025 Markus Göllnitz <camelcasenick@bewares.it>
 * Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Copyright (C) 2025 Lucas Baudin <lucas.baudin@ensae.fr>
 * Copyright (C) 2025 Aditya Tiwari <suntiwari3495@gmail.com>
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

#include "AnnotStampImageHelper.h"
#include "config.h"
#include "poppler.h"
#include "poppler-private.h"

#define ZERO_CROPBOX(c) (!((c) && ((c)->x1 > 0.01 || (c)->y1 > 0.01)))

const PDFRectangle *_poppler_annot_get_cropbox_and_page(PopplerAnnot *poppler_annot, Page **page_out);
std::unique_ptr<AnnotStampImageHelper> _poppler_convert_cairo_image_to_stamp_image_helper(cairo_surface_t *image, PDFDoc *doc, GError **error);

/**
 * SECTION:poppler-annot
 * @short_description: Annotations
 * @title: PopplerAnnot
 */
struct _PopplerAnnotClass
{
    GObjectClass parent_class;
};
using PopplerAnnotClass = _PopplerAnnotClass;

struct _PopplerAnnotMarkup
{
    PopplerAnnot parent_instance;
};

struct _PopplerAnnotMarkupClass
{
    PopplerAnnotClass parent_class;
};
using PopplerAnnotMarkupClass = _PopplerAnnotMarkupClass;

struct _PopplerAnnotText
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotTextClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotTextClass = _PopplerAnnotTextClass;

struct _PopplerAnnotTextMarkup
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotTextMarkupClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotTextMarkupClass = _PopplerAnnotTextMarkupClass;

struct _PopplerAnnotFreeText
{
    PopplerAnnotMarkup parent_instance;
    PopplerFontDescription *font_desc;
    PopplerColor font_color;
};

struct _PopplerAnnotFreeTextClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotFreeTextClass = _PopplerAnnotFreeTextClass;

struct _PopplerAnnotFileAttachment
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotFileAttachmentClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotFileAttachmentClass = _PopplerAnnotFileAttachmentClass;

struct _PopplerAnnotMovie
{
    PopplerAnnot parent_instance;

    PopplerMovie *movie;
};

struct _PopplerAnnotMovieClass
{
    PopplerAnnotClass parent_class;
};
using PopplerAnnotMovieClass = _PopplerAnnotMovieClass;

struct _PopplerAnnotScreen
{
    PopplerAnnot parent_instance;

    PopplerAction *action;
};

struct _PopplerAnnotScreenClass
{
    PopplerAnnotClass parent_class;
};
using PopplerAnnotScreenClass = _PopplerAnnotScreenClass;

struct _PopplerAnnotLine
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotLineClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotLineClass = _PopplerAnnotLineClass;

struct _PopplerAnnotCircle
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotCircleClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotCircleClass = _PopplerAnnotCircleClass;

struct _PopplerAnnotSquare
{
    PopplerAnnotMarkup parent_instance;
};

struct _PopplerAnnotSquareClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotSquareClass = _PopplerAnnotSquareClass;

struct _PopplerAnnotStamp
{
    PopplerAnnotMarkup parent_instance;
};
struct _PopplerAnnotStampClass
{
    PopplerAnnotMarkupClass parent_class;
};
using PopplerAnnotStampClass = _PopplerAnnotStampClass;

struct _PopplerAnnotInk
{
    PopplerAnnot parent_instance;
};

struct _PopplerAnnotInkClass
{
    PopplerAnnotClass parent_class;
};
using PopplerAnnotInkClass = _PopplerAnnotInkClass;

G_DEFINE_TYPE(PopplerAnnot, poppler_annot, G_TYPE_OBJECT)
G_DEFINE_TYPE(PopplerAnnotMarkup, poppler_annot_markup, POPPLER_TYPE_ANNOT)
G_DEFINE_TYPE(PopplerAnnotTextMarkup, poppler_annot_text_markup, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotText, poppler_annot_text, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotFreeText, poppler_annot_free_text, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotFileAttachment, poppler_annot_file_attachment, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotMovie, poppler_annot_movie, POPPLER_TYPE_ANNOT)
G_DEFINE_TYPE(PopplerAnnotScreen, poppler_annot_screen, POPPLER_TYPE_ANNOT)
G_DEFINE_TYPE(PopplerAnnotLine, poppler_annot_line, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotCircle, poppler_annot_circle, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotSquare, poppler_annot_square, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotStamp, poppler_annot_stamp, POPPLER_TYPE_ANNOT_MARKUP)
G_DEFINE_TYPE(PopplerAnnotInk, poppler_annot_ink, POPPLER_TYPE_ANNOT_MARKUP)

static PopplerAnnot *_poppler_create_annot(GType annot_type, std::shared_ptr<Annot> annot)
{
    PopplerAnnot *poppler_annot;

    poppler_annot = POPPLER_ANNOT(g_object_new(annot_type, nullptr));
    poppler_annot->annot = std::move(annot);

    return poppler_annot;
}

static void poppler_annot_finalize(GObject *object)
{
    PopplerAnnot *poppler_annot = POPPLER_ANNOT(object);

    if (poppler_annot->annot) {
        poppler_annot->annot.reset();
    }

    G_OBJECT_CLASS(poppler_annot_parent_class)->finalize(object);
}

static void poppler_annot_init(PopplerAnnot * /*poppler_annot*/) { }

static void poppler_annot_class_init(PopplerAnnotClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_annot_finalize;
}

PopplerAnnot *_poppler_annot_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT, annot);
}

static void poppler_annot_markup_init(PopplerAnnotMarkup * /*poppler_annot*/) { }

static void poppler_annot_markup_class_init(PopplerAnnotMarkupClass * /*klass*/) { }

static void poppler_annot_text_init(PopplerAnnotText * /*poppler_annot*/) { }

static void poppler_annot_text_class_init(PopplerAnnotTextClass * /*klass*/) { }

PopplerAnnot *_poppler_annot_text_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_TEXT, annot);
}

/**
 * poppler_annot_text_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new Text annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: A newly created #PopplerAnnotText annotation
 *
 * Since: 0.16
 */
PopplerAnnot *poppler_annot_text_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotText>(doc->doc.get(), &pdf_rect);

    return _poppler_annot_text_new(annot);
}

PopplerAnnot *_poppler_annot_text_markup_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_TEXT_MARKUP, annot);
}

static AnnotQuadrilaterals *create_annot_quads_from_poppler_quads(GArray *quads)
{
    g_assert(quads->len > 0);

    auto quads_array = std::make_unique<AnnotQuadrilaterals::AnnotQuadrilateral[]>(quads->len);
    for (guint i = 0; i < quads->len; i++) {
        PopplerQuadrilateral *quadrilateral = &g_array_index(quads, PopplerQuadrilateral, i);

        quads_array[i] = AnnotQuadrilaterals::AnnotQuadrilateral(quadrilateral->p1.x, quadrilateral->p1.y, quadrilateral->p2.x, quadrilateral->p2.y, quadrilateral->p3.x, quadrilateral->p3.y, quadrilateral->p4.x, quadrilateral->p4.y);
    }

    return new AnnotQuadrilaterals(std::move(quads_array), quads->len);
}

/* If @crop_box parameter is non null, it will substract the crop_box offset
 * from the coordinates of the returned #PopplerQuadrilateral array */
static GArray *create_poppler_quads_from_annot_quads(AnnotQuadrilaterals *quads_array, const PDFRectangle *crop_box)
{
    GArray *quads;
    guint quads_len;
    PDFRectangle zerobox;

    if (!crop_box) {
        zerobox = PDFRectangle();
        crop_box = &zerobox;
    }

    quads_len = quads_array->getQuadrilateralsLength();
    quads = g_array_sized_new(FALSE, FALSE, sizeof(PopplerQuadrilateral), quads_len);
    g_array_set_size(quads, quads_len);

    for (guint i = 0; i < quads_len; ++i) {
        PopplerQuadrilateral *quadrilateral = &g_array_index(quads, PopplerQuadrilateral, i);

        quadrilateral->p1.x = quads_array->getX1(i) - crop_box->x1;
        quadrilateral->p1.y = quads_array->getY1(i) - crop_box->y1;
        quadrilateral->p2.x = quads_array->getX2(i) - crop_box->x1;
        quadrilateral->p2.y = quads_array->getY2(i) - crop_box->y1;
        quadrilateral->p3.x = quads_array->getX3(i) - crop_box->x1;
        quadrilateral->p3.y = quads_array->getY3(i) - crop_box->y1;
        quadrilateral->p4.x = quads_array->getX4(i) - crop_box->x1;
        quadrilateral->p4.y = quads_array->getY4(i) - crop_box->y1;
    }

    return quads;
}

static void poppler_annot_text_markup_init(PopplerAnnotTextMarkup * /*poppler_annot*/) { }

static void poppler_annot_text_markup_class_init(PopplerAnnotTextMarkupClass * /*klass*/) { }

/**
 * poppler_annot_text_markup_new_highlight:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 * @quadrilaterals: (element-type PopplerQuadrilateral): A #GArray of
 *   #PopplerQuadrilateral<!-- -->s
 *
 * Creates a new Highlight Text annotation that will be
 * located on @rect when added to a page. See poppler_page_add_annot()
 *
 * Return value: (transfer full): A newly created #PopplerAnnotTextMarkup annotation
 *
 * Since: 0.26
 */
PopplerAnnot *poppler_annot_text_markup_new_highlight(PopplerDocument *doc, PopplerRectangle *rect, GArray *quadrilaterals)
{
    PopplerAnnot *poppler_annot;
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotTextMarkup>(doc->doc.get(), &pdf_rect, Annot::typeHighlight);

    poppler_annot = _poppler_annot_text_markup_new(annot);
    poppler_annot_text_markup_set_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(poppler_annot), quadrilaterals);
    return poppler_annot;
}

/**
 * poppler_annot_text_markup_new_squiggly:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 * @quadrilaterals: (element-type PopplerQuadrilateral): A #GArray of
 *   #PopplerQuadrilateral<!-- -->s
 *
 * Creates a new Squiggly Text annotation that will be
 * located on @rect when added to a page. See poppler_page_add_annot()
 *
 * Return value: (transfer full): A newly created #PopplerAnnotTextMarkup annotation
 *
 * Since: 0.26
 */
PopplerAnnot *poppler_annot_text_markup_new_squiggly(PopplerDocument *doc, PopplerRectangle *rect, GArray *quadrilaterals)
{
    PopplerAnnot *poppler_annot;
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    g_return_val_if_fail(quadrilaterals != nullptr && quadrilaterals->len > 0, NULL);

    auto annot = std::make_shared<AnnotTextMarkup>(doc->doc.get(), &pdf_rect, Annot::typeSquiggly);

    poppler_annot = _poppler_annot_text_markup_new(annot);
    poppler_annot_text_markup_set_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(poppler_annot), quadrilaterals);
    return poppler_annot;
}

/**
 * poppler_annot_text_markup_new_strikeout:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 * @quadrilaterals: (element-type PopplerQuadrilateral): A #GArray of
 *   #PopplerQuadrilateral<!-- -->s
 *
 * Creates a new Strike Out Text annotation that will be
 * located on @rect when added to a page. See poppler_page_add_annot()
 *
 * Return value: (transfer full): A newly created #PopplerAnnotTextMarkup annotation
 *
 * Since: 0.26
 */
PopplerAnnot *poppler_annot_text_markup_new_strikeout(PopplerDocument *doc, PopplerRectangle *rect, GArray *quadrilaterals)
{
    PopplerAnnot *poppler_annot;
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    g_return_val_if_fail(quadrilaterals != nullptr && quadrilaterals->len > 0, NULL);

    auto annot = std::make_shared<AnnotTextMarkup>(doc->doc.get(), &pdf_rect, Annot::typeStrikeOut);

    poppler_annot = _poppler_annot_text_markup_new(annot);
    poppler_annot_text_markup_set_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(poppler_annot), quadrilaterals);
    return poppler_annot;
}

/**
 * poppler_annot_text_markup_new_underline:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 * @quadrilaterals: (element-type PopplerQuadrilateral): A #GArray of
 *   #PopplerQuadrilateral<!-- -->s
 *
 * Creates a new Underline Text annotation that will be
 * located on @rect when added to a page. See poppler_page_add_annot()
 *
 * Return value: (transfer full): A newly created #PopplerAnnotTextMarkup annotation
 *
 * Since: 0.26
 */
PopplerAnnot *poppler_annot_text_markup_new_underline(PopplerDocument *doc, PopplerRectangle *rect, GArray *quadrilaterals)
{
    PopplerAnnot *poppler_annot;
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    g_return_val_if_fail(quadrilaterals != nullptr && quadrilaterals->len > 0, NULL);

    auto annot = std::make_shared<AnnotTextMarkup>(doc->doc.get(), &pdf_rect, Annot::typeUnderline);

    poppler_annot = _poppler_annot_text_markup_new(annot);
    poppler_annot_text_markup_set_quadrilaterals(POPPLER_ANNOT_TEXT_MARKUP(poppler_annot), quadrilaterals);
    return poppler_annot;
}

PopplerColor *_poppler_convert_annot_color_to_poppler_color(const AnnotColor *color)
{
    PopplerColor *poppler_color = nullptr;

    if (color) {
        const std::array<double, 4> &values = color->getValues();

        switch (color->getSpace()) {
        case AnnotColor::colorGray:
            poppler_color = g_new(PopplerColor, 1);

            poppler_color->red = CLAMP((guint16)(values[0] * 65535), 0, 65535);
            poppler_color->green = poppler_color->red;
            poppler_color->blue = poppler_color->red;

            break;
        case AnnotColor::colorRGB:
            poppler_color = g_new(PopplerColor, 1);

            poppler_color->red = CLAMP((guint16)(values[0] * 65535), 0, 65535);
            poppler_color->green = CLAMP((guint16)(values[1] * 65535), 0, 65535);
            poppler_color->blue = CLAMP((guint16)(values[2] * 65535), 0, 65535);

            break;
        case AnnotColor::colorCMYK:
            g_warning("Unsupported Annot Color: colorCMYK");
            break;
        case AnnotColor::colorTransparent:
            break;
        }
    }

    return poppler_color;
}

std::unique_ptr<AnnotColor> _poppler_convert_poppler_color_to_annot_color(const PopplerColor *poppler_color)
{
    if (!poppler_color) {
        return nullptr;
    }

    return std::make_unique<AnnotColor>(CLAMP((double)poppler_color->red / 65535, 0., 1.), CLAMP((double)poppler_color->green / 65535, 0., 1.), CLAMP((double)poppler_color->blue / 65535, 0., 1.));
}

static void poppler_annot_free_text_init(PopplerAnnotFreeText * /*poppler_annot*/) { }

static void poppler_annot_free_text_class_init(PopplerAnnotFreeTextClass * /*klass*/) { }

// Single map with string keys and enum values
enum FontPropType
{
    TYPE_STYLE,
    TYPE_WEIGHT,
    TYPE_STRETCH,
    TYPE_NORMAL
};

using FontstyleMap = std::map<std::string, std::pair<FontPropType, int>>;

static const FontstyleMap string_to_fontstyle = { { "UltraCondensed", std::pair(TYPE_STRETCH, POPPLER_STRETCH_ULTRA_CONDENSED) },
                                                  { "ExtraCondensed", std::pair(TYPE_STRETCH, POPPLER_STRETCH_EXTRA_CONDENSED) },
                                                  { "Condensed", std::pair(TYPE_STRETCH, POPPLER_STRETCH_CONDENSED) },
                                                  { "SemiCondensed", std::pair(TYPE_STRETCH, POPPLER_STRETCH_SEMI_CONDENSED) },
                                                  { "SemiExpanded", std::pair(TYPE_STRETCH, POPPLER_STRETCH_SEMI_EXPANDED) },
                                                  { "Expanded", std::pair(TYPE_STRETCH, POPPLER_STRETCH_EXPANDED) },
                                                  { "UltraExpanded", std::pair(TYPE_STRETCH, POPPLER_STRETCH_ULTRA_EXPANDED) },
                                                  { "ExtraExpanded", std::pair(TYPE_STRETCH, POPPLER_STRETCH_EXTRA_EXPANDED) },
                                                  { "Thin", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_THIN) },
                                                  { "UltraLight", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_ULTRALIGHT) },
                                                  { "Light", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_LIGHT) },
                                                  { "Medium", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_MEDIUM) },
                                                  { "SemiBold", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_SEMIBOLD) },
                                                  { "Bold", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_BOLD) },
                                                  { "UltraBold", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_ULTRABOLD) },
                                                  { "Heavy", std::pair(TYPE_WEIGHT, POPPLER_WEIGHT_HEAVY) },
                                                  { "Italic", std::pair(TYPE_STYLE, POPPLER_STYLE_ITALIC) },
                                                  { "Oblique", std::pair(TYPE_STYLE, POPPLER_STYLE_OBLIQUE) },
                                                  { "Regular", std::pair(TYPE_NORMAL, 0) },
                                                  { "Normal", std::pair(TYPE_NORMAL, 0) } };

static const char *stretch_to_str[] = { "UltraCondensed", "ExtraCondensed", "Condensed", "SemiCondensed", /* Normal */ "", "SemiExpanded", "Expanded", "ExtraExpanded", "UltraExpanded" };

const std::map<std::string, std::string> fallback_fonts = {
    { "/Helvetica", "Helvetica" }, /* iOS */
    { "Helv", "Helvetica" } /* Firefox */
};

static bool update_font_desc_with_word(PopplerFontDescription &font_desc, std::string &word)
{
    auto a = string_to_fontstyle.find(word);
    if (a != string_to_fontstyle.end()) {
        std::pair<FontPropType, int> elt = a->second;
        switch (elt.first) {
        case TYPE_STYLE:
            font_desc.style = (PopplerStyle)elt.second;
            return true;
        case TYPE_WEIGHT:
            font_desc.weight = (PopplerWeight)elt.second;
            return true;
        case TYPE_STRETCH:
            font_desc.stretch = (PopplerStretch)elt.second;
            return true;
        case TYPE_NORMAL:
            return true;
        }
    }
    return false;
}

static void poppler_font_name_to_description(const std::string &name, PopplerFontDescription &font_desc)
{
    /* Last three words of the font name may be style indications */
    size_t end = name.size();
    size_t start;

    for (int i = 3; i >= 1; --i) {
        start = name.find_last_of(' ', end - 1);
        if (start == std::string::npos) {
            break;
        }
        std::string word = name.substr(start + 1, end - start - 1);

        if (!update_font_desc_with_word(font_desc, word)) {
            break;
        }

        end = start;
    }
    font_desc.font_name = g_strdup(name.substr(0, end).c_str());
}

PopplerAnnot *_poppler_annot_free_text_new(const std::shared_ptr<Annot> &annot)
{
    PopplerAnnot *poppler_annot = _poppler_create_annot(POPPLER_TYPE_ANNOT_FREE_TEXT, annot);
    PopplerAnnotFreeText *ft_annot = POPPLER_ANNOT_FREE_TEXT(poppler_annot);
    std::unique_ptr<DefaultAppearance> da = ((AnnotFreeText *)annot.get())->getDefaultAppearance();
    PopplerFontDescription *desc = nullptr;
    if (!da->getFontName().empty()) {
        desc = poppler_font_description_new(da->getFontName().c_str());
        desc->size_pt = da->getFontPtSize();

        /* Attempt to resolve the actual font name. */
        Form *form = annot->getDoc()->getCatalog()->getCreateForm();
        if (form) {
            GfxResources *res = form->getDefaultResources();
            if (res) {
                std::shared_ptr<GfxFont> font = res->lookupFont(desc->font_name);
                if (font) {
                    const std::optional<std::string> &fontName = font->getName();
                    if (fontName) {
                        poppler_font_name_to_description(fontName.value(), *desc);
                    }
                }
            }
        }

        auto fallback_font = fallback_fonts.find(std::string(desc->font_name));
        if (fallback_font != fallback_fonts.end()) {
            desc->font_name = g_strdup(fallback_font->second.c_str());
        }
    }

    ft_annot->font_desc = desc;

    const AnnotColor *ac = da->getFontColor();

    if (ac) {
        PopplerColor *font_color = _poppler_convert_annot_color_to_poppler_color(ac);
        ft_annot->font_color = *font_color;
        poppler_color_free(font_color);
    }

    return poppler_annot;
}

/**
 * poppler_annot_free_text_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new Free Text annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot(). It initially has no content. Font family, size and
 * color are initially undefined and must be set, see
 * poppler_annot_free_text_set_font_desc() and
 * poppler_annot_free_text_set_font_color().
 *
 * Returns: (transfer full): A newly created #PopplerAnnotFreeText annotation
 */
PopplerAnnot *poppler_annot_free_text_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotFreeText>(doc->doc.get(), &pdf_rect);

    PopplerAnnot *poppler_annot = _poppler_annot_free_text_new(annot);

    return poppler_annot;
}

static void poppler_annot_file_attachment_init(PopplerAnnotFileAttachment * /*poppler_annot*/) { }

static void poppler_annot_file_attachment_class_init(PopplerAnnotFileAttachmentClass * /*klass*/) { }

PopplerAnnot *_poppler_annot_file_attachment_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_FILE_ATTACHMENT, annot);
}

static void poppler_annot_movie_finalize(GObject *object)
{
    PopplerAnnotMovie *annot_movie = POPPLER_ANNOT_MOVIE(object);

    if (annot_movie->movie) {
        g_object_unref(annot_movie->movie);
        annot_movie->movie = nullptr;
    }

    G_OBJECT_CLASS(poppler_annot_movie_parent_class)->finalize(object);
}

static void poppler_annot_movie_init(PopplerAnnotMovie * /*poppler_annot*/) { }

static void poppler_annot_movie_class_init(PopplerAnnotMovieClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_annot_movie_finalize;
}

PopplerAnnot *_poppler_annot_movie_new(const std::shared_ptr<Annot> &annot)
{
    PopplerAnnot *poppler_annot;
    AnnotMovie *annot_movie;

    poppler_annot = _poppler_create_annot(POPPLER_TYPE_ANNOT_MOVIE, annot);
    annot_movie = static_cast<AnnotMovie *>(poppler_annot->annot.get());
    POPPLER_ANNOT_MOVIE(poppler_annot)->movie = _poppler_movie_new(annot_movie->getMovie());

    return poppler_annot;
}

static void poppler_annot_screen_finalize(GObject *object)
{
    PopplerAnnotScreen *annot_screen = POPPLER_ANNOT_SCREEN(object);

    if (annot_screen->action) {
        poppler_action_free(annot_screen->action);
        annot_screen->action = nullptr;
    }

    G_OBJECT_CLASS(poppler_annot_screen_parent_class)->finalize(object);
}

static void poppler_annot_screen_init(PopplerAnnotScreen * /*poppler_annot*/) { }

static void poppler_annot_screen_class_init(PopplerAnnotScreenClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_annot_screen_finalize;
}

PopplerAnnot *_poppler_annot_screen_new(PopplerDocument *doc, const std::shared_ptr<Annot> &annot)
{
    PopplerAnnot *poppler_annot;
    AnnotScreen *annot_screen;
    LinkAction *action;

    poppler_annot = _poppler_create_annot(POPPLER_TYPE_ANNOT_SCREEN, annot);
    annot_screen = static_cast<AnnotScreen *>(poppler_annot->annot.get());
    action = annot_screen->getAction();
    if (action) {
        POPPLER_ANNOT_SCREEN(poppler_annot)->action = _poppler_action_new(doc, action, nullptr);
    }

    return poppler_annot;
}

PopplerAnnot *_poppler_annot_line_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_LINE, annot);
}

static void poppler_annot_line_init(PopplerAnnotLine * /*poppler_annot*/) { }

static void poppler_annot_line_class_init(PopplerAnnotLineClass * /*klass*/) { }

/**
 * poppler_annot_line_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 * @start: a #PopplerPoint of the starting vertice
 * @end: a #PopplerPoint of the ending vertice
 *
 * Creates a new Line annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: A newly created #PopplerAnnotLine annotation
 *
 * Since: 0.26
 */
PopplerAnnot *poppler_annot_line_new(PopplerDocument *doc, PopplerRectangle *rect, PopplerPoint *start, PopplerPoint *end)
{
    PopplerAnnot *poppler_annot;
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotLine>(doc->doc.get(), &pdf_rect);

    poppler_annot = _poppler_annot_line_new(annot);
    poppler_annot_line_set_vertices(POPPLER_ANNOT_LINE(poppler_annot), start, end);
    return poppler_annot;
}

PopplerAnnot *_poppler_annot_circle_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_CIRCLE, annot);
}

static void poppler_annot_circle_init(PopplerAnnotCircle * /*poppler_annot*/) { }

static void poppler_annot_circle_class_init(PopplerAnnotCircleClass * /*klass*/) { }

/**
 * poppler_annot_circle_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new Circle annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: a newly created #PopplerAnnotCircle annotation
 *
 * Since: 0.26
 **/
PopplerAnnot *poppler_annot_circle_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotGeometry>(doc->doc.get(), &pdf_rect, Annot::typeCircle);

    return _poppler_annot_circle_new(annot);
}

PopplerAnnot *_poppler_annot_square_new(const std::shared_ptr<Annot> &annot)
{
    return _poppler_create_annot(POPPLER_TYPE_ANNOT_SQUARE, annot);
}

static void poppler_annot_square_init(PopplerAnnotSquare * /*poppler_annot*/) { }

static void poppler_annot_square_class_init(PopplerAnnotSquareClass * /*klass*/) { }

/**
 * poppler_annot_square_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new Square annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: a newly created #PopplerAnnotSquare annotation
 *
 * Since: 0.26
 **/
PopplerAnnot *poppler_annot_square_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotGeometry>(doc->doc.get(), &pdf_rect, Annot::typeSquare);

    return _poppler_annot_square_new(annot);
}

static void poppler_annot_stamp_finalize(GObject *object)
{
    G_OBJECT_CLASS(poppler_annot_stamp_parent_class)->finalize(object);
}

static void poppler_annot_stamp_init(PopplerAnnotStamp * /*poppler_annot*/) { }

static void poppler_annot_stamp_class_init(PopplerAnnotStampClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_annot_stamp_finalize;
}

PopplerAnnot *_poppler_annot_stamp_new(const std::shared_ptr<Annot> &annot)
{
    PopplerAnnot *poppler_annot;

    poppler_annot = _poppler_create_annot(POPPLER_TYPE_ANNOT_STAMP, annot);

    return poppler_annot;
}

/**
 * poppler_annot_stamp_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new Stamp annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: a newly created #PopplerAnnotStamp annotation
 *
 * Since: 22.07.0
 **/
PopplerAnnot *poppler_annot_stamp_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotStamp>(doc->doc.get(), &pdf_rect);

    return _poppler_annot_stamp_new(annot);
}

static gboolean get_raw_data_from_cairo_image(cairo_surface_t *image, cairo_format_t format, const int width, const int height, const size_t rowstride_c, GByteArray *data, GByteArray *soft_mask_data)
{
    gboolean has_alpha = format == CAIRO_FORMAT_ARGB32;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    static const size_t CAIRO_B = 0;
    static const size_t CAIRO_G = 1;
    static const size_t CAIRO_R = 2;
    static const size_t CAIRO_A = 3;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    static const size_t CAIRO_A = 0;
    static const size_t CAIRO_R = 1;
    static const size_t CAIRO_G = 2;
    static const size_t CAIRO_B = 3;
#else
#    error "Unsupported endian type"
#endif

    cairo_surface_flush(image);
    unsigned char *pixels_c = cairo_image_surface_get_data(image);

    if (format == CAIRO_FORMAT_ARGB32 || format == CAIRO_FORMAT_RGB24) {
        unsigned char pixel[3];

        for (int h = 0; h < height; h++) {
            unsigned char *iter_c = pixels_c + h * rowstride_c;
            for (int w = 0; w < width; w++) {
                pixel[0] = iter_c[CAIRO_R];
                pixel[1] = iter_c[CAIRO_G];
                pixel[2] = iter_c[CAIRO_B];
                iter_c += 4;

                g_byte_array_append(data, (guint8 *)pixel, 3);
                if (has_alpha) {
                    g_byte_array_append(soft_mask_data, (guint8 *)&iter_c[CAIRO_A], 1);
                }
            }
        }
        return TRUE;
    }

    return FALSE;
}

std::unique_ptr<AnnotStampImageHelper> _poppler_convert_cairo_image_to_stamp_image_helper(cairo_surface_t *image, PDFDoc *doc, GError **error)
{
    std::unique_ptr<AnnotStampImageHelper> annotImg;
    GByteArray *data;
    GByteArray *sMaskData;

    int bitsPerComponent;
    const int width = cairo_image_surface_get_width(image);
    const int height = cairo_image_surface_get_height(image);
    const size_t rowstride_c = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    cairo_format_t format = cairo_image_surface_get_format(image);

    ColorSpace colorSpace;

    if (format == CAIRO_FORMAT_ARGB32 || format == CAIRO_FORMAT_RGB24) {
        colorSpace = ColorSpace::DeviceRGB;
        bitsPerComponent = 8;
    } else {
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Invalid or unsupported cairo image type %u", (unsigned int)format);
        return nullptr;
    }

    data = g_byte_array_sized_new((guint)((width * 4) + rowstride_c) * height);
    sMaskData = g_byte_array_sized_new((guint)((width * 4) + rowstride_c) * height);

    if (!get_raw_data_from_cairo_image(image, format, width, height, rowstride_c, data, sMaskData)) {
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Failed to get raw data from cairo image");
        g_byte_array_unref(data);
        g_byte_array_unref(sMaskData);
        return nullptr;
    }

    if (sMaskData->len > 0) {
        AnnotStampImageHelper sMask(doc, width, height, ColorSpace::DeviceGray, 8, (char *)sMaskData->data, (int)sMaskData->len);
        annotImg = std::make_unique<AnnotStampImageHelper>(doc, width, height, colorSpace, bitsPerComponent, (char *)data->data, (int)data->len, sMask.getRef());
    } else {
        annotImg = std::make_unique<AnnotStampImageHelper>(doc, width, height, colorSpace, bitsPerComponent, (char *)data->data, (int)data->len);
    }

    g_byte_array_unref(data);
    g_byte_array_unref(sMaskData);

    return annotImg;
}

/* Public methods */
/**
 * poppler_annot_get_annot_type:
 * @poppler_annot: a #PopplerAnnot
 *
 * Gets the type of @poppler_annot
 *
 * Return value: #PopplerAnnotType of @poppler_annot.
 **/
PopplerAnnotType poppler_annot_get_annot_type(PopplerAnnot *poppler_annot)
{
    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), POPPLER_ANNOT_UNKNOWN);

    switch (poppler_annot->annot->getType()) {
    case Annot::typeText:
        return POPPLER_ANNOT_TEXT;
    case Annot::typeLink:
        return POPPLER_ANNOT_LINK;
    case Annot::typeFreeText:
        return POPPLER_ANNOT_FREE_TEXT;
    case Annot::typeLine:
        return POPPLER_ANNOT_LINE;
    case Annot::typeSquare:
        return POPPLER_ANNOT_SQUARE;
    case Annot::typeCircle:
        return POPPLER_ANNOT_CIRCLE;
    case Annot::typePolygon:
        return POPPLER_ANNOT_POLYGON;
    case Annot::typePolyLine:
        return POPPLER_ANNOT_POLY_LINE;
    case Annot::typeHighlight:
        return POPPLER_ANNOT_HIGHLIGHT;
    case Annot::typeUnderline:
        return POPPLER_ANNOT_UNDERLINE;
    case Annot::typeSquiggly:
        return POPPLER_ANNOT_SQUIGGLY;
    case Annot::typeStrikeOut:
        return POPPLER_ANNOT_STRIKE_OUT;
    case Annot::typeStamp:
        return POPPLER_ANNOT_STAMP;
    case Annot::typeCaret:
        return POPPLER_ANNOT_CARET;
    case Annot::typeInk:
        return POPPLER_ANNOT_INK;
    case Annot::typePopup:
        return POPPLER_ANNOT_POPUP;
    case Annot::typeFileAttachment:
        return POPPLER_ANNOT_FILE_ATTACHMENT;
    case Annot::typeSound:
        return POPPLER_ANNOT_SOUND;
    case Annot::typeMovie:
        return POPPLER_ANNOT_MOVIE;
    case Annot::typeWidget:
        return POPPLER_ANNOT_WIDGET;
    case Annot::typeScreen:
        return POPPLER_ANNOT_SCREEN;
    case Annot::typePrinterMark:
        return POPPLER_ANNOT_PRINTER_MARK;
    case Annot::typeTrapNet:
        return POPPLER_ANNOT_TRAP_NET;
    case Annot::typeWatermark:
        return POPPLER_ANNOT_WATERMARK;
    case Annot::type3D:
        return POPPLER_ANNOT_3D;
    default:
        g_warning("Unsupported Annot Type");
    }

    return POPPLER_ANNOT_UNKNOWN;
}

/**
 * poppler_annot_get_contents:
 * @poppler_annot: a #PopplerAnnot
 *
 * Retrieves the contents of @poppler_annot.
 *
 * Return value: a new allocated string with the contents of @poppler_annot. It
 *               must be freed with g_free() when done.
 **/
gchar *poppler_annot_get_contents(PopplerAnnot *poppler_annot)
{
    const GooString *contents;

    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), NULL);

    contents = poppler_annot->annot->getContents();

    return contents && !contents->empty() ? _poppler_goo_string_to_utf8(contents) : nullptr;
}

/**
 * poppler_annot_set_contents:
 * @poppler_annot: a #PopplerAnnot
 * @contents: a text string containing the new contents
 *
 * Sets the contents of @poppler_annot to the given value,
 * replacing the current contents.
 *
 * Since: 0.12
 **/
void poppler_annot_set_contents(PopplerAnnot *poppler_annot, const gchar *contents)
{
    gchar *tmp;
    gsize length = 0;

    g_return_if_fail(POPPLER_IS_ANNOT(poppler_annot));

    tmp = contents ? g_convert(contents, -1, "UTF-16BE", "UTF-8", nullptr, &length, nullptr) : nullptr;
    poppler_annot->annot->setContents(std::make_unique<GooString>(tmp, length));
    g_free(tmp);
}

/**
 * poppler_annot_get_name:
 * @poppler_annot: a #PopplerAnnot
 *
 * Retrieves the name of @poppler_annot.
 *
 * Return value: a new allocated string with the name of @poppler_annot. It must
 *               be freed with g_free() when done.
 **/
gchar *poppler_annot_get_name(PopplerAnnot *poppler_annot)
{
    const GooString *name;

    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), NULL);

    name = poppler_annot->annot->getName();

    return name ? _poppler_goo_string_to_utf8(name) : nullptr;
}

/**
 * poppler_annot_get_modified:
 * @poppler_annot: a #PopplerAnnot
 *
 * Retrieves the last modification data of @poppler_annot. The returned
 * string will be either a PDF format date or a text string.
 * See also #poppler_date_parse()
 *
 * Return value: a new allocated string with the last modification data of
 *               @poppler_annot. It must be freed with g_free() when done.
 **/
gchar *poppler_annot_get_modified(PopplerAnnot *poppler_annot)
{
    const GooString *text;

    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), NULL);

    text = poppler_annot->annot->getModified();

    return text ? _poppler_goo_string_to_utf8(text) : nullptr;
}

/**
 * poppler_annot_get_flags:
 * @poppler_annot: a #PopplerAnnot
 *
 * Retrieves the flag field specifying various characteristics of the
 * @poppler_annot.
 *
 * Return value: the flag field of @poppler_annot.
 **/
PopplerAnnotFlag poppler_annot_get_flags(PopplerAnnot *poppler_annot)
{
    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), (PopplerAnnotFlag)0);

    return (PopplerAnnotFlag)poppler_annot->annot->getFlags();
}

/**
 * poppler_annot_set_flags:
 * @poppler_annot: a #PopplerAnnot
 * @flags: a #PopplerAnnotFlag
 *
 * Sets the flag field specifying various characteristics of the
 * @poppler_annot.
 *
 * Since: 0.22
 **/
void poppler_annot_set_flags(PopplerAnnot *poppler_annot, PopplerAnnotFlag flags)
{
    g_return_if_fail(POPPLER_IS_ANNOT(poppler_annot));

    if (poppler_annot_get_flags(poppler_annot) == flags) {
        return;
    }

    poppler_annot->annot->setFlags((guint)flags);
}

/**
 * poppler_annot_get_color:
 * @poppler_annot: a #PopplerAnnot
 *
 * Retrieves the color of @poppler_annot.
 *
 * Return value: a new allocated #PopplerColor with the color values of
 *               @poppler_annot, or %NULL. It must be freed with g_free() when done.
 **/
PopplerColor *poppler_annot_get_color(PopplerAnnot *poppler_annot)
{
    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), NULL);

    return _poppler_convert_annot_color_to_poppler_color(poppler_annot->annot->getColor());
}

/**
 * poppler_annot_set_color:
 * @poppler_annot: a #PopplerAnnot
 * @poppler_color: (allow-none): a #PopplerColor, or %NULL
 *
 * Sets the color of @poppler_annot.
 *
 * Since: 0.16
 */
void poppler_annot_set_color(PopplerAnnot *poppler_annot, PopplerColor *poppler_color)
{
    poppler_annot->annot->setColor(_poppler_convert_poppler_color_to_annot_color(poppler_color));
}

/**
 * poppler_annot_get_page_index:
 * @poppler_annot: a #PopplerAnnot
 *
 * Returns the page index to which @poppler_annot is associated, or -1 if unknown
 *
 * Return value: page index or -1
 *
 * Since: 0.14
 **/
gint poppler_annot_get_page_index(PopplerAnnot *poppler_annot)
{
    gint page_num;

    g_return_val_if_fail(POPPLER_IS_ANNOT(poppler_annot), -1);

    page_num = poppler_annot->annot->getPageNum();
    return page_num <= 0 ? -1 : page_num - 1;
}

/* Returns cropbox rect for the page where the passed in @poppler_annot is in,
 * or NULL when could not retrieve the cropbox. If @page_out is non-null then
 * it will be set with the page that @poppler_annot is in. */
const PDFRectangle *_poppler_annot_get_cropbox_and_page(PopplerAnnot *poppler_annot, Page **page_out)
{
    int page_index;

    /* A returned zero means annot is not added to any page yet */
    page_index = poppler_annot->annot->getPageNum();

    if (page_index) {
        Page *page;

        page = poppler_annot->annot->getDoc()->getPage(page_index);
        if (page) {
            if (page_out) {
                *page_out = page;
            }

            return page->getCropBox();
        }
    }

    return nullptr;
}

/* Returns cropbox rect for the page where the passed in @poppler_annot is in,
 * or NULL when could not retrieve the cropbox */
const PDFRectangle *_poppler_annot_get_cropbox(PopplerAnnot *poppler_annot)
{
    return _poppler_annot_get_cropbox_and_page(poppler_annot, nullptr);
}

/**
 * poppler_annot_get_rectangle:
 * @poppler_annot: a #PopplerAnnot
 * @poppler_rect: (out): a #PopplerRectangle to store the annotation's coordinates
 *
 * Retrieves the rectangle representing the page coordinates where the
 * annotation @poppler_annot is placed.
 *
 * Since: 0.26
 */
void poppler_annot_get_rectangle(PopplerAnnot *poppler_annot, PopplerRectangle *poppler_rect)
{
    const PDFRectangle *crop_box;
    PDFRectangle zerobox;
    Page *page = nullptr;

    g_return_if_fail(POPPLER_IS_ANNOT(poppler_annot));
    g_return_if_fail(poppler_rect != nullptr);

    crop_box = _poppler_annot_get_cropbox_and_page(poppler_annot, &page);
    if (!crop_box) {
        zerobox = PDFRectangle();
        crop_box = &zerobox;
    }

    const PDFRectangle &annot_rect = poppler_annot->annot->getRect();
    poppler_rect->x1 = annot_rect.x1 - crop_box->x1;
    poppler_rect->x2 = annot_rect.x2 - crop_box->x1;
    poppler_rect->y1 = annot_rect.y1 - crop_box->y1;
    poppler_rect->y2 = annot_rect.y2 - crop_box->y1;
}

/**
 * poppler_annot_set_rectangle:
 * @poppler_annot: a #PopplerAnnot
 * @poppler_rect: a #PopplerRectangle with the new annotation's coordinates
 *
 * Move the annotation to the rectangle representing the page coordinates
 * where the annotation @poppler_annot should be placed.
 *
 * Since: 0.26
 */
void poppler_annot_set_rectangle(PopplerAnnot *poppler_annot, PopplerRectangle *poppler_rect)
{
    const PDFRectangle *crop_box;
    PDFRectangle zerobox;
    double x1, y1, x2, y2;
    Page *page = nullptr;

    g_return_if_fail(POPPLER_IS_ANNOT(poppler_annot));
    g_return_if_fail(poppler_rect != nullptr);

    crop_box = _poppler_annot_get_cropbox_and_page(poppler_annot, &page);
    if (!crop_box) {
        zerobox = PDFRectangle();
        crop_box = &zerobox;
    }

    x1 = poppler_rect->x1;
    y1 = poppler_rect->y1;
    x2 = poppler_rect->x2;
    y2 = poppler_rect->y2;

    if (page && SUPPORTED_ROTATION(page->getRotate())) {
        /* annot is inside a rotated page, as core poppler rect must be saved
         * un-rotated, let's proceed to un-rotate rect before saving */
        _unrotate_rect_for_annot_and_page(page, poppler_annot->annot.get(), &x1, &y1, &x2, &y2);
    }

    poppler_annot->annot->setRect(x1 + crop_box->x1, y1 + crop_box->y1, x2 + crop_box->x1, y2 + crop_box->y1);
}

/* PopplerAnnotMarkup */
/**
 * poppler_annot_markup_get_label:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Retrieves the label text of @poppler_annot.
 *
 * Return value: the label text of @poppler_annot.
 */
gchar *poppler_annot_markup_get_label(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;
    const GooString *text;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), NULL);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    text = annot->getLabel();

    return text ? _poppler_goo_string_to_utf8(text) : nullptr;
}

/**
 * poppler_annot_markup_set_label:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @label: (allow-none): a text string containing the new label, or %NULL
 *
 * Sets the label text of @poppler_annot, replacing the current one
 *
 * Since: 0.16
 */
void poppler_annot_markup_set_label(PopplerAnnotMarkup *poppler_annot, const gchar *label)
{
    AnnotMarkup *annot;
    gchar *tmp;
    gsize length = 0;

    g_return_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot));

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    tmp = label ? g_convert(label, -1, "UTF-16BE", "UTF-8", nullptr, &length, nullptr) : nullptr;
    annot->setLabel(std::make_unique<GooString>(tmp, length));
    g_free(tmp);
}

/**
 * poppler_annot_markup_has_popup:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Return %TRUE if the markup annotation has a popup window associated
 *
 * Return value: %TRUE, if @poppler_annot has popup, %FALSE otherwise
 *
 * Since: 0.12
 **/
gboolean poppler_annot_markup_has_popup(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), FALSE);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    return annot->getPopup() != nullptr;
}

/**
 * poppler_annot_markup_set_popup:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @popup_rect: a #PopplerRectangle
 *
 * Associates a new popup window for editing contents of @poppler_annot.
 * Popup window shall be displayed by viewers at @popup_rect on the page.
 *
 * Since: 0.16
 */
void poppler_annot_markup_set_popup(PopplerAnnotMarkup *poppler_annot, PopplerRectangle *popup_rect)
{
    AnnotMarkup *annot;
    PDFRectangle pdf_rect(popup_rect->x1, popup_rect->y1, popup_rect->x2, popup_rect->y2);

    g_return_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot));

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot->setPopup(std::make_shared<AnnotPopup>(annot->getDoc(), &pdf_rect));
}

/**
 * poppler_annot_markup_get_popup_is_open:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Retrieves the state of the popup window related to @poppler_annot.
 *
 * Return value: the state of @poppler_annot. %TRUE if it's open, %FALSE in
 *               other case.
 **/
gboolean poppler_annot_markup_get_popup_is_open(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;
    AnnotPopup *annot_popup;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), FALSE);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    if ((annot_popup = annot->getPopup().get())) {
        return annot_popup->getOpen();
    }

    return FALSE;
}

/**
 * poppler_annot_markup_set_popup_is_open:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @is_open: whether popup window should initially be displayed open
 *
 * Sets the state of the popup window related to @poppler_annot.
 *
 * Since: 0.16
 **/
void poppler_annot_markup_set_popup_is_open(PopplerAnnotMarkup *poppler_annot, gboolean is_open)
{
    AnnotMarkup *annot;
    AnnotPopup *annot_popup;

    g_return_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot));

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    annot_popup = annot->getPopup().get();
    if (!annot_popup) {
        return;
    }

    if (annot_popup->getOpen() != is_open) {
        annot_popup->setOpen(is_open);
    }
}

/**
 * poppler_annot_markup_get_popup_rectangle:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @poppler_rect: (out): a #PopplerRectangle to store the popup rectangle
 *
 * Retrieves the rectangle of the popup window related to @poppler_annot.
 *
 * Return value: %TRUE if #PopplerRectangle was correctly filled, %FALSE otherwise
 *
 * Since: 0.12
 **/
gboolean poppler_annot_markup_get_popup_rectangle(PopplerAnnotMarkup *poppler_annot, PopplerRectangle *poppler_rect)
{
    AnnotMarkup *annot;
    Annot *annot_popup;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), FALSE);
    g_return_val_if_fail(poppler_rect != nullptr, FALSE);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot_popup = annot->getPopup().get();
    if (!annot_popup) {
        return FALSE;
    }

    const PDFRectangle &annot_rect = annot_popup->getRect();
    poppler_rect->x1 = annot_rect.x1;
    poppler_rect->x2 = annot_rect.x2;
    poppler_rect->y1 = annot_rect.y1;
    poppler_rect->y2 = annot_rect.y2;

    return TRUE;
}

/**
 * poppler_annot_markup_set_popup_rectangle:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @poppler_rect: a #PopplerRectangle to set
 *
 * Sets the rectangle of the popup window related to @poppler_annot.
 * This doesn't have any effect if @poppler_annot doesn't have a
 * popup associated, use poppler_annot_markup_set_popup() to associate
 * a popup window to a #PopplerAnnotMarkup.
 *
 * Since: 0.33
 */
void poppler_annot_markup_set_popup_rectangle(PopplerAnnotMarkup *poppler_annot, PopplerRectangle *poppler_rect)
{
    AnnotMarkup *annot;
    Annot *annot_popup;

    g_return_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot));
    g_return_if_fail(poppler_rect != nullptr);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot_popup = annot->getPopup().get();
    if (!annot_popup) {
        return;
    }

    annot_popup->setRect(poppler_rect->x1, poppler_rect->y1, poppler_rect->x2, poppler_rect->y2);
}

/**
 * poppler_annot_markup_get_opacity:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Retrieves the opacity value of @poppler_annot.
 *
 * Return value: the opacity value of @poppler_annot,
 *               between 0 (transparent) and 1 (opaque)
 */
gdouble poppler_annot_markup_get_opacity(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), 0);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    return annot->getOpacity();
}

/**
 * poppler_annot_markup_set_opacity:
 * @poppler_annot: a #PopplerAnnotMarkup
 * @opacity: a constant opacity value, between 0 (transparent) and 1 (opaque)
 *
 * Sets the opacity of @poppler_annot. This value applies to
 * all visible elements of @poppler_annot in its closed state,
 * but not to the pop-up window that appears when it's openened
 *
 * Since: 0.16
 */
void poppler_annot_markup_set_opacity(PopplerAnnotMarkup *poppler_annot, gdouble opacity)
{
    AnnotMarkup *annot;

    g_return_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot));

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot->setOpacity(opacity);
}

/**
 * poppler_annot_markup_get_date:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Returns the date and time when the annotation was created
 *
 * Return value: (transfer full): a #GDate representing the date and time
 *               when the annotation was created, or %NULL
 */
GDate *poppler_annot_markup_get_date(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;
    const GooString *annot_date;
    time_t timet;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), NULL);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot_date = annot->getDate();
    if (!annot_date) {
        return nullptr;
    }

    if (_poppler_convert_pdf_date_to_gtime(annot_date, &timet)) {
        GDate *date;

        date = g_date_new();
        g_date_set_time_t(date, timet);

        return date;
    }

    return nullptr;
}

/**
 * poppler_annot_markup_get_subject:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Retrives the subject text of @poppler_annot.
 *
 * Return value: the subject text of @poppler_annot.
 */
gchar *poppler_annot_markup_get_subject(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;
    const GooString *text;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), NULL);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    text = annot->getSubject();

    return text ? _poppler_goo_string_to_utf8(text) : nullptr;
}

/**
 * poppler_annot_markup_get_reply_to:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Gets the reply type of @poppler_annot.
 *
 * Return value: #PopplerAnnotMarkupReplyType of @poppler_annot.
 */
PopplerAnnotMarkupReplyType poppler_annot_markup_get_reply_to(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), POPPLER_ANNOT_MARKUP_REPLY_TYPE_R);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    switch (annot->getReplyTo()) {
    case AnnotMarkup::replyTypeR:
        return POPPLER_ANNOT_MARKUP_REPLY_TYPE_R;
    case AnnotMarkup::replyTypeGroup:
        return POPPLER_ANNOT_MARKUP_REPLY_TYPE_GROUP;
    default:
        g_warning("Unsupported Annot Markup Reply To Type");
    }

    return POPPLER_ANNOT_MARKUP_REPLY_TYPE_R;
}

/**
 * poppler_annot_markup_get_external_data:
 * @poppler_annot: a #PopplerAnnotMarkup
 *
 * Gets the external data type of @poppler_annot.
 *
 * Return value: #PopplerAnnotExternalDataType of @poppler_annot.
 */
PopplerAnnotExternalDataType poppler_annot_markup_get_external_data(PopplerAnnotMarkup *poppler_annot)
{
    AnnotMarkup *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MARKUP(poppler_annot), POPPLER_ANNOT_EXTERNAL_DATA_MARKUP_UNKNOWN);

    annot = static_cast<AnnotMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    switch (annot->getExData()) {
    case annotExternalDataMarkup3D:
        return POPPLER_ANNOT_EXTERNAL_DATA_MARKUP_3D;
    case annotExternalDataMarkupUnknown:
        return POPPLER_ANNOT_EXTERNAL_DATA_MARKUP_UNKNOWN;
    default:
        g_warning("Unsupported Annot Markup External Data");
    }

    return POPPLER_ANNOT_EXTERNAL_DATA_MARKUP_UNKNOWN;
}

/* PopplerAnnotText */
/**
 * poppler_annot_text_get_is_open:
 * @poppler_annot: a #PopplerAnnotText
 *
 * Retrieves the state of @poppler_annot.
 *
 * Return value: the state of @poppler_annot. %TRUE if it's open, %FALSE in
 *               other case.
 **/
gboolean poppler_annot_text_get_is_open(PopplerAnnotText *poppler_annot)
{
    AnnotText *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_TEXT(poppler_annot), FALSE);

    annot = static_cast<AnnotText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    return annot->getOpen();
}

/**
 * poppler_annot_text_set_is_open:
 * @poppler_annot: a #PopplerAnnotText
 * @is_open: whether annotation should initially be displayed open
 *
 * Sets whether @poppler_annot should initially be displayed open
 *
 * Since: 0.16
 */
void poppler_annot_text_set_is_open(PopplerAnnotText *poppler_annot, gboolean is_open)
{
    AnnotText *annot;

    g_return_if_fail(POPPLER_IS_ANNOT_TEXT(poppler_annot));

    annot = static_cast<AnnotText *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot->setOpen(is_open);
}

/**
 * poppler_annot_text_get_icon:
 * @poppler_annot: a #PopplerAnnotText
 *
 * Gets name of the icon of @poppler_annot.
 *
 * Return value: a new allocated string containing the icon name
 */
gchar *poppler_annot_text_get_icon(PopplerAnnotText *poppler_annot)
{
    AnnotText *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_TEXT(poppler_annot), NULL);

    annot = static_cast<AnnotText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    const std::string &text = annot->getIcon();

    return text.empty() ? nullptr : g_strdup(text.c_str());
}

/**
 * poppler_annot_text_set_icon:
 * @poppler_annot: a #PopplerAnnotText
 * @icon: the name of an icon
 *
 * Sets the icon of @poppler_annot. The following predefined
 * icons are currently supported:
 * <variablelist>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_NOTE</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_COMMENT</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_KEY</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_HELP</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_NEW_PARAGRAPH</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_PARAGRAPH</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_INSERT</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_CROSS</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_CIRCLE</term>
 *  </varlistentry>
 * </variablelist>
 *
 * Since 26.1.0, Poppler also knows how to render the following icons,
 * which are somewhat standard and other PDF renderers may also support:
 * <variablelist>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_CHECK</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_STAR</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_RIGHT_ARROW</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_RIGHT_POINTER</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_UP_ARROW</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_UP_LEFT_ARROW</term>
 *  </varlistentry>
 *  <varlistentry>
 *   <term>#POPPLER_ANNOT_TEXT_ICON_CROSS_HAIRS</term>
 *  </varlistentry>
 * </variablelist>
 *
 * Since: 0.16
 */
void poppler_annot_text_set_icon(PopplerAnnotText *poppler_annot, const gchar *icon)
{
    AnnotText *annot;

    g_return_if_fail(POPPLER_IS_ANNOT_TEXT(poppler_annot));

    annot = static_cast<AnnotText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    std::string text = icon ? std::string { icon } : std::string {};
    annot->setIcon(text);
}

/**
 * poppler_annot_text_get_state:
 * @poppler_annot: a #PopplerAnnotText
 *
 * Retrieves the state of @poppler_annot.
 *
 * Return value: #PopplerAnnotTextState of @poppler_annot.
 **/
PopplerAnnotTextState poppler_annot_text_get_state(PopplerAnnotText *poppler_annot)
{
    AnnotText *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_TEXT(poppler_annot), POPPLER_ANNOT_TEXT_STATE_UNKNOWN);

    annot = static_cast<AnnotText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    switch (annot->getState()) {
    case AnnotText::stateUnknown:
        return POPPLER_ANNOT_TEXT_STATE_UNKNOWN;
    case AnnotText::stateMarked:
        return POPPLER_ANNOT_TEXT_STATE_MARKED;
    case AnnotText::stateUnmarked:
        return POPPLER_ANNOT_TEXT_STATE_UNMARKED;
    case AnnotText::stateAccepted:
        return POPPLER_ANNOT_TEXT_STATE_ACCEPTED;
    case AnnotText::stateRejected:
        return POPPLER_ANNOT_TEXT_STATE_REJECTED;
    case AnnotText::stateCancelled:
        return POPPLER_ANNOT_TEXT_STATE_CANCELLED;
    case AnnotText::stateCompleted:
        return POPPLER_ANNOT_TEXT_STATE_COMPLETED;
    case AnnotText::stateNone:
        return POPPLER_ANNOT_TEXT_STATE_NONE;
    default:
        g_warning("Unsupported Annot Text State");
    }

    return POPPLER_ANNOT_TEXT_STATE_UNKNOWN;
}

/* PopplerAnnotTextMarkup */
/**
 * poppler_annot_text_markup_set_quadrilaterals:
 * @poppler_annot: A #PopplerAnnotTextMarkup
 * @quadrilaterals: (element-type PopplerQuadrilateral): A #GArray of
 *   #PopplerQuadrilateral<!-- -->s
 *
 * Set the regions (Quadrilaterals) to apply the text markup in @poppler_annot.
 *
 * Since: 0.26
 **/
void poppler_annot_text_markup_set_quadrilaterals(PopplerAnnotTextMarkup *poppler_annot, GArray *quadrilaterals)
{
    AnnotQuadrilaterals *quads, *quads_temp;
    AnnotTextMarkup *annot;
    const PDFRectangle *crop_box;
    Page *page = nullptr;

    g_return_if_fail(POPPLER_IS_ANNOT_TEXT_MARKUP(poppler_annot));
    g_return_if_fail(quadrilaterals != nullptr && quadrilaterals->len > 0);

    annot = static_cast<AnnotTextMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    crop_box = _poppler_annot_get_cropbox_and_page(POPPLER_ANNOT(poppler_annot), &page);
    quads = create_annot_quads_from_poppler_quads(quadrilaterals);

    if (page && SUPPORTED_ROTATION(page->getRotate())) {
        quads_temp = _page_new_quads_unrotated(page, quads);
        delete quads;
        quads = quads_temp;
    }

    if (!ZERO_CROPBOX(crop_box)) {
        quads_temp = quads;
        quads = new_quads_from_offset_cropbox(crop_box, quads, TRUE);
        delete quads_temp;
    }

    annot->setQuadrilaterals(*quads);
    delete quads;
}

/**
 * poppler_annot_text_markup_get_quadrilaterals:
 * @poppler_annot: A #PopplerAnnotTextMarkup
 *
 * Returns a #GArray of #PopplerQuadrilateral items that map from a
 * location on @page to a #PopplerAnnotTextMarkup.  This array must be freed
 * when done.
 *
 * Return value: (element-type PopplerQuadrilateral) (transfer full): A #GArray of #PopplerQuadrilateral
 *
 * Since: 0.26
 **/
GArray *poppler_annot_text_markup_get_quadrilaterals(PopplerAnnotTextMarkup *poppler_annot)
{
    const PDFRectangle *crop_box;
    AnnotTextMarkup *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_TEXT_MARKUP(poppler_annot), NULL);

    annot = static_cast<AnnotTextMarkup *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    crop_box = _poppler_annot_get_cropbox(POPPLER_ANNOT(poppler_annot));
    AnnotQuadrilaterals *quads = annot->getQuadrilaterals();

    return create_poppler_quads_from_annot_quads(quads, crop_box);
}

/* PopplerAnnotFreeText */
/**
 * poppler_annot_free_text_get_quadding:
 * @poppler_annot: a #PopplerAnnotFreeText
 *
 * Retrieves the justification of the text of @poppler_annot.
 *
 * Return value: #PopplerAnnotFreeTextQuadding of @poppler_annot.
 **/
PopplerAnnotFreeTextQuadding poppler_annot_free_text_get_quadding(PopplerAnnotFreeText *poppler_annot)
{
    AnnotFreeText *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_FREE_TEXT(poppler_annot), POPPLER_ANNOT_FREE_TEXT_QUADDING_LEFT_JUSTIFIED);

    annot = static_cast<AnnotFreeText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    switch (annot->getQuadding()) {
    case VariableTextQuadding::leftJustified:
        return POPPLER_ANNOT_FREE_TEXT_QUADDING_LEFT_JUSTIFIED;
    case VariableTextQuadding::centered:
        return POPPLER_ANNOT_FREE_TEXT_QUADDING_CENTERED;
    case VariableTextQuadding::rightJustified:
        return POPPLER_ANNOT_FREE_TEXT_QUADDING_RIGHT_JUSTIFIED;
    default:
        g_warning("Unsupported Annot Free Text Quadding");
    }

    return POPPLER_ANNOT_FREE_TEXT_QUADDING_LEFT_JUSTIFIED;
}

/**
 * poppler_annot_free_text_get_callout_line:
 * @poppler_annot: a #PopplerAnnotFreeText
 *
 * Retrieves a #PopplerAnnotCalloutLine of four or six numbers specifying a callout
 * line attached to the @poppler_annot.
 *
 * Return value: a new allocated #PopplerAnnotCalloutLine if the annot has a callout
 *               line, %NULL in other case. It must be freed with g_free() when
 *               done.
 **/
PopplerAnnotCalloutLine *poppler_annot_free_text_get_callout_line(PopplerAnnotFreeText *poppler_annot)
{
    AnnotFreeText *annot;
    AnnotCalloutLine *line;

    g_return_val_if_fail(POPPLER_IS_ANNOT_FREE_TEXT(poppler_annot), NULL);

    annot = static_cast<AnnotFreeText *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    if ((line = annot->getCalloutLine())) {
        AnnotCalloutMultiLine *multiline;
        auto *callout = g_new0(PopplerAnnotCalloutLine, 1);

        callout->x1 = line->getX1();
        callout->y1 = line->getY1();
        callout->x2 = line->getX2();
        callout->y2 = line->getY2();

        if ((multiline = dynamic_cast<AnnotCalloutMultiLine *>(line))) {
            callout->multiline = TRUE;
            callout->x3 = multiline->getX3();
            callout->y3 = multiline->getY3();
            return callout;
        }

        callout->multiline = FALSE;
        return callout;
    }

    return nullptr;
}

static std::string poppler_font_description_to_style(PopplerFontDescription *font_desc)
{
    std::string style;
    std::function<void(const char *)> add_style = [&style](const char *a) {
        if (strcmp(a, "") != 0) {
            if (!style.empty()) {
                style += " ";
            }
            style += a;
        }
    };

    /* Stretch */
    add_style(stretch_to_str[font_desc->stretch]);

    /* Don't use a map so that intermediate pango weights are correctly mapped */
    PopplerWeight w = font_desc->weight;
    if (w <= POPPLER_WEIGHT_THIN) {
        add_style("Thin");
    } else if (w <= POPPLER_WEIGHT_ULTRALIGHT) {
        add_style("UltraLight");
    } else if (w <= POPPLER_WEIGHT_LIGHT) {
        add_style("Light");
    } else if (w <= POPPLER_WEIGHT_NORMAL) {
        add_style("");
    } else if (w <= POPPLER_WEIGHT_MEDIUM) {
        add_style("Medium");
    } else if (w <= POPPLER_WEIGHT_SEMIBOLD) {
        add_style("SemiBold");
    } else if (w <= POPPLER_WEIGHT_BOLD) {
        add_style("Bold");
    } else if (w <= POPPLER_WEIGHT_ULTRABOLD) {
        add_style("UltraBold");
    } else {
        add_style("Heavy");
    }

    /* Style, i.e. italic, oblique or normal */
    if (font_desc->style == POPPLER_STYLE_ITALIC) {
        add_style("Italic");
    } else if (font_desc->style == POPPLER_STYLE_OBLIQUE) {
        add_style("Oblique");
    }

    return style;
}

static void poppler_annot_free_text_set_da_to_native(PopplerAnnotFreeText *poppler_annot)
{
    Annot *annot = POPPLER_ANNOT(poppler_annot)->annot.get();

    std::string font_name = "Sans";
    double size = 11.;

    if (poppler_annot->font_desc) {
        const char *family = poppler_annot->font_desc->font_name;
        const std::string style = poppler_font_description_to_style(poppler_annot->font_desc);

        Form *form = annot->getDoc()->getCatalog()->getCreateForm();
        if (form) {
            font_name = form->findFontInDefaultResources(family, style);
            if (font_name.empty()) {
                font_name = form->addFontToDefaultResources(family, style).fontName;
            }

            if (!font_name.empty()) {
                form->ensureFontsForAllCharacters(annot->getContents(), font_name);
            }
        }
        size = poppler_annot->font_desc->size_pt;
    }

    DefaultAppearance da { font_name, size, _poppler_convert_poppler_color_to_annot_color(&(poppler_annot->font_color)) };
    ((AnnotFreeText *)annot)->setDefaultAppearance(da);
}

/**
 * poppler_annot_free_text_set_font_desc:
 * @poppler_annot: a #PopplerAnnotFreeText
 * @font_desc: a #PopplerFontDescription
 *
 * Sets the font description (i.e. font family name, style, weight, stretch and size).
 *
 * Since: 24.12.0
 **/
void poppler_annot_free_text_set_font_desc(PopplerAnnotFreeText *poppler_annot, PopplerFontDescription *font_desc)
{
    if (poppler_annot->font_desc) {
        poppler_font_description_free(poppler_annot->font_desc);
    }
    poppler_annot->font_desc = poppler_font_description_copy(font_desc);
    poppler_annot_free_text_set_da_to_native(poppler_annot);
}

/**
 * poppler_annot_free_text_get_font_desc:
 * @poppler_annot: a #PopplerAnnotFreeText
 *
 * Gets the font description (i.e. font family name, style, weight, stretch and size).
 *
 * Returns: (nullable) (transfer full): a copy of the annotation font description, or NULL if there is
 * no font description set.
 *
 * Since: 24.12.0
 **/
PopplerFontDescription *poppler_annot_free_text_get_font_desc(PopplerAnnotFreeText *poppler_annot)
{
    if (poppler_annot->font_desc) {
        return poppler_font_description_copy(poppler_annot->font_desc);
    }
    return nullptr;
}

/**
 * poppler_annot_free_text_set_font_color:
 * @poppler_annot: a #PopplerAnnotFreeText
 * @color: a #PopplerColor
 *
 * Sets the font color.
 *
 * Since: 24.12.0
 **/
void poppler_annot_free_text_set_font_color(PopplerAnnotFreeText *poppler_annot, PopplerColor *color)
{
    poppler_annot->font_color = *color;
    poppler_annot_free_text_set_da_to_native(poppler_annot);
}

/**
 * poppler_annot_free_text_get_font_color:
 * @poppler_annot: a #PopplerAnnotFreeText
 *
 * Gets the font color.
 *
 * Returns: (transfer full): a copy of the font's #PopplerColor.
 *
 * Since: 24.12.0
 **/
PopplerColor *poppler_annot_free_text_get_font_color(PopplerAnnotFreeText *poppler_annot)
{
    auto *color = g_new(PopplerColor, 1);
    *color = poppler_annot->font_color;
    return color;
}

/* PopplerAnnotFileAttachment */
/**
 * poppler_annot_file_attachment_get_attachment:
 * @poppler_annot: a #PopplerAnnotFileAttachment
 *
 * Creates a #PopplerAttachment for the file of the file attachment annotation @annot.
 * The #PopplerAttachment must be unrefed with g_object_unref by the caller.
 *
 * Return value: (transfer full): @PopplerAttachment
 *
 * Since: 0.14
 **/
PopplerAttachment *poppler_annot_file_attachment_get_attachment(PopplerAnnotFileAttachment *poppler_annot)
{
    AnnotFileAttachment *annot;
    PopplerAttachment *attachment;

    g_return_val_if_fail(POPPLER_IS_ANNOT_FILE_ATTACHMENT(poppler_annot), NULL);

    annot = static_cast<AnnotFileAttachment *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    FileSpec file { annot->getFile() };
    attachment = _poppler_attachment_new(&file);

    return attachment;
}

/**
 * poppler_annot_file_attachment_get_name:
 * @poppler_annot: a #PopplerAnnotFileAttachment
 *
 * Retrieves the name of @poppler_annot.
 *
 * Return value: a new allocated string with the name of @poppler_annot. It must
 *               be freed with g_free() when done.
 * Since: 0.14
 **/
gchar *poppler_annot_file_attachment_get_name(PopplerAnnotFileAttachment *poppler_annot)
{
    AnnotFileAttachment *annot;
    const GooString *name;

    g_return_val_if_fail(POPPLER_IS_ANNOT_FILE_ATTACHMENT(poppler_annot), NULL);

    annot = static_cast<AnnotFileAttachment *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    name = annot->getName();

    return name ? _poppler_goo_string_to_utf8(name) : nullptr;
}

/* PopplerAnnotCalloutLine */
G_DEFINE_BOXED_TYPE(PopplerAnnotCalloutLine, poppler_annot_callout_line, poppler_annot_callout_line_copy, poppler_annot_callout_line_free)

/**
 * poppler_annot_callout_line_new:
 *
 * Creates a new empty #PopplerAnnotCalloutLine.
 *
 * Return value: a new allocated #PopplerAnnotCalloutLine, %NULL in other case.
 *               It must be freed when done.
 **/
PopplerAnnotCalloutLine *poppler_annot_callout_line_new(void)
{
    return g_new0(PopplerAnnotCalloutLine, 1);
}

/**
 * poppler_annot_callout_line_copy:
 * @callout: the #PopplerAnnotCalloutLine to be copied.
 *
 * It does copy @callout to a new #PopplerAnnotCalloutLine.
 *
 * Return value: a new allocated #PopplerAnnotCalloutLine as exact copy of
 *               @callout, %NULL in other case. It must be freed when done.
 **/
PopplerAnnotCalloutLine *poppler_annot_callout_line_copy(PopplerAnnotCalloutLine *callout)
{
    PopplerAnnotCalloutLine *new_callout;

    g_return_val_if_fail(callout != nullptr, NULL);

    new_callout = g_new0(PopplerAnnotCalloutLine, 1);
    *new_callout = *callout;

    return new_callout;
}

/**
 * poppler_annot_callout_line_free:
 * @callout: a #PopplerAnnotCalloutLine
 *
 * Frees the memory used by #PopplerAnnotCalloutLine.
 **/
void poppler_annot_callout_line_free(PopplerAnnotCalloutLine *callout)
{
    g_free(callout);
}

/* PopplerAnnotMovie */
/**
 * poppler_annot_movie_get_title:
 * @poppler_annot: a #PopplerAnnotMovie
 *
 * Retrieves the movie title of @poppler_annot.
 *
 * Return value: the title text of @poppler_annot.
 *
 * Since: 0.14
 */
gchar *poppler_annot_movie_get_title(PopplerAnnotMovie *poppler_annot)
{
    AnnotMovie *annot;
    const GooString *title;

    g_return_val_if_fail(POPPLER_IS_ANNOT_MOVIE(poppler_annot), NULL);

    annot = static_cast<AnnotMovie *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    title = annot->getTitle();

    return title ? _poppler_goo_string_to_utf8(title) : nullptr;
}

/**
 * poppler_annot_movie_get_movie:
 * @poppler_annot: a #PopplerAnnotMovie
 *
 * Retrieves the movie object (PopplerMovie) stored in the @poppler_annot.
 *
 * Return value: (transfer none): the movie object stored in the @poppler_annot. The returned
 *               object is owned by #PopplerAnnotMovie and should not be freed
 *
 * Since: 0.14
 */
PopplerMovie *poppler_annot_movie_get_movie(PopplerAnnotMovie *poppler_annot)
{
    return poppler_annot->movie;
}

/* PopplerAnnotScreen */
/**
 * poppler_annot_screen_get_action:
 * @poppler_annot: a #PopplerAnnotScreen
 *
 * Retrieves the action (#PopplerAction) that shall be performed when @poppler_annot is activated
 *
 * Return value: (transfer none): the action to perform. The returned
 *               object is owned by @poppler_annot and should not be freed
 *
 * Since: 0.14
 */
PopplerAction *poppler_annot_screen_get_action(PopplerAnnotScreen *poppler_annot)
{
    return poppler_annot->action;
}

/* PopplerAnnotLine */
/**
 * poppler_annot_line_set_vertices:
 * @poppler_annot: a #PopplerAnnotLine
 * @start: a #PopplerPoint of the starting vertice
 * @end: a #PopplerPoint of the ending vertice
 *
 * Set the coordinate points where the @poppler_annot starts and ends.
 *
 * Since: 0.26
 */
void poppler_annot_line_set_vertices(PopplerAnnotLine *poppler_annot, PopplerPoint *start, PopplerPoint *end)
{
    AnnotLine *annot;

    g_return_if_fail(POPPLER_IS_ANNOT_LINE(poppler_annot));
    g_return_if_fail(start != nullptr);
    g_return_if_fail(end != nullptr);

    annot = static_cast<AnnotLine *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    annot->setVertices(start->x, start->y, end->x, end->y);
}

/* PopplerAnnotCircle and PopplerAnnotSquare helpers */
static PopplerColor *poppler_annot_geometry_get_interior_color(PopplerAnnot *poppler_annot)
{
    AnnotGeometry *annot;

    annot = static_cast<AnnotGeometry *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    return _poppler_convert_annot_color_to_poppler_color(annot->getInteriorColor());
}

static void poppler_annot_geometry_set_interior_color(PopplerAnnot *poppler_annot, PopplerColor *poppler_color)
{
    AnnotGeometry *annot;

    annot = static_cast<AnnotGeometry *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    annot->setInteriorColor(_poppler_convert_poppler_color_to_annot_color(poppler_color));
}

/* PopplerAnnotCircle */
/**
 * poppler_annot_circle_get_interior_color:
 * @poppler_annot: a #PopplerAnnotCircle
 *
 * Retrieves the interior color of @poppler_annot.
 *
 * Return value: a new allocated #PopplerColor with the color values of
 *               @poppler_annot, or %NULL. It must be freed with g_free() when done.
 *
 * Since: 0.26
 */
PopplerColor *poppler_annot_circle_get_interior_color(PopplerAnnotCircle *poppler_annot)
{
    g_return_val_if_fail(POPPLER_IS_ANNOT_CIRCLE(poppler_annot), NULL);

    return poppler_annot_geometry_get_interior_color(POPPLER_ANNOT(poppler_annot));
}

/**
 * poppler_annot_circle_set_interior_color:
 * @poppler_annot: a #PopplerAnnotCircle
 * @poppler_color: (allow-none): a #PopplerColor, or %NULL
 *
 * Sets the interior color of @poppler_annot.
 *
 * Since: 0.26
 */
void poppler_annot_circle_set_interior_color(PopplerAnnotCircle *poppler_annot, PopplerColor *poppler_color)
{
    g_return_if_fail(POPPLER_IS_ANNOT_CIRCLE(poppler_annot));

    poppler_annot_geometry_set_interior_color(POPPLER_ANNOT(poppler_annot), poppler_color);
}

/* PopplerAnnotSquare */
/**
 * poppler_annot_square_get_interior_color:
 * @poppler_annot: a #PopplerAnnotSquare
 *
 * Retrieves the interior color of @poppler_annot.
 *
 * Return value: a new allocated #PopplerColor with the color values of
 *               @poppler_annot, or %NULL. It must be freed with g_free() when done.
 *
 * Since: 0.26
 */
PopplerColor *poppler_annot_square_get_interior_color(PopplerAnnotSquare *poppler_annot)
{
    g_return_val_if_fail(POPPLER_IS_ANNOT_SQUARE(poppler_annot), NULL);

    return poppler_annot_geometry_get_interior_color(POPPLER_ANNOT(poppler_annot));
}

/**
 * poppler_annot_square_set_interior_color:
 * @poppler_annot: a #PopplerAnnotSquare
 * @poppler_color: (allow-none): a #PopplerColor, or %NULL
 *
 * Sets the interior color of @poppler_annot.
 *
 * Since: 0.26
 */
void poppler_annot_square_set_interior_color(PopplerAnnotSquare *poppler_annot, PopplerColor *poppler_color)
{
    g_return_if_fail(POPPLER_IS_ANNOT_SQUARE(poppler_annot));

    poppler_annot_geometry_set_interior_color(POPPLER_ANNOT(poppler_annot), poppler_color);
}

/**
 * poppler_annot_stamp_get_icon:
 * @poppler_annot: a #PopplerAnnotStamp
 *
 * Return value: the corresponding #PopplerAnnotStampIcon of the icon
 *
 * Since: 22.07.0
 */
PopplerAnnotStampIcon poppler_annot_stamp_get_icon(PopplerAnnotStamp *poppler_annot)
{
    AnnotStamp *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_STAMP(poppler_annot), POPPLER_ANNOT_STAMP_ICON_UNKNOWN);

    annot = static_cast<AnnotStamp *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    std::string text = annot->getIcon();

    if (text.empty()) {
        return POPPLER_ANNOT_STAMP_ICON_NONE;
    }

    if (text == "Approved") {
        return POPPLER_ANNOT_STAMP_ICON_APPROVED;
    }
    if (text == "AsIs") {
        return POPPLER_ANNOT_STAMP_ICON_AS_IS;
    }
    if (text == "Confidential") {
        return POPPLER_ANNOT_STAMP_ICON_CONFIDENTIAL;
    }
    if (text == "Final") {
        return POPPLER_ANNOT_STAMP_ICON_FINAL;
    }
    if (text == "Experimental") {
        return POPPLER_ANNOT_STAMP_ICON_EXPERIMENTAL;
    }
    if (text == "Expired") {
        return POPPLER_ANNOT_STAMP_ICON_EXPIRED;
    }
    if (text == "NotApproved") {
        return POPPLER_ANNOT_STAMP_ICON_NOT_APPROVED;
    }
    if (text == "NotForPublicRelease") {
        return POPPLER_ANNOT_STAMP_ICON_NOT_FOR_PUBLIC_RELEASE;
    }
    if (text == "Sold") {
        return POPPLER_ANNOT_STAMP_ICON_SOLD;
    }
    if (text == "Departmental") {
        return POPPLER_ANNOT_STAMP_ICON_DEPARTMENTAL;
    }
    if (text == "ForComment") {
        return POPPLER_ANNOT_STAMP_ICON_FOR_COMMENT;
    }
    if (text == "ForPublicRelease") {
        return POPPLER_ANNOT_STAMP_ICON_FOR_PUBLIC_RELEASE;
    }
    if (text == "TopSecret") {
        return POPPLER_ANNOT_STAMP_ICON_TOP_SECRET;
    }

    return POPPLER_ANNOT_STAMP_ICON_UNKNOWN;
}

/**
 * poppler_annot_stamp_set_icon:
 * @poppler_annot: a #PopplerAnnotStamp
 * @icon: the #PopplerAnnotStampIcon type of the icon
 *
 * Sets the icon of @poppler_annot to be one of the predefined values in #PopplerAnnotStampIcon
 *
 * Since: 22.07.0
 */
void poppler_annot_stamp_set_icon(PopplerAnnotStamp *poppler_annot, PopplerAnnotStampIcon icon)
{
    AnnotStamp *annot;
    const gchar *text;

    g_return_if_fail(POPPLER_IS_ANNOT_STAMP(poppler_annot));

    annot = static_cast<AnnotStamp *>(POPPLER_ANNOT(poppler_annot)->annot.get());

    if (icon == POPPLER_ANNOT_STAMP_ICON_NONE) {
        annot->setIcon(std::string {});
        return;
    }

    if (icon == POPPLER_ANNOT_STAMP_ICON_APPROVED) {
        text = "Approved";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_AS_IS) {
        text = "AsIs";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_CONFIDENTIAL) {
        text = "Confidential";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_FINAL) {
        text = "Final";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_EXPERIMENTAL) {
        text = "Experimental";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_EXPIRED) {
        text = "Expired";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_NOT_APPROVED) {
        text = "NotApproved";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_NOT_FOR_PUBLIC_RELEASE) {
        text = "NotForPublicRelease";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_SOLD) {
        text = "Sold";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_DEPARTMENTAL) {
        text = "Departmental";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_FOR_COMMENT) {
        text = "ForComment";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_FOR_PUBLIC_RELEASE) {
        text = "ForPublicRelease";
    } else if (icon == POPPLER_ANNOT_STAMP_ICON_TOP_SECRET) {
        text = "TopSecret";
    } else {
        return; /* POPPLER_ANNOT_STAMP_ICON_UNKNOWN */
    }

    annot->setIcon(std::string { text });
}

/**
 * poppler_annot_stamp_set_custom_image:
 * @poppler_annot: a #PopplerAnnotStamp
 * @image: an image cairo surface
 * @error: (nullable): return location for error, or %NULL.
 *
 * Sets the custom image of @poppler_annot to be @image
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: 22.07.0
 */
gboolean poppler_annot_stamp_set_custom_image(PopplerAnnotStamp *poppler_annot, cairo_surface_t *image, GError **error)
{
    AnnotStamp *annot;

    g_return_val_if_fail(POPPLER_IS_ANNOT_STAMP(poppler_annot), FALSE);

    annot = static_cast<AnnotStamp *>(POPPLER_ANNOT(poppler_annot)->annot.get());
    std::unique_ptr<AnnotStampImageHelper> annot_image_helper = _poppler_convert_cairo_image_to_stamp_image_helper(image, annot->getDoc(), error);
    if (!annot_image_helper) {
        return FALSE;
    }
    annot->setCustomImage(std::move(annot_image_helper));

    return TRUE;
}

/**
 * poppler_annot_get_border_width:
 * @poppler_annot: a #PopplerAnnot
 * @width: a valid pointer to a double
 *
 * Returns the border width of the annotation. Some PDF editors set a border
 * width even if the border is not actually drawn.
 *
 * Returns: true and sets @border_width to the actual border width if a border
 * is defined, otherwise returns false and sets @border_width to 0.
 *
 * Since: 24.12.0
 */
gboolean poppler_annot_get_border_width(PopplerAnnot *poppler_annot, double *width)
{
    AnnotBorder *b = poppler_annot->annot->getBorder();
    if (b) {
        *width = b->getWidth();
        return TRUE;
    }
    *width = 0.;
    return FALSE;
}
/**
 * poppler_annot_set_border_width:
 * @poppler_annot: a #PopplerAnnot
 * @width: the new border width
 *
 * Sets the border width of the annotation. Since there is currently no
 * mechanism in the GLib binding to control the appearance of the border width,
 * this should generally only be used to disable the border, although the
 * API might be completed in the future.
 *
 * Since: 24.12.0
 */
void poppler_annot_set_border_width(PopplerAnnot *poppler_annot, double width)
{
    std::unique_ptr<AnnotBorderArray> border = std::make_unique<AnnotBorderArray>();
    border->setWidth(width);
    poppler_annot->annot->setBorder(std::move(border));
}

/**
 * SECTION:poppler-font-description
 * @short_description: FontDescription
 * @title: PopplerFontDescription
 */

/* PopplerFontDescription type */
G_DEFINE_BOXED_TYPE(PopplerFontDescription, poppler_font_description, poppler_font_description_copy, poppler_font_description_free)

/**
 * poppler_font_description_new:
 * @font_name: the family name of the font
 *
 * Creates a new #PopplerFontDescriptions
 *
 * Returns: a new #PopplerFontDescription, use poppler_font_description_free() to free it
 */
PopplerFontDescription *poppler_font_description_new(const char *font_name)
{
    auto *font_desc = (PopplerFontDescription *)g_new0(PopplerFontDescription, 1);
    font_desc->font_name = g_strdup(font_name);
    font_desc->size_pt = 11.;
    font_desc->stretch = POPPLER_STRETCH_NORMAL;
    font_desc->style = POPPLER_STYLE_NORMAL;
    font_desc->weight = POPPLER_WEIGHT_NORMAL;
    return font_desc;
}

/**
 * poppler_font_description_free:
 * @font_desc: a #PopplerFontDescription
 *
 * Frees the given #PopplerFontDescription
 */
void poppler_font_description_free(PopplerFontDescription *font_desc)
{
    g_free(font_desc->font_name);
    g_free(font_desc);
}

/**
 * poppler_font_description_copy:
 * @font_desc: a #PopplerFontDescription to copy
 *
 * Creates a copy of @font_desc
 *
 * Returns: a new allocated copy of @font_desc
 */
PopplerFontDescription *poppler_font_description_copy(PopplerFontDescription *font_desc)
{
    PopplerFontDescription *new_font_desc;

    new_font_desc = g_new(PopplerFontDescription, 1);
    *new_font_desc = *font_desc;

    new_font_desc->font_name = g_strdup(font_desc->font_name);

    return new_font_desc;
}

/**
 * SECTION:poppler-path
 * @short_description: Path
 * @title: PopplerPath
 */
G_DEFINE_BOXED_TYPE(PopplerPath, poppler_path, poppler_path_copy, poppler_path_free)

/**
 * poppler_path_new_from_list:
 * @points: (element-type PopplerPoint) (array length=n_points) (transfer full): a #GSList of #PopplerPoint
 *
 * Creates a new #PopplerPath from a list of points.
 *
 * Returns: a new #PopplerPath containing the given points.
 *
 * Since: 25.06.0
 */
PopplerPath *poppler_path_new_from_array(PopplerPoint *points, gsize n_points)
{
    auto *path = (PopplerPath *)g_new(PopplerPath, 1);
    path->points = points;
    path->n_points = n_points;
    return path;
}

/**
 * poppler_path_free:
 * @path: a #PopplerPath
 *
 * Frees the given #PopplerPath.
 *
 * Since: 25.06.0
 */
void poppler_path_free(PopplerPath *path)
{
    g_free(path->points);
    g_free(path);
}

/**
 * poppler_path_copy:
 * @path: a #PopplerPath to copy
 *
 * Creates a copy of @path.
 *
 * Returns: a new allocated copy of @path
 *
 * Since: 25.06.0
 */
PopplerPath *poppler_path_copy(PopplerPath *path)
{
    PopplerPath *new_path;

    new_path = g_new(PopplerPath, 1);
    new_path->points = g_new(PopplerPoint, path->n_points);
    new_path->n_points = path->n_points;
    memcpy(new_path->points, path->points, sizeof(PopplerPoint) * path->n_points);

    return new_path;
}

/**
 * poppler_path_get_points:
 * @path: a #PopplerPath
 * @n_points: to store the length of the returned path
 *
 * Returns the array of points of @path.
 *
 * Returns: (array length=n_points) (transfer none): all the points of @path
 *
 * Since: 25.06.0
 */
PopplerPoint *poppler_path_get_points(PopplerPath *path, gsize *n_points)
{
    *n_points = path->n_points;
    return path->points;
}

static void poppler_annot_ink_class_init(PopplerAnnotInkClass * /*klass*/) { }

static void poppler_annot_ink_init(PopplerAnnotInk * /*poppler_annot*/) { }

/**
 * poppler_annot_ink_set_ink_list:
 * @annot: a #PopplerAnnotInk
 * @ink_list: (array length=n_paths) (transfer none): a list of #PopplerPath
 *
 * Each element of @ink_list is a path. The annotation must have
 * already been added to a page, otherwise the annotation may be
 * wrongly positioned if the page is rotated or has a cropbox.
 *
 * This function computes and set the appropriate smallest rectangle
 * area that contains all the points of @ink_list. Setting the rectangle
 * afterwards with #poppler_annot_set_rectangle should not be done
 * to preserve scaling and positioning.
 *
 * Since: 25.06.0
 */
void poppler_annot_ink_set_ink_list(PopplerAnnotInk *annot, PopplerPath **ink_list, gsize n_paths)
{
    double border_width;
    PopplerRectangle r = { G_MAXDOUBLE, G_MAXDOUBLE, 0, 0 };
    const PDFRectangle *crop_box;
    const PDFRectangle zerobox = PDFRectangle();
    Page *page = nullptr;
    auto *ink_annot = static_cast<AnnotInk *>(POPPLER_ANNOT(annot)->annot.get());
    std::vector<std::unique_ptr<AnnotPath>> paths;
    poppler_annot_get_border_width(POPPLER_ANNOT(annot), &border_width);

    crop_box = _poppler_annot_get_cropbox_and_page(POPPLER_ANNOT(annot), &page);
    if (!crop_box) {
        crop_box = &zerobox;
    }

    if (!page) {
        g_warning("An inklist of an ink annotation was set while the annotation was not"
                  " in a page, the computed coordinates may be wrong.");
    }

    for (gsize j = 0; j < n_paths; j++) {
        std::vector<AnnotCoord> coords;
        gsize n_points;
        PopplerPoint *points = poppler_path_get_points(ink_list[j], &n_points);

        for (gsize i = 0; i < n_points; i++) {
            PopplerPoint p = points[i];
            r.x1 = MIN(r.x1, p.x);
            r.y1 = MIN(r.y1, p.y);

            r.x2 = MAX(r.x2, p.x);
            r.y2 = MAX(r.y2, p.y);

            if (page) {
                _page_unrotate_xy(page, &p.x, &p.y);
            }
            p.x += crop_box->x1;
            p.y += crop_box->y1;
            coords.emplace_back(p.x, p.y);
        }
        paths.emplace_back(new AnnotPath(std::move(coords)));
    }

    r.x1 -= border_width;
    r.y1 -= border_width;
    r.x2 += border_width;
    r.y2 += border_width;
    poppler_annot_set_rectangle(POPPLER_ANNOT(annot), &r);
    ink_annot->setInkList(paths);
}

/**
 * poppler_annot_ink_get_ink_list:
 * @annot: a #PopplerAnnotInk
 * @n_paths: (out): to store the number of paths returned
 *
 * Each element of the return value is a path.
 *
 * Returns: (transfer full) (array length=n_paths): a GSList of PopplerPath
 *
 * Since: 25.06.0
 */
PopplerPath **poppler_annot_ink_get_ink_list(PopplerAnnotInk *annot, gsize *n_paths)
{
    PopplerPath **ink_list = nullptr;
    const PDFRectangle *crop_box;
    const PDFRectangle zerobox = PDFRectangle();
    Page *page = nullptr;
    int i = 0;

    auto *ink_annot = static_cast<AnnotInk *>(POPPLER_ANNOT(annot)->annot.get());
    const std::vector<std::unique_ptr<AnnotPath>> &paths = ink_annot->getInkList();

    *n_paths = paths.size();
    ink_list = g_new(PopplerPath *, paths.size());

    crop_box = _poppler_annot_get_cropbox_and_page(POPPLER_ANNOT(annot), &page);
    if (!crop_box) {
        crop_box = &zerobox;
    }

    for (const std::unique_ptr<AnnotPath> &path : paths) {
        auto *points = g_new(PopplerPoint, path->getCoordsLength());

        for (int j = 0; j < path->getCoordsLength(); ++j) {
            points[j].x = path->getX(j) - crop_box->x1;
            points[j].y = path->getY(j) - crop_box->y1;
            if (page) {
                _page_rotate_xy(page, &points[j].x, &points[j].y);
            }
        }
        ink_list[i] = poppler_path_new_from_array(points, path->getCoordsLength());
        i++;
    }

    return ink_list;
}

/**
 * poppler_annot_ink_set_draw_below:
 * @annot: a #PopplerAnnotInk
 * @draw_below: whether the annotation should be drawn below the document content
 *
 * This is typically used for highlight annotations. Technically, this implies that the
 * annotation is drawn using a multiply blend mode.
 *
 * Since: 25.10.0
 */
void poppler_annot_ink_set_draw_below(PopplerAnnotInk *annot, gboolean draw_below)
{
    auto *ink_annot = static_cast<AnnotInk *>(POPPLER_ANNOT(annot)->annot.get());

    ink_annot->setDrawBelow(draw_below);
}

/**
 * poppler_annot_ink_get_draw_below:
 * @annot: a #PopplerAnnotInk
 *
 * Returns whether the annotation is drawn below the page content or not.
 *
 * Since: 25.10.0
 */
gboolean poppler_annot_ink_get_draw_below(PopplerAnnotInk *annot)
{
    auto *ink_annot = static_cast<AnnotInk *>(POPPLER_ANNOT(annot)->annot.get());
    return ink_annot->getDrawBelow();
}

/**
 * poppler_annot_ink_new:
 * @doc: a #PopplerDocument
 * @rect: a #PopplerRectangle
 *
 * Creates a new ink annotation that will be
 * located on @rect when added to a page. See
 * poppler_page_add_annot()
 *
 * Return value: A newly created #PopplerAnnotInk annotation
 *
 * Since: 25.06.0
 */
PopplerAnnot *poppler_annot_ink_new(PopplerDocument *doc, PopplerRectangle *rect)
{
    PDFRectangle pdf_rect(rect->x1, rect->y1, rect->x2, rect->y2);

    auto annot = std::make_shared<AnnotInk>(doc->doc.get(), &pdf_rect);

    return _poppler_annot_ink_new(annot);
}

PopplerAnnot *_poppler_annot_ink_new(const std::shared_ptr<Annot> &annot)
{
    PopplerAnnot *poppler_annot;

    poppler_annot = _poppler_create_annot(POPPLER_TYPE_ANNOT_INK, annot);

    return poppler_annot;
}
