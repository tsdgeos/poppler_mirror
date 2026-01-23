/* poppler-page.cc: glib wrapper for poppler
 * Copyright (C) 2005, Red Hat, Inc.
 * Copyright (C) 2025 Lucas Baudin <lucas.baudin@ensae.fr>
 * Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Copyright (C) 2025 Nelson Benítez León <nbenitezl@gmail.com>
 * Copyright (C) 2025, 2026 Albert Astals Cid <aacid@kde.org>
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

#include "config.h"
#include <cmath>

#ifndef __GI_SCANNER__
#    include <GlobalParams.h>
#    include <PDFDoc.h>
#    include <Outline.h>
#    include <ErrorCodes.h>
#    include <UnicodeMap.h>
#    include <GfxState.h>
#    include <PageTransition.h>
#    include <BBoxOutputDev.h>
#    include <goo/gmem.h>
#endif

#include "poppler.h"
#include "poppler-private.h"

/**
 * SECTION:poppler-page
 * @short_description: Information about a page in a document
 * @title: PopplerPage
 */

namespace {
enum
{
    PROP_0,
    PROP_LABEL
};
}

static PopplerRectangleExtended *poppler_rectangle_extended_new();

struct _PopplerPageClass
{
    GObjectClass parent_class;
};
using PopplerPageClass = _PopplerPageClass;

G_DEFINE_TYPE(PopplerPage, poppler_page, G_TYPE_OBJECT)

PopplerPage *_poppler_page_new(PopplerDocument *document, Page *page, int index)
{
    PopplerPage *poppler_page;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    poppler_page = (PopplerPage *)g_object_new(POPPLER_TYPE_PAGE, nullptr, NULL);
    poppler_page->document = (PopplerDocument *)g_object_ref(document);
    poppler_page->page = page;
    poppler_page->index = index;

    return poppler_page;
}

static void poppler_page_finalize(GObject *object)
{
    PopplerPage *page = POPPLER_PAGE(object);

    g_object_unref(page->document);
    page->document = nullptr;
    page->text.reset();

    /* page->page is owned by the document */

    G_OBJECT_CLASS(poppler_page_parent_class)->finalize(object);
}

/**
 * poppler_page_get_size:
 * @page: A #PopplerPage
 * @width: (out) (allow-none): return location for the width of @page
 * @height: (out) (allow-none): return location for the height of @page
 *
 * Gets the size of @page at the current scale and rotation.
 **/
void poppler_page_get_size(PopplerPage *page, double *width, double *height)
{
    double page_width, page_height;
    int rotate;

    g_return_if_fail(POPPLER_IS_PAGE(page));

    rotate = page->page->getRotate();
    if (rotate == 90 || rotate == 270) {
        page_height = page->page->getCropWidth();
        page_width = page->page->getCropHeight();
    } else {
        page_width = page->page->getCropWidth();
        page_height = page->page->getCropHeight();
    }

    if (width != nullptr) {
        *width = page_width;
    }
    if (height != nullptr) {
        *height = page_height;
    }
}

/**
 * poppler_page_get_index:
 * @page: a #PopplerPage
 *
 * Returns the index of @page
 *
 * Return value: index value of @page
 **/
int poppler_page_get_index(PopplerPage *page)
{
    g_return_val_if_fail(POPPLER_IS_PAGE(page), 0);

    return page->index;
}

/**
 * poppler_page_get_label:
 * @page: a #PopplerPage
 *
 * Returns the label of @page. Note that page labels
 * and page indices might not coincide.
 *
 * Return value: a new allocated string containing the label of @page,
 *               or %NULL if @page doesn't have a label
 *
 * Since: 0.16
 **/
gchar *poppler_page_get_label(PopplerPage *page)
{
    GooString label;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    page->document->doc->getCatalog()->indexToLabel(page->index, &label);
    return _poppler_goo_string_to_utf8(&label);
}

/**
 * poppler_page_get_duration:
 * @page: a #PopplerPage
 *
 * Returns the duration of @page
 *
 * Return value: duration in seconds of @page or -1.
 **/
double poppler_page_get_duration(PopplerPage *page)
{
    g_return_val_if_fail(POPPLER_IS_PAGE(page), -1);

    return page->page->getDuration();
}

/**
 * poppler_page_get_transition:
 * @page: a #PopplerPage
 *
 * Returns the transition effect of @page
 *
 * Return value: a #PopplerPageTransition or %NULL.
 **/
PopplerPageTransition *poppler_page_get_transition(PopplerPage *page)
{
    PageTransition *trans;
    PopplerPageTransition *transition;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    Object obj = page->page->getTrans();
    trans = new PageTransition(&obj);

    if (!trans->isOk()) {
        delete trans;
        return nullptr;
    }

    transition = poppler_page_transition_new();

    switch (trans->getType()) {
    case transitionReplace:
        transition->type = POPPLER_PAGE_TRANSITION_REPLACE;
        break;
    case transitionSplit:
        transition->type = POPPLER_PAGE_TRANSITION_SPLIT;
        break;
    case transitionBlinds:
        transition->type = POPPLER_PAGE_TRANSITION_BLINDS;
        break;
    case transitionBox:
        transition->type = POPPLER_PAGE_TRANSITION_BOX;
        break;
    case transitionWipe:
        transition->type = POPPLER_PAGE_TRANSITION_WIPE;
        break;
    case transitionDissolve:
        transition->type = POPPLER_PAGE_TRANSITION_DISSOLVE;
        break;
    case transitionGlitter:
        transition->type = POPPLER_PAGE_TRANSITION_GLITTER;
        break;
    case transitionFly:
        transition->type = POPPLER_PAGE_TRANSITION_FLY;
        break;
    case transitionPush:
        transition->type = POPPLER_PAGE_TRANSITION_PUSH;
        break;
    case transitionCover:
        transition->type = POPPLER_PAGE_TRANSITION_COVER;
        break;
    case transitionUncover:
        transition->type = POPPLER_PAGE_TRANSITION_UNCOVER;
        break;
    case transitionFade:
        transition->type = POPPLER_PAGE_TRANSITION_FADE;
        break;
    default:
        g_assert_not_reached();
    }

    transition->alignment = (trans->getAlignment() == transitionHorizontal) ? POPPLER_PAGE_TRANSITION_HORIZONTAL : POPPLER_PAGE_TRANSITION_VERTICAL;

    transition->direction = (trans->getDirection() == transitionInward) ? POPPLER_PAGE_TRANSITION_INWARD : POPPLER_PAGE_TRANSITION_OUTWARD;

    transition->duration = trans->getDuration();
    transition->duration_real = trans->getDuration();
    transition->angle = trans->getAngle();
    transition->scale = trans->getScale();
    transition->rectangular = trans->isRectangular();

    delete trans;

    return transition;
}

static TextPage *poppler_page_get_text_page(PopplerPage *page)
{
    if (page->text == nullptr) {

        auto text_dev = std::make_unique<TextOutputDev>(nullptr, true, 0, false, false);
        std::unique_ptr<Gfx> gfx = page->page->createGfx(text_dev.get(), 72.0, 72.0, 0, false, /* useMediaBox */
                                                         true, /* Crop */
                                                         -1, -1, -1, -1, nullptr, nullptr);
        page->page->display(gfx.get());
        text_dev->endPage();

        page->text = text_dev->takeText();
    }

    return page->text.get();
}

static bool annots_display_decide_cb(Annot *annot, void *user_data)
{
    auto flags = (PopplerRenderAnnotsFlags)GPOINTER_TO_UINT(user_data);
    Annot::AnnotSubtype type = annot->getType();
    int typeMask = 1 << MAX(0, (((int)type) - 1));

    return (flags & typeMask) != 0;
}

/**
 * poppler_page_render_full:
 * @page: the page to render from
 * @cairo: cairo context to render to
 * @printing: cairo context to render to
 * @flags: flags which allow to select which annotations to render
 *
 * Render the page to the given cairo context, manually selecting which
 * annotations should be displayed.
 *
 * The @printing parameter determines whether a page is rendered for printing
 * or for displaying it on a screen. See the documentation for
 * poppler_page_render_for_printing() for the differences between rendering to
 * the screen and rendering to a printer.
 *
 * Since: 25.02
 **/
void poppler_page_render_full(PopplerPage *page, cairo_t *cairo, gboolean printing, PopplerRenderAnnotsFlags flags)
{
    CairoOutputDev *output_dev;

    g_return_if_fail(POPPLER_IS_PAGE(page));
    g_return_if_fail(cairo != nullptr);

    output_dev = page->document->output_dev;
    output_dev->setCairo(cairo);
    output_dev->setPrinting(printing);

    if (!printing && page->text == nullptr) {
        page->text = std::make_shared<TextPage>(false);
        output_dev->setTextPage(page->text);
    }

    cairo_save(cairo);
    page->page->displaySlice(output_dev, 72.0, 72.0, 0, false, /* useMediaBox */
                             true, /* Crop */
                             -1, -1, -1, -1, /* instead of passing -1 we could use cairo_clip_extents() to get a bounding box */

                             printing, nullptr, nullptr, annots_display_decide_cb, GUINT_TO_POINTER((guint)flags));
    cairo_restore(cairo);

    output_dev->setCairo(nullptr);
    output_dev->setTextPage(nullptr);
}

/**
 * poppler_page_render:
 * @page: the page to render from
 * @cairo: cairo context to render to
 *
 * Render the page to the given cairo context. This function
 * is for rendering a page that will be displayed. If you want
 * to render a page that will be printed use
 * poppler_page_render_for_printing() instead.  Please see the documentation
 * for that function for the differences between rendering to the screen and
 * rendering to a printer.
 **/
void poppler_page_render(PopplerPage *page, cairo_t *cairo)
{
    poppler_page_render_full(page, cairo, false, POPPLER_RENDER_ANNOTS_ALL);
}

/**
 * poppler_page_render_for_printing_with_options:
 * @page: the page to render from
 * @cairo: cairo context to render to
 * @options: print options
 *
 * Render the page to the given cairo context for printing
 * with the specified options
 *
 * See the documentation for poppler_page_render_for_printing() for the
 * differences between rendering to the screen and rendering to a printer.
 *
 * Since: 0.16
 *
 * Deprecated: 25.02: Use poppler_page_render_full() instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
void poppler_page_render_for_printing_with_options(PopplerPage *page, cairo_t *cairo, PopplerPrintFlags options)
{
    int flags = (int)POPPLER_RENDER_ANNOTS_PRINT_DOCUMENT;

    if (options & POPPLER_PRINT_STAMP_ANNOTS_ONLY) {
        flags |= POPPLER_RENDER_ANNOTS_PRINT_STAMP;
    }
    if (options & POPPLER_PRINT_MARKUP_ANNOTS) {
        flags |= POPPLER_RENDER_ANNOTS_PRINT_MARKUP;
    }

    poppler_page_render_full(page, cairo, true, (PopplerRenderAnnotsFlags)flags);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * poppler_page_render_for_printing:
 * @page: the page to render from
 * @cairo: cairo context to render to
 *
 * Render the page to the given cairo context for printing with
 * #POPPLER_PRINT_ALL flags selected.  If you want a different set of flags,
 * use poppler_page_render_full() with printing #TRUE and the corresponding
 * flags.
 *
 * The difference between poppler_page_render() and this function is that some
 * things get rendered differently between screens and printers:
 *
 * <itemizedlist>
 *   <listitem>
 *     PDF annotations get rendered according to their #PopplerAnnotFlag value.
 *     For example, #POPPLER_ANNOT_FLAG_PRINT refers to whether an annotation
 *     is printed or not, whereas #POPPLER_ANNOT_FLAG_NO_VIEW refers to whether
 *     an annotation is invisible when displaying to the screen.
 *   </listitem>
 *   <listitem>
 *     PDF supports "hairlines" of width 0.0, which often get rendered as
 *     having a width of 1 device pixel.  When displaying on a screen, Cairo
 *     may render such lines wide so that they are hard to see, and Poppler
 *     makes use of PDF's Stroke Adjust graphics parameter to make the lines
 *     easier to see.  However, when printing, Poppler is able to directly use a
 *     printer's pixel size instead.
 *   </listitem>
 *   <listitem>
 *     Some advanced features in PDF may require an image to be rasterized
 *     before sending off to a printer.  This may produce raster images which
 *     exceed Cairo's limits.  The "printing" functions will detect this condition
 *     and try to down-scale the intermediate surfaces as appropriate.
 *   </listitem>
 * </itemizedlist>
 *
 **/
void poppler_page_render_for_printing(PopplerPage *page, cairo_t *cairo)
{
    poppler_page_render_full(page, cairo, true, POPPLER_RENDER_ANNOTS_PRINT_ALL);
}

static cairo_surface_t *create_surface_from_thumbnail_data(guchar *data, gint width, gint height, gint rowstride)
{
    guchar *cairo_pixels;
    gint cairo_stride;
    cairo_surface_t *surface;
    int j;

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    if (cairo_surface_status(surface)) {
        return nullptr;
    }

    cairo_pixels = cairo_image_surface_get_data(surface);
    cairo_stride = cairo_image_surface_get_stride(surface);

    for (j = height; j; j--) {
        guchar *p = data;
        guchar *q = cairo_pixels;
        guchar *end = p + 3 * width;

        while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
            q[0] = p[2];
            q[1] = p[1];
            q[2] = p[0];
#else
            q[1] = p[0];
            q[2] = p[1];
            q[3] = p[2];
#endif
            p += 3;
            q += 4;
        }

        data += rowstride;
        cairo_pixels += cairo_stride;
    }

    return surface;
}

/**
 * poppler_page_get_thumbnail:
 * @page: the #PopplerPage to get the thumbnail for
 *
 * Get the embedded thumbnail for the specified page.  If the document
 * doesn't have an embedded thumbnail for the page, this function
 * returns %NULL.
 *
 * Return value: the tumbnail as a cairo_surface_t or %NULL if the document
 * doesn't have a thumbnail for this page.
 **/
cairo_surface_t *poppler_page_get_thumbnail(PopplerPage *page)
{
    unsigned char *data;
    int width, height, rowstride;
    cairo_surface_t *surface;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    if (!page->page->loadThumb(&data, &width, &height, &rowstride)) {
        return nullptr;
    }

    surface = create_surface_from_thumbnail_data(data, width, height, rowstride);
    gfree(data);

    return surface;
}

static void render_selection(PopplerPage *page, cairo_t *cairo, PopplerRectangle *selection, PopplerSelectionStyle style, PopplerColor *glyph_color, PopplerColor *background_color, double background_opacity, bool draw_glyphs)
{
    CairoOutputDev *output_dev;
    TextPage *text;
    SelectionStyle selection_style = selectionStyleGlyph;
    PDFRectangle pdf_selection(selection->x1, selection->y1, selection->x2, selection->y2);

    GfxColor gfx_background_color = { { background_color->red, background_color->green, background_color->blue } };
    GfxColor gfx_glyph_color = { { glyph_color->red, glyph_color->green, glyph_color->blue } };

    switch (style) {
    case POPPLER_SELECTION_GLYPH:
        selection_style = selectionStyleGlyph;
        break;
    case POPPLER_SELECTION_WORD:
        selection_style = selectionStyleWord;
        break;
    case POPPLER_SELECTION_LINE:
        selection_style = selectionStyleLine;
        break;
    }

    output_dev = page->document->output_dev;
    output_dev->setCairo(cairo);

    text = poppler_page_get_text_page(page);
    text->drawSelection(output_dev, 1.0, 0, &pdf_selection, selection_style, &gfx_glyph_color, &gfx_background_color, background_opacity, draw_glyphs);

    output_dev->setCairo(nullptr);
}

/**
 * poppler_page_render_selection:
 * @page: the #PopplerPage for which to render selection
 * @cairo: cairo context to render to
 * @selection: start and end point of selection as a rectangle
 * @old_selection: previous selection
 * @style: a #PopplerSelectionStyle
 * @glyph_color: color to use for drawing glyphs
 * @background_color: color to use for the selection background
 *
 * Render the selection specified by @selection for @page to
 * the given cairo context.  The selection will be rendered, using
 * @glyph_color for the glyphs and @background_color for the selection
 * background.
 *
 * If non-NULL, @old_selection specifies the selection that is already
 * rendered to @cairo, in which case this function will (some day)
 * only render the changed part of the selection.
 **/
void poppler_page_render_selection(PopplerPage *page, cairo_t *cairo, PopplerRectangle *selection, PopplerRectangle * /*old_selection*/, PopplerSelectionStyle style, PopplerColor *glyph_color, PopplerColor *background_color)
{
    render_selection(page, cairo, selection, style, glyph_color, background_color, 1, TRUE);
}

/**
 * poppler_page_render_transparent_selection:
 * @page: the #PopplerPage for which to render selection
 * @cairo: cairo context to render to
 * @selection: start and end point of selection as a rectangle
 * @old_selection: previous selection
 * @style: a #PopplerSelectionStyle
 * @background_color: color to use for the selection background
 * @background_opacity: opacity to use for the selection background
 *
 * Render the selection specified by @selection for @page to
 * the given cairo context.  The selection will be rendered using
 * @background_color and @background_opacity for the selection
 * background. Glyphs will not be drawn.
 *
 * If non-NULL, @old_selection specifies the selection that is already
 * rendered to @cairo, in which case this function will (some day)
 * only render the changed part of the selection.
 *
 * Since: 25.08
 **/
void poppler_page_render_transparent_selection(PopplerPage *page, cairo_t *cairo, PopplerRectangle *selection, PopplerRectangle * /*old_selection*/, PopplerSelectionStyle style, PopplerColor *background_color, double background_opacity)
{
    PopplerColor glyph_color = { 0, 0, 0 };

    render_selection(page, cairo, selection, style, &glyph_color, background_color, background_opacity, FALSE);
}

/**
 * poppler_page_get_thumbnail_size:
 * @page: A #PopplerPage
 * @width: (out): return location for width
 * @height: (out): return location for height
 *
 * Returns %TRUE if @page has a thumbnail associated with it.  It also
 * fills in @width and @height with the width and height of the
 * thumbnail.  The values of width and height are not changed if no
 * appropriate thumbnail exists.
 *
 * Return value: %TRUE, if @page has a thumbnail associated with it.
 **/
gboolean poppler_page_get_thumbnail_size(PopplerPage *page, int *width, int *height)
{
    Dict *dict;
    gboolean retval = FALSE;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), FALSE);
    g_return_val_if_fail(width != nullptr, FALSE);
    g_return_val_if_fail(height != nullptr, FALSE);

    Object thumb = page->page->getThumb();
    if (!thumb.isStream()) {
        return FALSE;
    }

    dict = thumb.streamGetDict();

    /* Theoretically, this could succeed and you would still fail when
     * loading the thumb */
    if (dict->lookupInt("Width", "W", width) && dict->lookupInt("Height", "H", height)) {
        retval = TRUE;
    }

    return retval;
}

/**
 * poppler_page_get_selection_region:
 * @page: a #PopplerPage
 * @scale: scale specified as pixels per point
 * @style: a #PopplerSelectionStyle
 * @selection: start and end point of selection as a rectangle
 *
 * Returns a region containing the area that would be rendered by
 * poppler_page_render_selection() as a #GList of
 * #PopplerRectangle. The returned list must be freed with
 * poppler_page_selection_region_free().
 *
 * Return value: (element-type PopplerRectangle) (transfer full): a #GList of #PopplerRectangle
 *
 * Deprecated: 0.16: Use poppler_page_get_selected_region() instead.
 **/
GList *poppler_page_get_selection_region(PopplerPage *page, gdouble scale, PopplerSelectionStyle style, PopplerRectangle *selection)
{
    PDFRectangle poppler_selection;
    TextPage *text;
    SelectionStyle selection_style = selectionStyleGlyph;
    GList *region = nullptr;

    poppler_selection.x1 = selection->x1;
    poppler_selection.y1 = selection->y1;
    poppler_selection.x2 = selection->x2;
    poppler_selection.y2 = selection->y2;

    switch (style) {
    case POPPLER_SELECTION_GLYPH:
        selection_style = selectionStyleGlyph;
        break;
    case POPPLER_SELECTION_WORD:
        selection_style = selectionStyleWord;
        break;
    case POPPLER_SELECTION_LINE:
        selection_style = selectionStyleLine;
        break;
    }

    text = poppler_page_get_text_page(page);
    std::vector<PDFRectangle *> *list = text->getSelectionRegion(&poppler_selection, selection_style, scale);

    for (const PDFRectangle *selection_rect : *list) {
        PopplerRectangle *rect;

        rect = poppler_rectangle_new_from_pdf_rectangle(selection_rect);

        region = g_list_prepend(region, rect);

        delete selection_rect;
    }

    delete list;

    return g_list_reverse(region);
}

/**
 * poppler_page_selection_region_free:
 * @region: (element-type PopplerRectangle): a #GList of
 *   #PopplerRectangle
 *
 * Frees @region
 *
 * Deprecated: 0.16: Use only to free deprecated regions created by
 * poppler_page_get_selection_region(). Regions created by
 * poppler_page_get_selected_region() should be freed with
 * cairo_region_destroy() instead.
 */
void poppler_page_selection_region_free(GList *region)
{
    if (G_UNLIKELY(!region)) {
        return;
    }

    g_list_free_full(region, (GDestroyNotify)poppler_rectangle_free);
}

/**
 * poppler_page_get_selected_region:
 * @page: a #PopplerPage
 * @scale: scale specified as pixels per point
 * @style: a #PopplerSelectionStyle
 * @selection: start and end point of selection as a rectangle
 *
 * Returns a region containing the area that would be rendered by
 * poppler_page_render_selection().
 * The returned region must be freed with cairo_region_destroy()
 *
 * Return value: (transfer full): a cairo_region_t
 *
 * Since: 0.16
 **/
cairo_region_t *poppler_page_get_selected_region(PopplerPage *page, gdouble scale, PopplerSelectionStyle style, PopplerRectangle *selection)
{
    PDFRectangle poppler_selection;
    TextPage *text;
    SelectionStyle selection_style = selectionStyleGlyph;
    cairo_region_t *region;

    poppler_selection.x1 = selection->x1;
    poppler_selection.y1 = selection->y1;
    poppler_selection.x2 = selection->x2;
    poppler_selection.y2 = selection->y2;

    switch (style) {
    case POPPLER_SELECTION_GLYPH:
        selection_style = selectionStyleGlyph;
        break;
    case POPPLER_SELECTION_WORD:
        selection_style = selectionStyleWord;
        break;
    case POPPLER_SELECTION_LINE:
        selection_style = selectionStyleLine;
        break;
    }

    text = poppler_page_get_text_page(page);
    std::vector<PDFRectangle *> *list = text->getSelectionRegion(&poppler_selection, selection_style, 1.0);

    region = cairo_region_create();

    for (const PDFRectangle *selection_rect : *list) {
        cairo_rectangle_int_t rect;

        rect.x = (gint)((selection_rect->x1 * scale) + 0.5);
        rect.y = (gint)((selection_rect->y1 * scale) + 0.5);
        rect.width = (gint)(((selection_rect->x2 - selection_rect->x1) * scale) + 0.5);
        rect.height = (gint)(((selection_rect->y2 - selection_rect->y1) * scale) + 0.5);
        cairo_region_union_rectangle(region, &rect);

        delete selection_rect;
    }

    delete list;

    return region;
}

/**
 * poppler_page_get_selected_text:
 * @page: a #PopplerPage
 * @style: a #PopplerSelectionStyle
 * @selection: the #PopplerRectangle including the text
 *
 * Retrieves the contents of the specified @selection as text.
 *
 * Returns: (transfer full): a pointer to the contents of the
 * @selection as a string
 *
 * Since: 0.16
 **/
char *poppler_page_get_selected_text(PopplerPage *page, PopplerSelectionStyle style, PopplerRectangle *selection)
{
    char *result;
    TextPage *text;
    SelectionStyle selection_style = selectionStyleGlyph;
    PDFRectangle pdf_selection;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);
    g_return_val_if_fail(selection != nullptr, NULL);

    pdf_selection.x1 = selection->x1;
    pdf_selection.y1 = selection->y1;
    pdf_selection.x2 = selection->x2;
    pdf_selection.y2 = selection->y2;

    switch (style) {
    case POPPLER_SELECTION_GLYPH:
        selection_style = selectionStyleGlyph;
        break;
    case POPPLER_SELECTION_WORD:
        selection_style = selectionStyleWord;
        break;
    case POPPLER_SELECTION_LINE:
        selection_style = selectionStyleLine;
        break;
    }

    text = poppler_page_get_text_page(page);
    GooString sel_text = text->getSelectionText(&pdf_selection, selection_style);
    result = g_strdup(sel_text.c_str());

    return result;
}

/**
 * poppler_page_get_text:
 * @page: a #PopplerPage
 *
 * Retrieves the text of @page.
 *
 * Return value: a pointer to the text of the @page
 *               as a string
 * Since: 0.16
 **/
char *poppler_page_get_text(PopplerPage *page)
{
    PopplerRectangle rectangle = { 0, 0, 0, 0 };

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    poppler_page_get_size(page, &rectangle.x2, &rectangle.y2);

    return poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &rectangle);
}

/**
 * poppler_page_get_text_for_area:
 * @page: a #PopplerPage
 * @area: a #PopplerRectangle
 *
 * Retrieves the text of @page contained in @area.
 *
 * Return value: a pointer to the text as a string
 *
 * Since: 0.26
 **/
char *poppler_page_get_text_for_area(PopplerPage *page, PopplerRectangle *area)
{
    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);
    g_return_val_if_fail(area != nullptr, NULL);

    return poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, area);
}

/**
 * poppler_page_find_text_with_options:
 * @page: a #PopplerPage
 * @text: the text to search for (UTF-8 encoded)
 * @options: find options
 *
 * Finds @text in @page with the given #PopplerFindFlags options and
 * returns a #GList of rectangles for each occurrence of the text on the page.
 * The coordinates are in PDF points.
 *
 * When %POPPLER_FIND_MULTILINE is passed in @options, matches may span more than
 * one line. In this case, the returned list will contain one #PopplerRectangle
 * for each part of a match. The function poppler_rectangle_find_get_match_continued()
 * will return %TRUE for all rectangles belonging to the same match, except for
 * the last one. If a hyphen was ignored at the end of the part of the match,
 * poppler_rectangle_find_get_ignored_hyphen() will return %TRUE for that
 * rectangle.
 *
 * Note that currently matches spanning more than two lines are not found.
 * (This limitation may be lifted in a future version.)
 *
 * Note also that currently finding multi-line matches backwards is not
 * implemented; if you pass %POPPLER_FIND_BACKWARDS and %POPPLER_FIND_MULTILINE
 * together, %POPPLER_FIND_MULTILINE will be ignored.
 *
 * Return value: (element-type PopplerRectangle) (transfer full): a newly allocated list
 * of newly allocated #PopplerRectangle. Free with g_list_free_full() using poppler_rectangle_free().
 *
 * Since: 0.22
 **/
GList *poppler_page_find_text_with_options(PopplerPage *page, const char *text, PopplerFindFlags options)
{
    PopplerRectangleExtended *match;
    GList *matches;
    double xMin, yMin, xMax, yMax;
    PDFRectangle continueMatch;
    bool ignoredHyphen;
    gunichar *ucs4;
    glong ucs4_len;
    double height;
    TextPage *text_dev;
    gboolean backwards;
    gboolean start_at_last = FALSE;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);
    g_return_val_if_fail(text != nullptr, NULL);

    text_dev = poppler_page_get_text_page(page);

    ucs4 = g_utf8_to_ucs4_fast(text, -1, &ucs4_len);
    poppler_page_get_size(page, nullptr, &height);

    const bool multiline = (options & POPPLER_FIND_MULTILINE);
    backwards = options & POPPLER_FIND_BACKWARDS;
    matches = nullptr;
    xMin = 0;
    yMin = backwards ? height : 0;

    continueMatch.x1 = std::numeric_limits<double>::max(); // we use this to detect valid returned values

    while (text_dev->findText(ucs4, ucs4_len, false, true, // startAtTop, stopAtBottom
                              start_at_last,
                              false, // stopAtLast
                              options & POPPLER_FIND_CASE_SENSITIVE, options & POPPLER_FIND_IGNORE_DIACRITICS, options & POPPLER_FIND_MULTILINE, backwards, options & POPPLER_FIND_WHOLE_WORDS_ONLY, &xMin, &yMin, &xMax, &yMax, &continueMatch,
                              &ignoredHyphen)) {
        match = poppler_rectangle_extended_new();
        match->x1 = xMin;
        match->y1 = height - yMax;
        match->x2 = xMax;
        match->y2 = height - yMin;
        match->match_continued = false;
        match->ignored_hyphen = false;
        matches = g_list_prepend(matches, match);
        start_at_last = TRUE;

        if (continueMatch.x1 != std::numeric_limits<double>::max()) {
            // received rect for next-line part of a multi-line match, add it.
            if (multiline) {
                match->match_continued = true;
                match->ignored_hyphen = ignoredHyphen;
                match = poppler_rectangle_extended_new();
                match->x1 = continueMatch.x1;
                match->y1 = height - continueMatch.y1;
                match->x2 = continueMatch.x2;
                match->y2 = height - continueMatch.y2;
                match->match_continued = false;
                match->ignored_hyphen = false;
                matches = g_list_prepend(matches, match);
            }

            continueMatch.x1 = std::numeric_limits<double>::max();
        }
    }

    g_free(ucs4);

    return g_list_reverse(matches);
}

/**
 * poppler_page_find_text:
 * @page: a #PopplerPage
 * @text: the text to search for (UTF-8 encoded)
 *
 * Finds @text in @page with the default options (%POPPLER_FIND_DEFAULT) and
 * returns a #GList of rectangles for each occurrence of the text on the page.
 * The coordinates are in PDF points.
 *
 * Return value: (element-type PopplerRectangle) (transfer full): a #GList of #PopplerRectangle,
 **/
GList *poppler_page_find_text(PopplerPage *page, const char *text)
{
    return poppler_page_find_text_with_options(page, text, POPPLER_FIND_DEFAULT);
}

static CairoImageOutputDev *poppler_page_get_image_output_dev(PopplerPage *page, bool (*imgDrawDeviceCbk)(int img_id, void *data), void *imgDrawCbkData)
{
    CairoImageOutputDev *image_dev;

    image_dev = new CairoImageOutputDev();

    if (imgDrawDeviceCbk) {
        image_dev->setImageDrawDecideCbk(imgDrawDeviceCbk, imgDrawCbkData);
    }

    std::unique_ptr<Gfx> gfx = page->page->createGfx(image_dev, 72.0, 72.0, 0, false, /* useMediaBox */
                                                     true, /* Crop */
                                                     -1, -1, -1, -1, nullptr, nullptr);
    page->page->display(gfx.get());

    return image_dev;
}

/**
 * poppler_page_get_image_mapping:
 * @page: A #PopplerPage
 *
 * Returns a list of #PopplerImageMapping items that map from a
 * location on @page to an image of the page. This list must be freed
 * with poppler_page_free_image_mapping() when done.
 *
 * Return value: (element-type PopplerImageMapping) (transfer full): A #GList of #PopplerImageMapping
 **/
GList *poppler_page_get_image_mapping(PopplerPage *page)
{
    GList *map_list = nullptr;
    CairoImageOutputDev *out;
    gint i;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    out = poppler_page_get_image_output_dev(page, nullptr, nullptr);

    for (i = 0; i < out->getNumImages(); i++) {
        PopplerImageMapping *mapping;
        CairoImage *image;

        image = out->getImage(i);

        /* Create the mapping */
        mapping = poppler_image_mapping_new();

        image->getRect(&(mapping->area.x1), &(mapping->area.y1), &(mapping->area.x2), &(mapping->area.y2));
        mapping->image_id = i;

        mapping->area.x1 -= page->page->getCropBox()->x1;
        mapping->area.x2 -= page->page->getCropBox()->x1;
        mapping->area.y1 -= page->page->getCropBox()->y1;
        mapping->area.y2 -= page->page->getCropBox()->y1;

        map_list = g_list_prepend(map_list, mapping);
    }

    delete out;

    return map_list;
}

static bool image_draw_decide_cb(int image_id, void *data)
{
    return (image_id == GPOINTER_TO_INT(data));
}

/**
 * poppler_page_get_image:
 * @page: A #PopplerPage
 * @image_id: The image identifier
 *
 * Returns a cairo surface for the image of the @page
 *
 * Return value: A cairo surface for the image
 **/
cairo_surface_t *poppler_page_get_image(PopplerPage *page, gint image_id)
{
    CairoImageOutputDev *out;
    cairo_surface_t *image;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    out = poppler_page_get_image_output_dev(page, image_draw_decide_cb, GINT_TO_POINTER(image_id));

    if (image_id >= out->getNumImages()) {
        delete out;

        return nullptr;
    }

    image = out->getImage(image_id)->getImage();
    if (!image) {
        delete out;

        return nullptr;
    }

    cairo_surface_reference(image);
    delete out;

    return image;
}

/**
 * poppler_page_free_image_mapping:
 * @list: (element-type PopplerImageMapping): A list of
 *   #PopplerImageMapping<!-- -->s
 *
 * Frees a list of #PopplerImageMapping<!-- -->s allocated by
 * poppler_page_get_image_mapping().
 **/
void poppler_page_free_image_mapping(GList *list)
{
    if (G_UNLIKELY(list == nullptr)) {
        return;
    }

    g_list_free_full(list, (GDestroyNotify)poppler_image_mapping_free);
}

/**
 * poppler_page_render_to_ps:
 * @page: a #PopplerPage
 * @ps_file: the PopplerPSFile to render to
 *
 * Render the page on a postscript file
 *
 **/
void poppler_page_render_to_ps(PopplerPage *page, PopplerPSFile *ps_file)
{
    g_return_if_fail(POPPLER_IS_PAGE(page));
    g_return_if_fail(ps_file != nullptr);

    if (!ps_file->out) {
        std::vector<int> pages;
        for (int i = ps_file->first_page; i <= ps_file->last_page; ++i) {
            pages.push_back(i);
        }
        if (ps_file->fd != -1) {
            ps_file->out = new PSOutputDev(ps_file->fd, ps_file->document->doc.get(), nullptr, pages, psModePS, (int)ps_file->paper_width, (int)ps_file->paper_height, false, ps_file->duplex, 0, 0, 0, 0, psRasterizeWhenNeeded, false,
                                           nullptr, nullptr);
        } else {
            ps_file->out = new PSOutputDev(ps_file->filename, ps_file->document->doc.get(), nullptr, pages, psModePS, (int)ps_file->paper_width, (int)ps_file->paper_height, false, ps_file->duplex, 0, 0, 0, 0, psRasterizeWhenNeeded, false,
                                           nullptr, nullptr);
        }
    }

    ps_file->document->doc->displayPage(ps_file->out, page->index + 1, 72.0, 72.0, 0, false, true, false);
}

static void poppler_page_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    PopplerPage *page = POPPLER_PAGE(object);

    switch (prop_id) {
    case PROP_LABEL:
        g_value_take_string(value, poppler_page_get_label(page));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void poppler_page_class_init(PopplerPageClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_page_finalize;
    gobject_class->get_property = poppler_page_get_property;

    /**
     * PopplerPage:label:
     *
     * The label of the page or %NULL. See also poppler_page_get_label()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_LABEL, g_param_spec_string("label", "Page Label", "The label of the page", nullptr, G_PARAM_READABLE));
}

static void poppler_page_init(PopplerPage * /*page*/) { }

/**
 * poppler_page_get_link_mapping:
 * @page: A #PopplerPage
 *
 * Returns a list of #PopplerLinkMapping items that map from a
 * location on @page to a #PopplerAction.  This list must be freed
 * with poppler_page_free_link_mapping() when done.
 *
 * Return value: (element-type PopplerLinkMapping) (transfer full): A #GList of #PopplerLinkMapping
 **/
GList *poppler_page_get_link_mapping(PopplerPage *page)
{
    GList *map_list = nullptr;
    Links *links;
    double width, height;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    links = new Links(page->page->getAnnots());

    if (links == nullptr) {
        return nullptr;
    }

    poppler_page_get_size(page, &width, &height);

    for (const std::shared_ptr<AnnotLink> &link : links->getLinks()) {
        PopplerLinkMapping *mapping;
        PopplerRectangle rect;
        LinkAction *link_action;

        link_action = link->getAction();

        /* Create the mapping */
        mapping = poppler_link_mapping_new();
        mapping->action = _poppler_action_new(page->document, link_action, nullptr);

        link->getRect(&rect.x1, &rect.y1, &rect.x2, &rect.y2);

        rect.x1 -= page->page->getCropBox()->x1;
        rect.x2 -= page->page->getCropBox()->x1;
        rect.y1 -= page->page->getCropBox()->y1;
        rect.y2 -= page->page->getCropBox()->y1;

        switch (page->page->getRotate()) {
        case 90:
            mapping->area.x1 = rect.y1;
            mapping->area.y1 = height - rect.x2;
            mapping->area.x2 = mapping->area.x1 + (rect.y2 - rect.y1);
            mapping->area.y2 = mapping->area.y1 + (rect.x2 - rect.x1);

            break;
        case 180:
            mapping->area.x1 = width - rect.x2;
            mapping->area.y1 = height - rect.y2;
            mapping->area.x2 = mapping->area.x1 + (rect.x2 - rect.x1);
            mapping->area.y2 = mapping->area.y1 + (rect.y2 - rect.y1);

            break;
        case 270:
            mapping->area.x1 = width - rect.y2;
            mapping->area.y1 = rect.x1;
            mapping->area.x2 = mapping->area.x1 + (rect.y2 - rect.y1);
            mapping->area.y2 = mapping->area.y1 + (rect.x2 - rect.x1);

            break;
        default:
            mapping->area.x1 = rect.x1;
            mapping->area.y1 = rect.y1;
            mapping->area.x2 = rect.x2;
            mapping->area.y2 = rect.y2;
        }

        map_list = g_list_prepend(map_list, mapping);
    }

    delete links;

    return map_list;
}

/**
 * poppler_page_free_link_mapping:
 * @list: (element-type PopplerLinkMapping): A list of
 *   #PopplerLinkMapping<!-- -->s
 *
 * Frees a list of #PopplerLinkMapping<!-- -->s allocated by
 * poppler_page_get_link_mapping().  It also frees the #PopplerAction<!-- -->s
 * that each mapping contains, so if you want to keep them around, you need to
 * copy them with poppler_action_copy().
 **/
void poppler_page_free_link_mapping(GList *list)
{
    if (G_UNLIKELY(list == nullptr)) {
        return;
    }

    g_list_free_full(list, (GDestroyNotify)poppler_link_mapping_free);
}

/**
 * poppler_page_get_form_field_mapping:
 * @page: A #PopplerPage
 *
 * Returns a list of #PopplerFormFieldMapping items that map from a
 * location on @page to a form field.  This list must be freed
 * with poppler_page_free_form_field_mapping() when done.
 *
 * Return value: (element-type PopplerFormFieldMapping) (transfer full): A #GList of #PopplerFormFieldMapping
 **/
GList *poppler_page_get_form_field_mapping(PopplerPage *page)
{
    GList *map_list = nullptr;
    gint i;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    const std::unique_ptr<FormPageWidgets> forms = page->page->getFormWidgets();

    if (forms == nullptr) {
        return nullptr;
    }

    for (i = 0; i < forms->getNumWidgets(); i++) {
        PopplerFormFieldMapping *mapping;
        FormWidget *field;

        mapping = poppler_form_field_mapping_new();

        field = forms->getWidget(i);

        mapping->field = _poppler_form_field_new(page->document, field);
        field->getRect(&(mapping->area.x1), &(mapping->area.y1), &(mapping->area.x2), &(mapping->area.y2));

        mapping->area.x1 -= page->page->getCropBox()->x1;
        mapping->area.x2 -= page->page->getCropBox()->x1;
        mapping->area.y1 -= page->page->getCropBox()->y1;
        mapping->area.y2 -= page->page->getCropBox()->y1;

        map_list = g_list_prepend(map_list, mapping);
    }

    return map_list;
}

/**
 * poppler_page_free_form_field_mapping:
 * @list: (element-type PopplerFormFieldMapping): A list of
 *   #PopplerFormFieldMapping<!-- -->s
 *
 * Frees a list of #PopplerFormFieldMapping<!-- -->s allocated by
 * poppler_page_get_form_field_mapping().
 **/
void poppler_page_free_form_field_mapping(GList *list)
{
    if (G_UNLIKELY(list == nullptr)) {
        return;
    }

    g_list_free_full(list, (GDestroyNotify)poppler_form_field_mapping_free);
}

/**
 * poppler_page_get_annot_mapping:
 * @page: A #PopplerPage
 *
 * Returns a list of #PopplerAnnotMapping items that map from a location on
 * @page to a #PopplerAnnot.  This list must be freed with
 * poppler_page_free_annot_mapping() when done.
 *
 * Return value: (element-type PopplerAnnotMapping) (transfer full): A #GList of #PopplerAnnotMapping
 **/
GList *poppler_page_get_annot_mapping(PopplerPage *page)
{
    GList *map_list = nullptr;
    double width, height;
    Annots *annots;
    const PDFRectangle *crop_box;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    annots = page->page->getAnnots();
    if (!annots) {
        return nullptr;
    }

    poppler_page_get_size(page, &width, &height);
    crop_box = page->page->getCropBox();

    for (const std::shared_ptr<Annot> &annot : annots->getAnnots()) {
        PopplerAnnotMapping *mapping;
        PopplerRectangle rect;
        gboolean flag_no_rotate;
        gint rotation = 0;

        flag_no_rotate = annot->getFlags() & Annot::flagNoRotate;

        /* Create the mapping */
        mapping = poppler_annot_mapping_new();

        switch (annot->getType()) {
        case Annot::typeText:
            mapping->annot = _poppler_annot_text_new(annot);
            break;
        case Annot::typeInk:
            mapping->annot = _poppler_annot_ink_new(annot);
            break;
        case Annot::typeFreeText:
            mapping->annot = _poppler_annot_free_text_new(annot);
            break;
        case Annot::typeFileAttachment:
            mapping->annot = _poppler_annot_file_attachment_new(annot);
            break;
        case Annot::typeMovie:
            mapping->annot = _poppler_annot_movie_new(annot);
            break;
        case Annot::typeScreen:
            mapping->annot = _poppler_annot_screen_new(page->document, annot);
            break;
        case Annot::typeLine:
            mapping->annot = _poppler_annot_line_new(annot);
            break;
        case Annot::typeSquare:
            mapping->annot = _poppler_annot_square_new(annot);
            break;
        case Annot::typeCircle:
            mapping->annot = _poppler_annot_circle_new(annot);
            break;
        case Annot::typeHighlight:
        case Annot::typeUnderline:
        case Annot::typeSquiggly:
        case Annot::typeStrikeOut:
            mapping->annot = _poppler_annot_text_markup_new(annot);
            break;
        case Annot::typeStamp:
            mapping->annot = _poppler_annot_stamp_new(annot);
            break;
        default:
            mapping->annot = _poppler_annot_new(annot);
            break;
        }

        const PDFRectangle &annot_rect = annot->getRect();
        rect.x1 = annot_rect.x1 - crop_box->x1;
        rect.y1 = annot_rect.y1 - crop_box->y1;
        rect.x2 = annot_rect.x2 - crop_box->x1;
        rect.y2 = annot_rect.y2 - crop_box->y1;

        rotation = page->page->getRotate();

        if (rotation == 0 || !SUPPORTED_ROTATION(rotation)) { /* zero or unknown rotation */
            mapping->area.x1 = rect.x1;
            mapping->area.y1 = rect.y1;
            mapping->area.x2 = rect.x2;
            mapping->area.y2 = rect.y2;
        } else {
            gdouble annot_height = rect.y2 - rect.y1;
            gdouble annot_width = rect.x2 - rect.x1;

            if (flag_no_rotate) {
                if (rotation == 90) {
                    mapping->area.x1 = rect.y2;
                    mapping->area.y1 = height - (rect.x1 + annot_height);
                    mapping->area.x2 = rect.y2 + annot_width;
                    mapping->area.y2 = height - rect.x1;
                } else if (rotation == 180) {
                    mapping->area.x1 = width - rect.x1;
                    mapping->area.x2 = MIN(mapping->area.x1 + annot_width, width);
                    mapping->area.y2 = height - rect.y2;
                    mapping->area.y1 = MAX(0, mapping->area.y2 - annot_height);
                } else if (rotation == 270) {
                    mapping->area.x1 = width - rect.y2;
                    mapping->area.x2 = MIN(mapping->area.x1 + annot_width, width);
                    mapping->area.y2 = rect.x1;
                    mapping->area.y1 = MAX(0, mapping->area.y2 - annot_height);
                }
            } else { /* !flag_no_rotate */
                if (rotation == 90) {
                    mapping->area.x1 = rect.y1;
                    mapping->area.y1 = height - rect.x2;
                    mapping->area.x2 = mapping->area.x1 + annot_height;
                    mapping->area.y2 = mapping->area.y1 + annot_width;
                } else if (rotation == 180) {
                    mapping->area.x1 = width - rect.x2;
                    mapping->area.y1 = height - rect.y2;
                    mapping->area.x2 = mapping->area.x1 + annot_width;
                    mapping->area.y2 = mapping->area.y1 + annot_height;
                } else if (rotation == 270) {
                    mapping->area.x1 = width - rect.y2;
                    mapping->area.y1 = rect.x1;
                    mapping->area.x2 = mapping->area.x1 + annot_height;
                    mapping->area.y2 = mapping->area.y1 + annot_width;
                }
            }
        }

        map_list = g_list_prepend(map_list, mapping);
    }

    return g_list_reverse(map_list);
}

/**
 * poppler_page_free_annot_mapping:
 * @list: (element-type PopplerAnnotMapping): A list of
 *   #PopplerAnnotMapping<!-- -->s
 *
 * Frees a list of #PopplerAnnotMapping<!-- -->s allocated by
 * poppler_page_get_annot_mapping().  It also unreferences the #PopplerAnnot<!-- -->s
 * that each mapping contains, so if you want to keep them around, you need to
 * reference them with g_object_ref().
 **/
void poppler_page_free_annot_mapping(GList *list)
{
    if (G_UNLIKELY(!list)) {
        return;
    }

    g_list_free_full(list, (GDestroyNotify)poppler_annot_mapping_free);
}

/* Adds or removes (according to @add parameter) the passed in @crop_box from the
 * passed in @quads and returns it as a new #AnnotQuadrilaterals object */
AnnotQuadrilaterals *new_quads_from_offset_cropbox(const PDFRectangle *crop_box, AnnotQuadrilaterals *quads, gboolean add)
{
    int len = quads->getQuadrilateralsLength();
    auto quads_array = std::make_unique<AnnotQuadrilaterals::AnnotQuadrilateral[]>(len);
    for (int i = 0; i < len; i++) {
        if (add) {
            quads_array[i] = AnnotQuadrilaterals::AnnotQuadrilateral(quads->getX1(i) + crop_box->x1, quads->getY1(i) + crop_box->y1, quads->getX2(i) + crop_box->x1, quads->getY2(i) + crop_box->y1, quads->getX3(i) + crop_box->x1,
                                                                     quads->getY3(i) + crop_box->y1, quads->getX4(i) + crop_box->x1, quads->getY4(i) + crop_box->y1);
        } else {
            quads_array[i] = AnnotQuadrilaterals::AnnotQuadrilateral(quads->getX1(i) - crop_box->x1, quads->getY1(i) - crop_box->y1, quads->getX2(i) - crop_box->x1, quads->getY2(i) - crop_box->y1, quads->getX3(i) - crop_box->x1,
                                                                     quads->getY3(i) - crop_box->y1, quads->getX4(i) - crop_box->x1, quads->getY4(i) - crop_box->y1);
        }
    }

    return new AnnotQuadrilaterals(std::move(quads_array), len);
}

/* This function rotates the passed-in @x @y point with the page rotation.
 * In other words, it moves the point to where it'll be located in a displayed document */
void _page_rotate_xy(Page *page, double *x, double *y)
{
    double page_width, page_height, temp;
    gint rotation = page->getRotate();

    if (rotation == 90 || rotation == 270) {
        page_height = page->getCropWidth();
        page_width = page->getCropHeight();
    } else {
        page_width = page->getCropWidth();
        page_height = page->getCropHeight();
    }

    if (rotation == 90) {
        temp = *x;
        *x = *y;
        *y = page_height - temp;
    } else if (rotation == 180) {
        *x = page_width - *x;
        *y = page_height - *y;
    } else if (rotation == 270) {
        temp = *x;
        *x = page_width - *y;
        *y = temp;
    }
}

/* This function undoes the rotation of @page in the passed-in @x @y point.
 * In other words, it moves the point to where it'll be located if @page
 * was put to zero rotation (unrotated) */
void _page_unrotate_xy(Page *page, double *x, double *y)
{
    double page_width, page_height, temp;
    gint rotation = page->getRotate();

    if (rotation == 90 || rotation == 270) {
        page_height = page->getCropWidth();
        page_width = page->getCropHeight();
    } else {
        page_width = page->getCropWidth();
        page_height = page->getCropHeight();
    }

    if (rotation == 90) {
        temp = *x;
        *x = page_height - *y;
        *y = temp;
    } else if (rotation == 180) {
        *x = page_width - *x;
        *y = page_height - *y;
    } else if (rotation == 270) {
        temp = *x;
        *x = *y;
        *y = page_width - temp;
    }
}

AnnotQuadrilaterals *_page_new_quads_unrotated(Page *page, AnnotQuadrilaterals *quads)
{
    double x1, y1, x2, y2, x3, y3, x4, y4;
    int len = quads->getQuadrilateralsLength();
    auto quads_array = std::make_unique<AnnotQuadrilaterals::AnnotQuadrilateral[]>(len);

    for (int i = 0; i < len; i++) {
        x1 = quads->getX1(i);
        y1 = quads->getY1(i);
        x2 = quads->getX2(i);
        y2 = quads->getY2(i);
        x3 = quads->getX3(i);
        y3 = quads->getY3(i);
        x4 = quads->getX4(i);
        y4 = quads->getY4(i);

        _page_unrotate_xy(page, &x1, &y1);
        _page_unrotate_xy(page, &x2, &y2);
        _page_unrotate_xy(page, &x3, &y3);
        _page_unrotate_xy(page, &x4, &y4);

        quads_array[i] = AnnotQuadrilaterals::AnnotQuadrilateral(x1, y1, x2, y2, x3, y3, x4, y4);
    }

    return new AnnotQuadrilaterals(std::move(quads_array), len);
}

/* @x1 @y1 @x2 @y2 are both 'in' and 'out' parameters, representing
 * the diagonal of a rectangle which is the 'rect' area of @annot
 * which is inside @page.
 *
 * If @page is unrotated (i.e. has zero rotation) this function does
 * nothing, otherwise this function un-rotates the passed-in rect
 * coords according to @page rotation so as the returned coords are
 * those of the rect if page was put to zero rotation (unrotated).
 * This is mandated by PDF spec when saving annotation coords (see
 * 8.4.2 Annotation Flags) which also explains the special rotation
 * that needs to be done when @annot has the flagNoRotate set to
 * true, which this function follows. */
void _unrotate_rect_for_annot_and_page(Page *page, Annot *annot, double *x1, double *y1, double *x2, double *y2)
{
    gboolean flag_no_rotate;

    if (!SUPPORTED_ROTATION(page->getRotate())) {
        return;
    }
    /* Normalize received rect diagonal to be from UpperLeft to BottomRight,
     * as our algorithm follows that */
    if (*y2 > *y1) {
        double temp = *y1;
        *y1 = *y2;
        *y2 = temp;
    }
    if (G_UNLIKELY(*x1 > *x2)) {
        double temp = *x1;
        *x1 = *x2;
        *x2 = temp;
    }
    flag_no_rotate = annot->getFlags() & Annot::flagNoRotate;
    if (flag_no_rotate) {
        /* For this case rotating just the upperleft point is enough */
        double annot_height = *y1 - *y2;
        double annot_width = *x2 - *x1;
        _page_unrotate_xy(page, x1, y1);
        *x2 = *x1 + annot_width;
        *y2 = *y1 - annot_height;
    } else {
        _page_unrotate_xy(page, x1, y1);
        _page_unrotate_xy(page, x2, y2);
    }
}

/**
 * poppler_page_add_annot:
 * @page: a #PopplerPage
 * @annot: a #PopplerAnnot to add
 *
 * Adds annotation @annot to @page.
 *
 * Since: 0.16
 */
void poppler_page_add_annot(PopplerPage *page, PopplerAnnot *annot)
{
    double x1, y1, x2, y2;
    gboolean page_is_rotated;
    const PDFRectangle *crop_box;
    const PDFRectangle *page_crop_box;

    g_return_if_fail(POPPLER_IS_PAGE(page));
    g_return_if_fail(POPPLER_IS_ANNOT(annot));

    /* Add the page's cropBox to the coordinates of rect field of annot */
    page_crop_box = page->page->getCropBox();
    annot->annot->getRect(&x1, &y1, &x2, &y2);

    page_is_rotated = SUPPORTED_ROTATION(page->page->getRotate());
    if (page_is_rotated) {
        /* annot is inside a rotated page, as core poppler rect must be saved
         * un-rotated, let's proceed to un-rotate rect before saving */
        _unrotate_rect_for_annot_and_page(page->page, annot->annot.get(), &x1, &y1, &x2, &y2);
    }

    annot->annot->setRect(x1 + page_crop_box->x1, y1 + page_crop_box->y1, x2 + page_crop_box->x1, y2 + page_crop_box->y1);

    auto *annot_markup = dynamic_cast<AnnotTextMarkup *>(annot->annot.get());
    if (annot_markup) {
        crop_box = _poppler_annot_get_cropbox(annot);
        if (crop_box) {
            /* Handle hypothetical case of annot being added is already existing on a prior page, so
             * first remove cropbox of the prior page before adding cropbox of the new page later */
            AnnotQuadrilaterals *quads = new_quads_from_offset_cropbox(crop_box, annot_markup->getQuadrilaterals(), FALSE);
            annot_markup->setQuadrilaterals(*quads);
            delete quads;
        }
        if (page_is_rotated) {
            /* Quadrilateral's coords need to be saved un-rotated (same as rect coords) */
            AnnotQuadrilaterals *quads = _page_new_quads_unrotated(page->page, annot_markup->getQuadrilaterals());
            annot_markup->setQuadrilaterals(*quads);
            delete quads;
        }
        /* Add to annot's quadrilaterals the offset for the cropbox of the new page */
        AnnotQuadrilaterals *quads = new_quads_from_offset_cropbox(page_crop_box, annot_markup->getQuadrilaterals(), TRUE);
        annot_markup->setQuadrilaterals(*quads);
        delete quads;
    }

    page->page->addAnnot(annot->annot);
}

/**
 * poppler_page_remove_annot:
 * @page: a #PopplerPage
 * @annot: a #PopplerAnnot to remove
 *
 * Removes annotation @annot from @page
 *
 * Since: 0.22
 */
void poppler_page_remove_annot(PopplerPage *page, PopplerAnnot *annot)
{
    g_return_if_fail(POPPLER_IS_PAGE(page));
    g_return_if_fail(POPPLER_IS_ANNOT(annot));

    page->page->removeAnnot(annot->annot);
}

/* PopplerRectangle type */

G_DEFINE_BOXED_TYPE(PopplerRectangle, poppler_rectangle, poppler_rectangle_copy, poppler_rectangle_free)

static PopplerRectangleExtended *poppler_rectangle_extended_new()
{
    return g_slice_new0(PopplerRectangleExtended);
}

PopplerRectangle *poppler_rectangle_new_from_pdf_rectangle(const PDFRectangle *rect)
{
    auto *r = poppler_rectangle_extended_new();
    r->x1 = rect->x1;
    r->y1 = rect->y1;
    r->x2 = rect->x2;
    r->y2 = rect->y2;

    return reinterpret_cast<PopplerRectangle *>(r);
}

/**
 * poppler_rectangle_new:
 *
 * Creates a new #PopplerRectangle
 *
 * Returns: a new #PopplerRectangle, use poppler_rectangle_free() to free it
 */
PopplerRectangle *poppler_rectangle_new(void)
{
    return reinterpret_cast<PopplerRectangle *>(poppler_rectangle_extended_new());
}

/**
 * poppler_rectangle_copy:
 * @rectangle: a #PopplerRectangle to copy
 *
 * Creates a copy of @rectangle.
 *
 * Note that you must only use this function on an allocated PopplerRectangle, as
 * returned by poppler_rectangle_new(), poppler_rectangle_copy(), or the list elements
 * returned from poppler_page_find_text() or poppler_page_find_text_with_options().
 * Returns: a new allocated copy of @rectangle
 */
PopplerRectangle *poppler_rectangle_copy(PopplerRectangle *rectangle)
{
    g_return_val_if_fail(rectangle != nullptr, NULL);

    auto *ext_rectangle = reinterpret_cast<PopplerRectangleExtended *>(rectangle);
    return reinterpret_cast<PopplerRectangle *>(g_slice_dup(PopplerRectangleExtended, ext_rectangle));
}

/**
 * poppler_rectangle_free:
 * @rectangle: a #PopplerRectangle
 *
 * Frees the given #PopplerRectangle.
 *
 * Note that you must only use this function on an allocated PopplerRectangle, as
 * returned by poppler_rectangle_new(), poppler_rectangle_copy(), or the list elements
 * returned from poppler_page_find_text() or poppler_page_find_text_with_options().
 */
void poppler_rectangle_free(PopplerRectangle *rectangle)
{
    auto *ext_rectangle = reinterpret_cast<PopplerRectangleExtended *>(rectangle);
    g_slice_free(PopplerRectangleExtended, ext_rectangle);
}

/**
 * poppler_rectangle_find_get_match_continued:
 * @rectangle: a #PopplerRectangle
 *
 * When using poppler_page_find_text_with_options() with the
 * %POPPLER_FIND_MULTILINE flag, a match may span more than one line
 * and thus consist of more than one rectangle. Every rectangle belonging
 * to the same match will return %TRUE from this function, except for
 * the last rectangle, where this function will return %FALSE.
 *
 * Note that you must only call this function on a #PopplerRectangle
 * returned in the list from poppler_page_find_text() or
 * poppler_page_find_text_with_options().
 *
 * Returns: whether there are more rectangles belonging to the same match
 *
 * Since: 21.05.0
 */
gboolean poppler_rectangle_find_get_match_continued(const PopplerRectangle *rectangle)
{
    g_return_val_if_fail(rectangle != nullptr, false);

    const auto *ext_rectangle = reinterpret_cast<const PopplerRectangleExtended *>(rectangle);
    return ext_rectangle->match_continued;
}

/**
 * poppler_rectangle_find_get_ignored_hyphen:
 * @rectangle: a #PopplerRectangle
 *
 * When using poppler_page_find_text_with_options() with the
 * %POPPLER_FIND_MULTILINE flag, a match may span more than one line,
 * and may have been formed by ignoring a hyphen at the end of the line.
 * When this happens at the end of the line corresponding to @rectangle,
 * this function returns %TRUE (and then poppler_rectangle_find_get_match_continued()
 * will also return %TRUE); otherwise it returns %FALSE.
 *
 * Note that you must only call this function on a #PopplerRectangle
 * returned in the list from poppler_page_find_text() or
 * poppler_page_find_text_with_options().
 *
 * Returns: whether a hyphen was ignored at the end of the line corresponding to @rectangle.
 *
 * Since: 21.05.0
 */
gboolean poppler_rectangle_find_get_ignored_hyphen(const PopplerRectangle *rectangle)
{
    g_return_val_if_fail(rectangle != nullptr, false);

    const auto *ext_rectangle = reinterpret_cast<const PopplerRectangleExtended *>(rectangle);
    return ext_rectangle->ignored_hyphen;
}

G_DEFINE_BOXED_TYPE(PopplerPoint, poppler_point, poppler_point_copy, poppler_point_free)

/**
 * poppler_point_new:
 *
 * Creates a new #PopplerPoint. It must be freed with poppler_point_free() after use.
 *
 * Returns: a new #PopplerPoint
 *
 * Since: 0.26
 **/
PopplerPoint *poppler_point_new(void)
{
    return g_slice_new0(PopplerPoint);
}

/**
 * poppler_point_copy:
 * @point: a #PopplerPoint to copy
 *
 * Creates a copy of @point. The copy must be freed with poppler_point_free()
 * after use.
 *
 * Returns: a new allocated copy of @point
 *
 * Since: 0.26
 **/
PopplerPoint *poppler_point_copy(PopplerPoint *point)
{
    g_return_val_if_fail(point != nullptr, NULL);

    return g_slice_dup(PopplerPoint, point);
}

/**
 * poppler_point_free:
 * @point: a #PopplerPoint
 *
 * Frees the memory used by @point
 *
 * Since: 0.26
 **/
void poppler_point_free(PopplerPoint *point)
{
    g_slice_free(PopplerPoint, point);
}

/* PopplerQuadrilateral type */

G_DEFINE_BOXED_TYPE(PopplerQuadrilateral, poppler_quadrilateral, poppler_quadrilateral_copy, poppler_quadrilateral_free)

/**
 * poppler_quadrilateral_new:
 *
 * Creates a new #PopplerQuadrilateral. It must be freed with poppler_quadrilateral_free() after use.
 *
 * Returns: a new #PopplerQuadrilateral.
 *
 * Since: 0.26
 **/
PopplerQuadrilateral *poppler_quadrilateral_new(void)
{
    return g_slice_new0(PopplerQuadrilateral);
}

/**
 * poppler_quadrilateral_copy:
 * @quad: a #PopplerQuadrilateral to copy
 *
 * Creates a copy of @quad. The copy must be freed with poppler_quadrilateral_free() after use.
 *
 * Returns: a new allocated copy of @quad
 *
 * Since: 0.26
 **/
PopplerQuadrilateral *poppler_quadrilateral_copy(PopplerQuadrilateral *quad)
{
    g_return_val_if_fail(quad != nullptr, NULL);

    return g_slice_dup(PopplerQuadrilateral, quad);
}

/**
 * poppler_quadrilateral_free:
 * @quad: a #PopplerQuadrilateral
 *
 * Frees the memory used by @quad
 *
 * Since: 0.26
 **/
void poppler_quadrilateral_free(PopplerQuadrilateral *quad)
{
    g_slice_free(PopplerQuadrilateral, quad);
}

/* PopplerTextAttributes type */

G_DEFINE_BOXED_TYPE(PopplerTextAttributes, poppler_text_attributes, poppler_text_attributes_copy, poppler_text_attributes_free)

/**
 * poppler_text_attributes_new:
 *
 * Creates a new #PopplerTextAttributes
 *
 * Returns: a new #PopplerTextAttributes, use poppler_text_attributes_free() to free it
 *
 * Since: 0.18
 */
PopplerTextAttributes *poppler_text_attributes_new(void)
{
    return (PopplerTextAttributes *)g_slice_new0(PopplerTextAttributes);
}

static gchar *get_font_name_from_word(const TextWord *word, gint word_i)
{
    const GooString *font_name = word->getFontName(word_i);
    const gchar *name;
    gboolean subset;
    size_t i;

    if (!font_name || font_name->empty()) {
        return g_strdup("Default");
    }

    // check for a font subset name: capital letters followed by a '+' sign
    for (i = 0; i < font_name->size(); ++i) {
        if (font_name->getChar(i) < 'A' || font_name->getChar(i) > 'Z') {
            break;
        }
    }
    subset = i > 0 && i < font_name->size() && font_name->getChar(i) == '+';
    name = font_name->c_str();
    if (subset) {
        name += i + 1;
    }

    return g_strdup(name);
}

/*
 * Allocates a new PopplerTextAttributes with word attributes
 */
static PopplerTextAttributes *poppler_text_attributes_new_from_word(const TextWord *word, gint i)
{
    PopplerTextAttributes *attrs = poppler_text_attributes_new();
    gdouble r, g, b;

    attrs->font_name = get_font_name_from_word(word, i);
    attrs->font_size = word->getFontSize();
    attrs->is_underlined = word->isUnderlined();
    word->getColor(&r, &g, &b);
    attrs->color.red = (int)(r * 65535. + 0.5);
    attrs->color.green = (int)(g * 65535. + 0.5);
    attrs->color.blue = (int)(b * 65535. + 0.5);

    return attrs;
}

/**
 * poppler_text_attributes_copy:
 * @text_attrs: a #PopplerTextAttributes to copy
 *
 * Creates a copy of @text_attrs
 *
 * Returns: a new allocated copy of @text_attrs
 *
 * Since: 0.18
 */
PopplerTextAttributes *poppler_text_attributes_copy(PopplerTextAttributes *text_attrs)
{
    PopplerTextAttributes *attrs;

    attrs = g_slice_dup(PopplerTextAttributes, text_attrs);
    attrs->font_name = g_strdup(text_attrs->font_name);
    return attrs;
}

/**
 * poppler_text_attributes_free:
 * @text_attrs: a #PopplerTextAttributes
 *
 * Frees the given #PopplerTextAttributes
 *
 * Since: 0.18
 */
void poppler_text_attributes_free(PopplerTextAttributes *text_attrs)
{
    g_free(text_attrs->font_name);
    g_slice_free(PopplerTextAttributes, text_attrs);
}

/**
 * SECTION:poppler-color
 * @short_description: Colors
 * @title: PopplerColor
 */

/* PopplerColor type */
G_DEFINE_BOXED_TYPE(PopplerColor, poppler_color, poppler_color_copy, poppler_color_free)

/**
 * poppler_color_new:
 *
 * Creates a new #PopplerColor
 *
 * Returns: a new #PopplerColor, use poppler_color_free() to free it
 */
PopplerColor *poppler_color_new(void)
{
    return (PopplerColor *)g_new0(PopplerColor, 1);
}

/**
 * poppler_color_copy:
 * @color: a #PopplerColor to copy
 *
 * Creates a copy of @color
 *
 * Returns: a new allocated copy of @color
 */
PopplerColor *poppler_color_copy(PopplerColor *color)
{
    PopplerColor *new_color;

    new_color = g_new(PopplerColor, 1);
    *new_color = *color;

    return new_color;
}

/**
 * poppler_color_free:
 * @color: a #PopplerColor
 *
 * Frees the given #PopplerColor
 */
void poppler_color_free(PopplerColor *color)
{
    g_free(color);
}

/* PopplerLinkMapping type */
G_DEFINE_BOXED_TYPE(PopplerLinkMapping, poppler_link_mapping, poppler_link_mapping_copy, poppler_link_mapping_free)

/**
 * poppler_link_mapping_new:
 *
 * Creates a new #PopplerLinkMapping
 *
 * Returns: a new #PopplerLinkMapping, use poppler_link_mapping_free() to free it
 */
PopplerLinkMapping *poppler_link_mapping_new(void)
{
    return g_slice_new0(PopplerLinkMapping);
}

/**
 * poppler_link_mapping_copy:
 * @mapping: a #PopplerLinkMapping to copy
 *
 * Creates a copy of @mapping
 *
 * Returns: a new allocated copy of @mapping
 */
PopplerLinkMapping *poppler_link_mapping_copy(PopplerLinkMapping *mapping)
{
    PopplerLinkMapping *new_mapping;

    new_mapping = g_slice_dup(PopplerLinkMapping, mapping);

    if (new_mapping->action) {
        new_mapping->action = poppler_action_copy(new_mapping->action);
    }

    return new_mapping;
}

/**
 * poppler_link_mapping_free:
 * @mapping: a #PopplerLinkMapping
 *
 * Frees the given #PopplerLinkMapping
 */
void poppler_link_mapping_free(PopplerLinkMapping *mapping)
{
    if (G_UNLIKELY(!mapping)) {
        return;
    }

    if (mapping->action) {
        poppler_action_free(mapping->action);
    }

    g_slice_free(PopplerLinkMapping, mapping);
}

/* Poppler Image mapping type */
G_DEFINE_BOXED_TYPE(PopplerImageMapping, poppler_image_mapping, poppler_image_mapping_copy, poppler_image_mapping_free)

/**
 * poppler_image_mapping_new:
 *
 * Creates a new #PopplerImageMapping
 *
 * Returns: a new #PopplerImageMapping, use poppler_image_mapping_free() to free it
 */
PopplerImageMapping *poppler_image_mapping_new(void)
{
    return g_slice_new0(PopplerImageMapping);
}

/**
 * poppler_image_mapping_copy:
 * @mapping: a #PopplerImageMapping to copy
 *
 * Creates a copy of @mapping
 *
 * Returns: a new allocated copy of @mapping
 */
PopplerImageMapping *poppler_image_mapping_copy(PopplerImageMapping *mapping)
{
    return g_slice_dup(PopplerImageMapping, mapping);
}

/**
 * poppler_image_mapping_free:
 * @mapping: a #PopplerImageMapping
 *
 * Frees the given #PopplerImageMapping
 */
void poppler_image_mapping_free(PopplerImageMapping *mapping)
{
    g_slice_free(PopplerImageMapping, mapping);
}

/* Page Transition */
G_DEFINE_BOXED_TYPE(PopplerPageTransition, poppler_page_transition, poppler_page_transition_copy, poppler_page_transition_free)

/**
 * poppler_page_transition_new:
 *
 * Creates a new #PopplerPageTransition
 *
 * Returns: a new #PopplerPageTransition, use poppler_page_transition_free() to free it
 */
PopplerPageTransition *poppler_page_transition_new(void)
{
    return (PopplerPageTransition *)g_new0(PopplerPageTransition, 1);
}

/**
 * poppler_page_transition_copy:
 * @transition: a #PopplerPageTransition to copy
 *
 * Creates a copy of @transition
 *
 * Returns: a new allocated copy of @transition
 */
PopplerPageTransition *poppler_page_transition_copy(PopplerPageTransition *transition)
{
    PopplerPageTransition *new_transition;

    new_transition = poppler_page_transition_new();
    *new_transition = *transition;

    return new_transition;
}

/**
 * poppler_page_transition_free:
 * @transition: a #PopplerPageTransition
 *
 * Frees the given #PopplerPageTransition
 */
void poppler_page_transition_free(PopplerPageTransition *transition)
{
    g_free(transition);
}

/* Form Field Mapping Type */
G_DEFINE_BOXED_TYPE(PopplerFormFieldMapping, poppler_form_field_mapping, poppler_form_field_mapping_copy, poppler_form_field_mapping_free)

/**
 * poppler_form_field_mapping_new:
 *
 * Creates a new #PopplerFormFieldMapping
 *
 * Returns: a new #PopplerFormFieldMapping, use poppler_form_field_mapping_free() to free it
 */
PopplerFormFieldMapping *poppler_form_field_mapping_new(void)
{
    return g_slice_new0(PopplerFormFieldMapping);
}

/**
 * poppler_form_field_mapping_copy:
 * @mapping: a #PopplerFormFieldMapping to copy
 *
 * Creates a copy of @mapping
 *
 * Returns: a new allocated copy of @mapping
 */
PopplerFormFieldMapping *poppler_form_field_mapping_copy(PopplerFormFieldMapping *mapping)
{
    PopplerFormFieldMapping *new_mapping;

    new_mapping = g_slice_dup(PopplerFormFieldMapping, mapping);

    if (mapping->field) {
        new_mapping->field = (PopplerFormField *)g_object_ref(mapping->field);
    }

    return new_mapping;
}

/**
 * poppler_form_field_mapping_free:
 * @mapping: a #PopplerFormFieldMapping
 *
 * Frees the given #PopplerFormFieldMapping
 */
void poppler_form_field_mapping_free(PopplerFormFieldMapping *mapping)
{
    if (G_UNLIKELY(!mapping)) {
        return;
    }

    if (mapping->field) {
        g_object_unref(mapping->field);
    }

    g_slice_free(PopplerFormFieldMapping, mapping);
}

/* PopplerAnnot Mapping Type */
G_DEFINE_BOXED_TYPE(PopplerAnnotMapping, poppler_annot_mapping, poppler_annot_mapping_copy, poppler_annot_mapping_free)

/**
 * poppler_annot_mapping_new:
 *
 * Creates a new #PopplerAnnotMapping
 *
 * Returns: a new #PopplerAnnotMapping, use poppler_annot_mapping_free() to free it
 */
PopplerAnnotMapping *poppler_annot_mapping_new(void)
{
    return g_slice_new0(PopplerAnnotMapping);
}

/**
 * poppler_annot_mapping_copy:
 * @mapping: a #PopplerAnnotMapping to copy
 *
 * Creates a copy of @mapping
 *
 * Returns: a new allocated copy of @mapping
 */
PopplerAnnotMapping *poppler_annot_mapping_copy(PopplerAnnotMapping *mapping)
{
    PopplerAnnotMapping *new_mapping;

    new_mapping = g_slice_dup(PopplerAnnotMapping, mapping);

    if (mapping->annot) {
        new_mapping->annot = (PopplerAnnot *)g_object_ref(mapping->annot);
    }

    return new_mapping;
}

/**
 * poppler_annot_mapping_free:
 * @mapping: a #PopplerAnnotMapping
 *
 * Frees the given #PopplerAnnotMapping
 */
void poppler_annot_mapping_free(PopplerAnnotMapping *mapping)
{
    if (G_UNLIKELY(!mapping)) {
        return;
    }

    if (mapping->annot) {
        g_object_unref(mapping->annot);
    }

    g_slice_free(PopplerAnnotMapping, mapping);
}

/**
 * poppler_page_get_crop_box:
 * @page: a #PopplerPage
 * @rect: (out): a #PopplerRectangle to fill
 *
 * Retrurns the crop box of @page
 */
void poppler_page_get_crop_box(PopplerPage *page, PopplerRectangle *rect)
{
    const PDFRectangle *cropBox = page->page->getCropBox();

    rect->x1 = cropBox->x1;
    rect->x2 = cropBox->x2;
    rect->y1 = cropBox->y1;
    rect->y2 = cropBox->y2;
}

/*
 * poppler_page_get_bounding_box:
 * @page: A #PopplerPage
 * @rect: (out) return the bounding box of the page
 *
 * Returns the bounding box of the page, a rectangle enclosing all text, vector
 * graphics (lines, rectangles and curves) and raster images in the page.
 * Includes invisible text but not (yet) annotations like highlights and form
 * elements.
 *
 * Return value: %TRUE if the page contains graphics, %FALSE otherwise
 *
 * Since: 0.88
 */
gboolean poppler_page_get_bounding_box(PopplerPage *page, PopplerRectangle *rect)
{
    BBoxOutputDev *bb_out;
    bool hasGraphics;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), false);
    g_return_val_if_fail(rect != nullptr, false);

    bb_out = new BBoxOutputDev();

    page->page->displaySlice(bb_out, 72.0, 72.0, 0, false, /* useMediaBox */
                             true, /* Crop */
                             -1, -1, -1, -1, false, /* printing */
                             nullptr, nullptr, nullptr, nullptr);
    hasGraphics = bb_out->getHasGraphics();
    if (hasGraphics) {
        rect->x1 = bb_out->getX1();
        rect->y1 = bb_out->getY1();
        rect->x2 = bb_out->getX2();
        rect->y2 = bb_out->getY2();
    }

    delete bb_out;
    return hasGraphics;
}

/**
 * poppler_page_get_text_layout:
 * @page: A #PopplerPage
 * @rectangles: (out) (array length=n_rectangles) (transfer container): return location for an array of #PopplerRectangle
 * @n_rectangles: (out): length of returned array
 *
 * Obtains the layout of the text as a list of #PopplerRectangle
 * This array must be freed with g_free() when done.
 *
 * The position in the array represents an offset in the text returned by
 * poppler_page_get_text()
 *
 * See also poppler_page_get_text_layout_for_area().
 *
 * Return value: %TRUE if the page contains text, %FALSE otherwise
 *
 * Since: 0.16
 **/
gboolean poppler_page_get_text_layout(PopplerPage *page, PopplerRectangle **rectangles, guint *n_rectangles)
{
    PopplerRectangle selection = { 0, 0, 0, 0 };

    g_return_val_if_fail(POPPLER_IS_PAGE(page), FALSE);

    poppler_page_get_size(page, &selection.x2, &selection.y2);

    return poppler_page_get_text_layout_for_area(page, &selection, rectangles, n_rectangles);
}

/**
 * poppler_page_get_text_layout_for_area:
 * @page: A #PopplerPage
 * @area: a #PopplerRectangle
 * @rectangles: (out) (array length=n_rectangles) (transfer container): return location for an array of #PopplerRectangle
 * @n_rectangles: (out): length of returned array
 *
 * Obtains the layout of the text contained in @area as a list of #PopplerRectangle
 * This array must be freed with g_free() when done.
 *
 * The position in the array represents an offset in the text returned by
 * poppler_page_get_text_for_area()
 *
 * Return value: %TRUE if the page contains text, %FALSE otherwise
 *
 * Since: 0.26
 **/
gboolean poppler_page_get_text_layout_for_area(PopplerPage *page, PopplerRectangle *area, PopplerRectangle **rectangles, guint *n_rectangles)
{
    TextPage *text;
    PopplerRectangle *rect;
    PDFRectangle selection;
    guint offset = 0;
    guint n_rects = 0;
    gdouble x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    gdouble x3, y3, x4, y4;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), FALSE);
    g_return_val_if_fail(area != nullptr, FALSE);

    *n_rectangles = 0;

    selection.x1 = area->x1;
    selection.y1 = area->y1;
    selection.x2 = area->x2;
    selection.y2 = area->y2;

    text = poppler_page_get_text_page(page);
    std::vector<std::vector<std::unique_ptr<TextWordSelection>>> word_list = text->getSelectionWords(&selection, selectionStyleGlyph);
    if (word_list.empty()) {
        return FALSE;
    }

    n_rects += word_list.size() - 1;
    for (const std::vector<std::unique_ptr<TextWordSelection>> &line_words : word_list) {
        n_rects += line_words.size() - 1;
        for (std::size_t j = 0; j < line_words.size(); j++) {
            const TextWordSelection *word_sel = line_words[j].get();
            n_rects += word_sel->getEnd() - word_sel->getBegin();
            if (!word_sel->getWord()->hasSpaceAfter() && j < line_words.size() - 1) {
                n_rects--;
            }
        }
    }

    *rectangles = g_new(PopplerRectangle, n_rects);
    *n_rectangles = n_rects;

    for (size_t i = 0; i < word_list.size(); i++) {
        std::vector<std::unique_ptr<TextWordSelection>> &line_words = word_list[i];
        for (std::size_t j = 0; j < line_words.size(); j++) {
            TextWordSelection *word_sel = line_words[j].get();
            const TextWord *word = word_sel->getWord();
            int end = word_sel->getEnd();

            for (int k = word_sel->getBegin(); k < end; k++) {
                rect = *rectangles + offset;
                word->getCharBBox(k, &(rect->x1), &(rect->y1), &(rect->x2), &(rect->y2));
                offset++;
            }

            rect = *rectangles + offset;
            word->getBBox(&x1, &y1, &x2, &y2);

            if (word->hasSpaceAfter() && j < line_words.size() - 1) {
                TextWordSelection *next_word_sel = line_words[j + 1].get();

                next_word_sel->getWord()->getBBox(&x3, &y3, &x4, &y4);
                // space is from one word to other and with the same height as
                // first word.
                rect->x1 = x2;
                rect->y1 = y1;
                rect->x2 = x3;
                rect->y2 = y2;
                offset++;
            }
        }

        if (i < word_list.size() - 1 && offset > 0) {
            // end of line
            rect->x1 = x2;
            rect->y1 = y2;
            rect->x2 = x2;
            rect->y2 = y2;
            offset++;
        }
    }
    return TRUE;
}

/**
 * poppler_page_free_text_attributes:
 * @list: (element-type PopplerTextAttributes): A list of
 *   #PopplerTextAttributes<!-- -->s
 *
 * Frees a list of #PopplerTextAttributes<!-- -->s allocated by
 * poppler_page_get_text_attributes().
 *
 * Since: 0.18
 **/
void poppler_page_free_text_attributes(GList *list)
{
    if (G_UNLIKELY(list == nullptr)) {
        return;
    }

    g_list_free_full(list, (GDestroyNotify)poppler_text_attributes_free);
}

static gboolean word_text_attributes_equal(const TextWord *a, gint ai, const TextWord *b, gint bi)
{
    double ar, ag, ab, br, bg, bb;

    if (!a->getFontInfo(ai)->matches(b->getFontInfo(bi))) {
        return FALSE;
    }

    if (a->getFontSize() != b->getFontSize()) {
        return FALSE;
    }

    if (a->isUnderlined() != b->isUnderlined()) {
        return FALSE;
    }

    a->getColor(&ar, &ag, &ab);
    b->getColor(&br, &bg, &bb);
    return (ar == br && ag == bg && ab == bb);
}

/**
 * poppler_page_get_text_attributes:
 * @page: A #PopplerPage
 *
 * Obtains the attributes of the text as a #GList of #PopplerTextAttributes.
 * This list must be freed with poppler_page_free_text_attributes() when done.
 *
 * Each list element is a #PopplerTextAttributes struct where start_index and
 * end_index indicates the range of text (as returned by poppler_page_get_text())
 * to which text attributes apply.
 *
 * See also poppler_page_get_text_attributes_for_area()
 *
 * Return value: (element-type PopplerTextAttributes) (transfer full): A #GList of #PopplerTextAttributes
 *
 * Since: 0.18
 **/
GList *poppler_page_get_text_attributes(PopplerPage *page)
{
    PopplerRectangle selection = { 0, 0, 0, 0 };

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);

    poppler_page_get_size(page, &selection.x2, &selection.y2);

    return poppler_page_get_text_attributes_for_area(page, &selection);
}

/**
 * poppler_page_get_text_attributes_for_area:
 * @page: A #PopplerPage
 * @area: a #PopplerRectangle
 *
 * Obtains the attributes of the text in @area as a #GList of #PopplerTextAttributes.
 * This list must be freed with poppler_page_free_text_attributes() when done.
 *
 * Each list element is a #PopplerTextAttributes struct where start_index and
 * end_index indicates the range of text (as returned by poppler_page_get_text_for_area())
 * to which text attributes apply.
 *
 * Return value: (element-type PopplerTextAttributes) (transfer full): A #GList of #PopplerTextAttributes
 *
 * Since: 0.26
 **/
GList *poppler_page_get_text_attributes_for_area(PopplerPage *page, PopplerRectangle *area)
{
    TextPage *text;
    PDFRectangle selection;
    PopplerTextAttributes *attrs = nullptr;
    const TextWord *word, *prev_word = nullptr;
    gint word_i, prev_word_i;
    gint offset = 0;
    GList *attributes = nullptr;

    g_return_val_if_fail(POPPLER_IS_PAGE(page), NULL);
    g_return_val_if_fail(area != nullptr, nullptr);

    selection.x1 = area->x1;
    selection.y1 = area->y1;
    selection.x2 = area->x2;
    selection.y2 = area->y2;

    text = poppler_page_get_text_page(page);
    std::vector<std::vector<std::unique_ptr<TextWordSelection>>> word_list = text->getSelectionWords(&selection, selectionStyleGlyph);
    if (word_list.empty()) {
        return nullptr;
    }

    for (size_t i = 0; i < word_list.size(); i++) {
        std::vector<std::unique_ptr<TextWordSelection>> &line_words = word_list[i];
        for (std::size_t j = 0; j < line_words.size(); j++) {
            TextWordSelection *word_sel = line_words[j].get();
            int end = word_sel->getEnd();

            word = word_sel->getWord();

            for (word_i = word_sel->getBegin(); word_i < end; word_i++) {
                if (!prev_word || !word_text_attributes_equal(word, word_i, prev_word, prev_word_i)) {
                    attrs = poppler_text_attributes_new_from_word(word, word_i);
                    attrs->start_index = offset;
                    attributes = g_list_prepend(attributes, attrs);
                }
                attrs->end_index = offset;
                offset++;
                prev_word = word;
                prev_word_i = word_i;
            }

            if (word->hasSpaceAfter() && j < line_words.size() - 1) {
                attrs->end_index = offset;
                offset++;
            }
        }

        if (i < word_list.size() - 1) {
            attrs->end_index = offset;
            offset++;
        }
    }
    return g_list_reverse(attributes);
}
