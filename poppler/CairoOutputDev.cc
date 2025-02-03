//========================================================================
//
// CairoOutputDev.cc
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, Inc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005-2008 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005, 2009, 2012, 2017-2021, 2023, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2005 Nickolay V. Shmyrev <nshmyrev@yandex.ru>
// Copyright (C) 2006-2011, 2013, 2014, 2017, 2018 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2008 Carl Worth <cworth@cworth.org>
// Copyright (C) 2008-2018, 2021-2024 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2008 Michael Vrable <mvrable@cs.ucsd.edu>
// Copyright (C) 2008, 2009 Chris Wilson <chris@chris-wilson.co.uk>
// Copyright (C) 2008, 2012 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2009, 2010 David Benjamin <davidben@mit.edu>
// Copyright (C) 2011-2014 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2012 Patrick Pfeifer <p2000@mailinator.com>
// Copyright (C) 2012, 2015, 2016 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2015 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018, 2020 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019, 2020, 2022 Marek Kasik <mkasik@redhat.com>
// Copyright (C) 2020 Michal <sudolskym@gmail.com>
// Copyright (C) 2020, 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2021 Uli Schlachter <psychon@znc.in>
// Copyright (C) 2021 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2022 Zachary Travis <ztravis@everlaw.com>
// Copyright (C) 2023 Artemy Gordon <artemy.gordon@gmail.com>
// Copyright (C) 2023 Anton Thomasson <antonthomasson@gmail.com>
// Copyright (C) 2024 Vincent Lefevre <vincent@vinc17.net>
// Copyright (C) 2024 Athul Raj Kollareth <krathul3152@gmail.com>
// Copyright (C) 2024, 2025 Nelson Benítez León <nbenitezl@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cairo.h>

#include "goo/gfile.h"
#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "Gfx.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "Page.h"
#include "Link.h"
#include "FontEncodingTables.h"
#include "PDFDocEncoding.h"
#include <fofi/FoFiTrueType.h>
#include <splash/SplashBitmap.h>
#include "CairoOutputDev.h"
#include "CairoFontEngine.h"
#include "CairoRescaleBox.h"
#include "UnicodeMap.h"
#include "UTF.h"
#include "JBIG2Stream.h"
//------------------------------------------------------------------------

// #define LOG_CAIRO

// To limit memory usage and improve performance when printing, limit
// cairo images to this size. 8192 is sufficient for an A2 sized
// 300ppi image.
#define MAX_PRINT_IMAGE_SIZE 8192
// Cairo has a max size for image surfaces due to their fixed-point
// coordinate handling, namely INT16_MAX, aka 32767.
#define MAX_CAIRO_IMAGE_SIZE 32767

#ifdef LOG_CAIRO
#    define LOG(x) (x)
#else
#    define LOG(x)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

//------------------------------------------------------------------------
// CairoImage
//------------------------------------------------------------------------

CairoImage::CairoImage(double x1A, double y1A, double x2A, double y2A)
{
    image = nullptr;
    x1 = x1A;
    y1 = y1A;
    x2 = x2A;
    y2 = y2A;
}

CairoImage::~CairoImage()
{
    if (image) {
        cairo_surface_destroy(image);
    }
}

void CairoImage::setImage(cairo_surface_t *i)
{
    if (image) {
        cairo_surface_destroy(image);
    }
    image = cairo_surface_reference(i);
}

//------------------------------------------------------------------------
// CairoOutputDev
//------------------------------------------------------------------------

// We cannot tie the lifetime of an FT_Library object to that of
// CairoOutputDev, since any FT_Faces created with it may end up with a
// reference by Cairo which can be held long after the CairoOutputDev is
// deleted.  The simplest way to avoid problems is to never tear down the
// FT_Library instance; to avoid leaks, just use a single global instance
// initialized the first time it is needed.
FT_Library CairoOutputDev::ft_lib;
std::once_flag CairoOutputDev::ft_lib_once_flag;

CairoOutputDev::CairoOutputDev()
{
    doc = nullptr;

    std::call_once(ft_lib_once_flag, FT_Init_FreeType, &ft_lib);

    fontEngine = nullptr;
    fontEngine_owner = false;
    glyphs = nullptr;
    fill_pattern = nullptr;
    fill_color = {};
    stroke_pattern = nullptr;
    stroke_color = {};
    stroke_opacity = 1.0;
    fill_opacity = 1.0;
    textClipPath = nullptr;
    strokePathClip = nullptr;
    cairo = nullptr;
    currentFont = nullptr;
    printing = true;
    use_show_text_glyphs = false;
    inUncoloredPattern = false;
    t3_render_state = Type3RenderNone;
    t3_glyph_has_bbox = false;
    t3_glyph_has_color = false;
    text_matrix_valid = true;

    groupColorSpaceStack = nullptr;
    group = nullptr;
    mask = nullptr;
    shape = nullptr;
    cairo_shape = nullptr;
    knockoutCount = 0;

    textPage = nullptr;
    actualText = nullptr;
    logicalStruct = false;
    pdfPageNum = 0;
    cairoPageNum = 0;

    // the SA parameter supposedly defaults to false, but Acrobat
    // apparently hardwires it to true
    stroke_adjust = true;
    align_stroke_coords = false;
    adjusted_stroke_width = false;
    xref = nullptr;
    currentStructParents = -1;
}

CairoOutputDev::~CairoOutputDev()
{
    if (fontEngine_owner && fontEngine) {
        delete fontEngine;
    }
    if (textClipPath) {
        cairo_path_destroy(textClipPath);
        textClipPath = nullptr;
    }

    if (cairo) {
        cairo_destroy(cairo);
    }
    cairo_pattern_destroy(stroke_pattern);
    cairo_pattern_destroy(fill_pattern);
    if (group) {
        cairo_pattern_destroy(group);
    }
    if (mask) {
        cairo_pattern_destroy(mask);
    }
    if (shape) {
        cairo_pattern_destroy(shape);
    }
    if (textPage) {
        textPage->decRefCnt();
    }
    delete actualText;
}

void CairoOutputDev::setCairo(cairo_t *c)
{
    if (cairo != nullptr) {
        cairo_status_t status = cairo_status(cairo);
        if (status) {
            error(errInternal, -1, "cairo context error: {0:s}", cairo_status_to_string(status));
        }
        cairo_destroy(cairo);
        assert(!cairo_shape);
    }
    if (c != nullptr) {
        cairo = cairo_reference(c);
        /* save the initial matrix so that we can use it for type3 fonts. */
        // XXX: is this sufficient? could we miss changes to the matrix somehow?
        cairo_get_matrix(cairo, &orig_matrix);
    } else {
        cairo = nullptr;
        cairo_shape = nullptr;
    }
}

bool CairoOutputDev::isPDF()
{
    if (cairo) {
        return cairo_surface_get_type(cairo_get_target(cairo)) == CAIRO_SURFACE_TYPE_PDF;
    }
    return false;
}

void CairoOutputDev::setTextPage(TextPage *text)
{
    if (textPage) {
        textPage->decRefCnt();
    }
    delete actualText;
    if (text) {
        textPage = text;
        textPage->incRefCnt();
        actualText = new ActualText(text);
    } else {
        textPage = nullptr;
        actualText = nullptr;
    }
}

void CairoOutputDev::copyAntialias(cairo_t *cr, cairo_t *source_cr)
{
    cairo_set_antialias(cr, cairo_get_antialias(source_cr));

    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_get_font_options(source_cr, font_options);
    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);
}

void CairoOutputDev::startDoc(PDFDoc *docA, CairoFontEngine *parentFontEngine)
{
    doc = docA;
    if (parentFontEngine) {
        fontEngine = parentFontEngine;
    } else {
        delete fontEngine;
        fontEngine = new CairoFontEngine(ft_lib);
        fontEngine_owner = true;
    }
    xref = doc->getXRef();

    mcidEmitted.clear();
    destsMap.clear();
    emittedDestinations.clear();
    pdfPageToCairoPageMap.clear();
    pdfPageRefToCairoPageNumMap.clear();
    cairoPageNum = 0;
    firstPage = true;
}

void CairoOutputDev::textStringToQuotedUtf8(const GooString *text, GooString *s)
{
    std::string utf8 = TextStringToUtf8(text->toStr());
    s->Set("'");
    for (char c : utf8) {
        if (c == '\\' || c == '\'') {
            s->append("\\");
        }
        s->append(c);
    }
    s->append("'");
}

// Initialization that needs to be performed after setCairo() is called.
void CairoOutputDev::startFirstPage(int pageNum, GfxState *state, XRef *xrefA)
{
    if (xrefA) {
        xref = xrefA;
    }

    if (logicalStruct && isPDF()) {
        int numDests = doc->getCatalog()->numDestNameTree();
        for (int i = 0; i < numDests; i++) {
            const GooString *name = doc->getCatalog()->getDestNameTreeName(i);
            std::unique_ptr<LinkDest> dest = doc->getCatalog()->getDestNameTreeDest(i);
            if (dest->isPageRef()) {
                Ref ref = dest->getPageRef();
                destsMap[ref].insert({ std::string(name->toStr()), std::move(dest) });
            }
        }

        numDests = doc->getCatalog()->numDests();
        for (int i = 0; i < numDests; i++) {
            const char *name = doc->getCatalog()->getDestsName(i);
            std::unique_ptr<LinkDest> dest = doc->getCatalog()->getDestsDest(i);
            if (dest->isPageRef()) {
                Ref ref = dest->getPageRef();
                destsMap[ref].insert({ std::string(name), std::move(dest) });
            }
        }
    }
}

void CairoOutputDev::startPage(int pageNum, GfxState *state, XRef *xrefA)
{
    if (firstPage) {
        startFirstPage(pageNum, state, xrefA);
        firstPage = false;
    }

    /* set up some per page defaults */
    cairo_pattern_destroy(fill_pattern);
    cairo_pattern_destroy(stroke_pattern);

    fill_pattern = cairo_pattern_create_rgb(0., 0., 0.);
    fill_color = { 0, 0, 0 };
    stroke_pattern = cairo_pattern_reference(fill_pattern);
    stroke_color = { 0, 0, 0 };

    if (textPage) {
        textPage->startPage(state);
    }

    pdfPageNum = pageNum;
    cairoPageNum++;
    pdfPageToCairoPageMap[pdfPageNum] = cairoPageNum;

    if (logicalStruct && isPDF()) {
        Object obj = doc->getPage(pageNum)->getAnnotsObject(xref);
        Annots *annots = new Annots(doc, pageNum, &obj);

        for (Annot *annot : annots->getAnnots()) {
            if (annot->getType() == Annot::typeLink) {
                annot->incRefCnt();
                annotations.push_back(annot);
            }
        }

        delete annots;

        // emit dests
        Ref *ref = doc->getCatalog()->getPageRef(pageNum);
        pdfPageRefToCairoPageNumMap[*ref] = cairoPageNum;
        auto pageDests = destsMap.find(*ref);
        if (pageDests != destsMap.end()) {
            for (auto &it : pageDests->second) {
                GooString quoted_name;
                GooString name(it.first);
                textStringToQuotedUtf8(&name, &quoted_name);
                emittedDestinations.insert(quoted_name.toStr());

                GooString attrib;
                attrib.appendf("name={0:t} ", &quoted_name);
                if (it.second->getChangeLeft()) {
                    attrib.appendf("x={0:g} ", it.second->getLeft());
                }
                if (it.second->getChangeTop()) {
                    attrib.appendf("y={0:g} ", state->getPageHeight() - it.second->getTop());
                }

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
                cairo_tag_begin(cairo, CAIRO_TAG_DEST, attrib.c_str());
                cairo_tag_end(cairo, CAIRO_TAG_DEST);
#endif
            }
        }

        currentStructParents = doc->getPage(pageNum)->getStructParents();
    }
}

void CairoOutputDev::endPage()
{
    if (textPage) {
        textPage->endPage();
        textPage->coalesce(true, 0, false);
    }
}

void CairoOutputDev::beginForm(Object *obj, Ref id)
{
    if (logicalStruct && isPDF()) {
        structParentsStack.push_back(currentStructParents);

        const Object tmp = obj->streamGetDict()->lookup("StructParents");
        if (!(tmp.isInt() || tmp.isNull())) {
            error(errSyntaxError, -1, "XObject StructParents object is wrong type ({0:s})", tmp.getTypeName());
        } else if (tmp.isInt()) {
            currentStructParents = tmp.getInt();
        }
    }
}

void CairoOutputDev::endForm(Object *obj, Ref id)
{
    if (logicalStruct && isPDF()) {
        currentStructParents = structParentsStack.back();
        structParentsStack.pop_back();
    }
}

void CairoOutputDev::quadToCairoRect(AnnotQuadrilaterals *quads, int idx, double pageHeight, cairo_rectangle_t *rect)
{
    double x1, x2, y1, y2;
    x1 = x2 = quads->getX1(idx);
    y1 = y2 = quads->getX2(idx);

    x1 = std::min(x1, quads->getX2(idx));
    x1 = std::min(x1, quads->getX3(idx));
    x1 = std::min(x1, quads->getX4(idx));

    y1 = std::min(y1, quads->getY2(idx));
    y1 = std::min(y1, quads->getY3(idx));
    y1 = std::min(y1, quads->getY4(idx));

    x2 = std::max(x2, quads->getX2(idx));
    x2 = std::max(x2, quads->getX3(idx));
    x2 = std::max(x2, quads->getX4(idx));

    y2 = std::max(y2, quads->getY2(idx));
    y2 = std::max(y2, quads->getY3(idx));
    y2 = std::max(y2, quads->getY4(idx));

    rect->x = x1;
    rect->y = pageHeight - y2;
    rect->width = x2 - x1;
    rect->height = y2 - y1;
}

bool CairoOutputDev::appendLinkDestRef(GooString *s, const LinkDest *dest)
{
    Ref ref = dest->getPageRef();
    auto pageNum = pdfPageRefToCairoPageNumMap.find(ref);
    if (pageNum != pdfPageRefToCairoPageNumMap.end()) {
        auto cairoPage = pdfPageToCairoPageMap.find(pageNum->second);
        if (cairoPage != pdfPageToCairoPageMap.end()) {
            s->appendf("page={0:d} ", cairoPage->second);
            double destPageHeight = doc->getPageMediaHeight(dest->getPageNum());
            appendLinkDestXY(s, dest, destPageHeight);
            return true;
        }
    }
    return false;
}

void CairoOutputDev::appendLinkDestXY(GooString *s, const LinkDest *dest, double destPageHeight)
{
    double x = 0;
    double y = 0;

    if (dest->getChangeLeft()) {
        x = dest->getLeft();
    }

    if (dest->getChangeTop()) {
        y = dest->getTop();
    }

    // if pageHeight is 0, dest is remote document, cairo uses PDF coords in this
    // case. So don't flip coords when pageHeight is 0.
    s->appendf("pos=[{0:g} {1:g}] ", x, destPageHeight ? destPageHeight - y : y);
}

bool CairoOutputDev::beginLinkTag(AnnotLink *annotLink)
{
    int page_num = annotLink->getPageNum();
    double height = doc->getPageMediaHeight(page_num);

    GooString attrib;
    attrib.appendf("link_page={0:d} ", page_num);
    attrib.append("rect=[");
    AnnotQuadrilaterals *quads = annotLink->getQuadrilaterals();
    if (quads && quads->getQuadrilateralsLength() > 0) {
        for (int i = 0; i < quads->getQuadrilateralsLength(); i++) {
            cairo_rectangle_t rect;
            quadToCairoRect(quads, i, height, &rect);
            attrib.appendf("{0:g} {1:g} {2:g} {3:g} ", rect.x, rect.y, rect.width, rect.height);
        }
    } else {
        double x1, x2, y1, y2;
        annotLink->getRect(&x1, &y1, &x2, &y2);
        attrib.appendf("{0:g} {1:g} {2:g} {3:g} ", x1, height - y2, x2 - x1, y2 - y1);
    }
    attrib.append("] ");

    LinkAction *action = annotLink->getAction();
    if (action->getKind() == actionGoTo) {
        LinkGoTo *act = static_cast<LinkGoTo *>(action);
        if (act->isOk()) {
            const GooString *namedDest = act->getNamedDest();
            const LinkDest *linkDest = act->getDest();
            if (namedDest) {
                GooString name;
                textStringToQuotedUtf8(namedDest, &name);
                if (!emittedDestinations.contains(name.toStr())) {
                    return false;
                }
                attrib.appendf("dest={0:t} ", &name);
            } else if (linkDest && linkDest->isOk() && linkDest->isPageRef()) {
                bool ok = appendLinkDestRef(&attrib, linkDest);
                if (!ok) {
                    return false;
                }
            }
        }
    } else if (action->getKind() == actionGoToR) {
        LinkGoToR *act = static_cast<LinkGoToR *>(action);
        attrib.appendf("file='{0:t}' ", act->getFileName());
        const GooString *namedDest = act->getNamedDest();
        const LinkDest *linkDest = act->getDest();
        if (namedDest) {
            GooString name;
            textStringToQuotedUtf8(namedDest, &name);
            if (!emittedDestinations.contains(name.toStr())) {
                return false;
            }
            attrib.appendf("dest={0:t} ", &name);
        } else if (linkDest && linkDest->isOk() && !linkDest->isPageRef()) {
            auto cairoPage = pdfPageToCairoPageMap.find(linkDest->getPageNum());
            if (cairoPage != pdfPageToCairoPageMap.end()) {
                attrib.appendf("page={0:d} ", cairoPage->second);
                appendLinkDestXY(&attrib, linkDest, 0.0);
            } else {
                return false;
            }
        }
    } else if (action->getKind() == actionURI) {
        LinkURI *act = static_cast<LinkURI *>(action);
        if (act->isOk()) {
            attrib.appendf("uri='{0:s}'", act->getURI().c_str());
        }
    }
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
    cairo_tag_begin(cairo, CAIRO_TAG_LINK, attrib.c_str());
#endif
    return true;
}

AnnotLink *CairoOutputDev::findLinkObject(const StructElement *elem)
{
    if (elem->isObjectRef()) {
        Ref ref = elem->getObjectRef();
        for (Annot *annot : annotations) {
            if (annot->getType() == Annot::typeLink && annot->match(&ref)) {
                return static_cast<AnnotLink *>(annot);
            }
        }
    }

    for (unsigned i = 0; i < elem->getNumChildren(); i++) {
        AnnotLink *link = findLinkObject(elem->getChild(i));
        if (link) {
            return link;
        }
    }

    return nullptr;
}

bool CairoOutputDev::beginLink(const StructElement *linkElem)
{
    bool emitted = true;
    AnnotLink *linkAnnot = findLinkObject(linkElem);
    if (linkAnnot) {
        emitted = beginLinkTag(linkAnnot);
    } else {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
        cairo_tag_begin(cairo, linkElem->getTypeName(), nullptr);
#endif
    }
    return emitted;
}

void CairoOutputDev::getStructElemAttributeString(const StructElement *elem)
{
    int mcid = 0;
    GooString attribs;
    Ref ref = elem->getObjectRef();
    attribs.appendf("id='{0:d}_{1:d}_{2:d}'", ref.num, ref.gen, mcid);
    attribs.appendf(" parent='{0:d}_{1:d}'", ref.num, ref.gen);
}

int CairoOutputDev::getContentElementStructParents(const StructElement *element)
{
    int structParents = -1;
    Ref ref;

    if (element->hasStmRef()) {
        element->getStmRef(ref);
        Object xobjectObj = xref->fetch(ref);
        const Object &spObj = xobjectObj.streamGetDict()->lookup("StructParents");
        if (spObj.isInt()) {
            structParents = spObj.getInt();
        }
    } else if (element->hasPageRef()) {
        element->getPageRef(ref);
        Object pageObj = xref->fetch(ref);
        const Object &spObj = pageObj.dictLookup("StructParents");
        if (spObj.isInt()) {
            structParents = spObj.getInt();
        }
    }

    if (structParents == -1) {
        error(errSyntaxError, -1, "Unable to find StructParents object for StructElement");
    }
    return structParents;
}

bool CairoOutputDev::checkIfStructElementNeeded(const StructElement *element)
{
    if (element->isContent() && !element->isObjectRef()) {
        int structParents = getContentElementStructParents(element);
        int mcid = element->getMCID();
        if (mcidEmitted.contains(std::pair(structParents, mcid))) {
            structElementNeeded.insert(element);
            return true;
        }
    } else if (!element->isContent()) {
        bool needed = false;
        for (unsigned i = 0; i < element->getNumChildren(); i++) {
            if (checkIfStructElementNeeded(element->getChild(i))) {
                needed = true;
            }
        }
        if (needed) {
            structElementNeeded.insert(element);
        }
        return needed;
    }
    return false;
}

void CairoOutputDev::emitStructElement(const StructElement *element)
{
    if (!structElementNeeded.contains(element)) {
        return;
    }

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
    if (element->isContent() && !element->isObjectRef()) {
        int structParents = getContentElementStructParents(element);
        int mcid = element->getMCID();
        GooString attribs;
        attribs.appendf("ref='{0:d}_{1:d}'", structParents, mcid);
        cairo_tag_begin(cairo, CAIRO_TAG_CONTENT_REF, attribs.c_str());
        cairo_tag_end(cairo, CAIRO_TAG_CONTENT_REF);
    } else if (!element->isContent()) {
        if (element->getType() == StructElement::Link) {
            bool ok = beginLink(element);
            if (!ok) {
                return;
            }
        } else {
            cairo_tag_begin(cairo, element->getTypeName(), "");
        }
        for (unsigned i = 0; i < element->getNumChildren(); i++) {
            emitStructElement(element->getChild(i));
        }
        cairo_tag_end(cairo, element->getTypeName());
    }
#endif
}

void CairoOutputDev::emitStructTree()
{
    if (logicalStruct && isPDF()) {
        const StructTreeRoot *root = doc->getStructTreeRoot();
        if (!root) {
            return;
        }

        for (unsigned i = 0; i < root->getNumChildren(); i++) {
            checkIfStructElementNeeded(root->getChild(i));
        }

        for (unsigned i = 0; i < root->getNumChildren(); i++) {
            emitStructElement(root->getChild(i));
        }
    }
}

void CairoOutputDev::startType3Render(GfxState *state, XRef *xrefA)
{
    /* When cairo calls a user font render function, the default
     * source set on the provided cairo_t must be used, except in the
     * case of a color user font explicitly setting a color.
     *
     * As startPage() resets the source to solid black, this function
     * is used instead to initialise the CairoOutputDev when rendering
     * a user font glyph.
     *
     * As noted in the Cairo documentation, the default source of a
     * render callback contains an internal marker denoting the
     * foreground color is to be used when the glyph is rendered, even
     * though querying the default source will reveal solid black.
     * For this reason, fill_color and stroke_color are set to nullopt
     * to ensure updateFillColor()/updateStrokeColor() will update the
     * color even if the new color is black.
     *
     * The saveState()/restoreState() functions also ensure the
     * default source is saved and restored, and the fill_color and
     * stroke_color is reset to nullopt for the same reason.
     */

    /* Initialise fill and stroke pattern to the current source pattern */
    fill_pattern = cairo_pattern_reference(cairo_get_source(cairo));
    stroke_pattern = cairo_pattern_reference(cairo_get_source(cairo));
    fill_color = {};
    stroke_color = {};
    t3_glyph_has_bbox = false;
    t3_glyph_has_color = false;

    if (xrefA != nullptr) {
        xref = xrefA;
    }
}

void CairoOutputDev::saveState(GfxState *state)
{
    LOG(printf("save\n"));
    cairo_save(cairo);
    if (cairo_shape) {
        cairo_save(cairo_shape);
    }

    /* To ensure the current source, potentially containing the hidden
     * foreground color maker, is saved and restored as required by
     * _render_type3_glyph, we avoid using the update color and
     * opacity functions in restoreState() and instead be careful to
     * save all the color related variables that have been set by the
     * update functions on the stack. */
    SaveStateElement elem;
    elem.fill_pattern = cairo_pattern_reference(fill_pattern);
    elem.fill_opacity = fill_opacity;
    elem.stroke_pattern = cairo_pattern_reference(stroke_pattern);
    elem.stroke_opacity = stroke_opacity;
    elem.mask = mask ? cairo_pattern_reference(mask) : nullptr;
    elem.mask_matrix = mask_matrix;
    elem.fontRef = currentFont ? currentFont->getRef() : Ref::INVALID();
    saveStateStack.push_back(elem);

    if (strokePathClip) {
        strokePathClip->ref_count++;
    }
}

void CairoOutputDev::restoreState(GfxState *state)
{
    LOG(printf("restore\n"));
    cairo_restore(cairo);
    if (cairo_shape) {
        cairo_restore(cairo_shape);
    }

    text_matrix_valid = true;

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = saveStateStack.back().fill_pattern;
    fill_color = {};
    fill_opacity = saveStateStack.back().fill_opacity;

    cairo_pattern_destroy(stroke_pattern);
    stroke_pattern = saveStateStack.back().stroke_pattern;
    stroke_color = {};
    stroke_opacity = saveStateStack.back().stroke_opacity;

    if (saveStateStack.back().fontRef != (currentFont ? currentFont->getRef() : Ref::INVALID())) {
        needFontUpdate = true;
    }

    /* This isn't restored by cairo_restore() since we keep it in the
     * output device. */
    updateBlendMode(state);

    if (mask) {
        cairo_pattern_destroy(mask);
    }
    mask = saveStateStack.back().mask;
    mask_matrix = saveStateStack.back().mask_matrix;
    saveStateStack.pop_back();

    if (strokePathClip && --strokePathClip->ref_count == 0) {
        delete strokePathClip->path;
        if (strokePathClip->dashes) {
            gfree(strokePathClip->dashes);
        }
        gfree(strokePathClip);
        strokePathClip = nullptr;
    }
}

void CairoOutputDev::updateAll(GfxState *state)
{
    updateLineDash(state);
    updateLineJoin(state);
    updateLineCap(state);
    updateLineWidth(state);
    updateFlatness(state);
    updateMiterLimit(state);
    updateFillColor(state);
    updateStrokeColor(state);
    updateFillOpacity(state);
    updateStrokeOpacity(state);
    updateBlendMode(state);
    needFontUpdate = true;
    if (textPage) {
        textPage->updateFont(state);
    }
}

void CairoOutputDev::setDefaultCTM(const double *ctm)
{
    cairo_matrix_t matrix;
    matrix.xx = ctm[0];
    matrix.yx = ctm[1];
    matrix.xy = ctm[2];
    matrix.yy = ctm[3];
    matrix.x0 = ctm[4];
    matrix.y0 = ctm[5];

    cairo_transform(cairo, &matrix);
    if (cairo_shape) {
        cairo_transform(cairo_shape, &matrix);
    }

    OutputDev::setDefaultCTM(ctm);
}

void CairoOutputDev::updateCTM(GfxState *state, double m11, double m12, double m21, double m22, double m31, double m32)
{
    cairo_matrix_t matrix, invert_matrix;
    matrix.xx = m11;
    matrix.yx = m12;
    matrix.xy = m21;
    matrix.yy = m22;
    matrix.x0 = m31;
    matrix.y0 = m32;

    /* Make sure the matrix is invertible before setting it.
     * cairo will blow up if we give it a matrix that's not
     * invertible, so we need to check before passing it
     * to cairo_transform. Ignoring it is likely to give better
     * results than not rendering anything at all. See #14398
     *
     * Ideally, we could do the cairo_transform
     * and then check if anything went wrong and fix it then
     * instead of having to invert the matrix. */
    invert_matrix = matrix;
    if (cairo_matrix_invert(&invert_matrix)) {
        error(errSyntaxWarning, -1, "matrix not invertible");
        return;
    }

    cairo_transform(cairo, &matrix);
    if (cairo_shape) {
        cairo_transform(cairo_shape, &matrix);
    }
    updateLineDash(state);
    updateLineJoin(state);
    updateLineCap(state);
    updateLineWidth(state);
}

void CairoOutputDev::updateLineDash(GfxState *state)
{
    double dashStart;

    const std::vector<double> &dashPattern = state->getLineDash(&dashStart);
    cairo_set_dash(cairo, dashPattern.data(), dashPattern.size(), dashStart);
    if (cairo_shape) {
        cairo_set_dash(cairo_shape, dashPattern.data(), dashPattern.size(), dashStart);
    }
}

void CairoOutputDev::updateFlatness(GfxState *state)
{
    // cairo_set_tolerance (cairo, state->getFlatness());
}

void CairoOutputDev::updateLineJoin(GfxState *state)
{
    switch (state->getLineJoin()) {
    case GfxState::LineJoinMitre:
        cairo_set_line_join(cairo, CAIRO_LINE_JOIN_MITER);
        break;
    case GfxState::LineJoinRound:
        cairo_set_line_join(cairo, CAIRO_LINE_JOIN_ROUND);
        break;
    case GfxState::LineJoinBevel:
        cairo_set_line_join(cairo, CAIRO_LINE_JOIN_BEVEL);
        break;
    }
    if (cairo_shape) {
        cairo_set_line_join(cairo_shape, cairo_get_line_join(cairo));
    }
}

void CairoOutputDev::updateLineCap(GfxState *state)
{
    switch (state->getLineCap()) {
    case GfxState::LineCapButt:
        cairo_set_line_cap(cairo, CAIRO_LINE_CAP_BUTT);
        break;
    case GfxState::LineCapRound:
        cairo_set_line_cap(cairo, CAIRO_LINE_CAP_ROUND);
        break;
    case GfxState::LineCapProjecting:
        cairo_set_line_cap(cairo, CAIRO_LINE_CAP_SQUARE);
        break;
    }
    if (cairo_shape) {
        cairo_set_line_cap(cairo_shape, cairo_get_line_cap(cairo));
    }
}

void CairoOutputDev::updateMiterLimit(GfxState *state)
{
    cairo_set_miter_limit(cairo, state->getMiterLimit());
    if (cairo_shape) {
        cairo_set_miter_limit(cairo_shape, state->getMiterLimit());
    }
}

void CairoOutputDev::updateLineWidth(GfxState *state)
{
    LOG(printf("line width: %f\n", state->getLineWidth()));
    adjusted_stroke_width = false;
    double width = state->getLineWidth();
    if (stroke_adjust && !printing) {
        double x, y;
        x = y = width;

        /* find out line width in device units */
        cairo_user_to_device_distance(cairo, &x, &y);
        if (fabs(x) <= 1.0 && fabs(y) <= 1.0) {
            /* adjust width to at least one device pixel */
            x = y = 1.0;
            cairo_device_to_user_distance(cairo, &x, &y);
            width = MIN(fabs(x), fabs(y));
            adjusted_stroke_width = true;
        }
    } else if (width == 0.0) {
        /* Cairo does not support 0 line width == 1 device pixel. Find out
         * how big pixels (device unit) are in the x and y
         * directions. Choose the smaller of the two as our line width.
         */
        double x = 1.0, y = 1.0;
        if (printing) {
            // assume printer pixel size is 1/600 inch
            x = 72.0 / 600;
            y = 72.0 / 600;
        }
        cairo_device_to_user_distance(cairo, &x, &y);
        width = MIN(fabs(x), fabs(y));
    }
    cairo_set_line_width(cairo, width);
    if (cairo_shape) {
        cairo_set_line_width(cairo_shape, cairo_get_line_width(cairo));
    }
}

void CairoOutputDev::updateFillColor(GfxState *state)
{
    if (inUncoloredPattern) {
        return;
    }

    GfxRGB new_color;
    state->getFillRGB(&new_color);
    bool color_match = fill_color && *fill_color == new_color;
    if (cairo_pattern_get_type(fill_pattern) != CAIRO_PATTERN_TYPE_SOLID || !color_match) {
        cairo_pattern_destroy(fill_pattern);
        fill_pattern = cairo_pattern_create_rgba(colToDbl(new_color.r), colToDbl(new_color.g), colToDbl(new_color.b), fill_opacity);
        fill_color = new_color;
        LOG(printf("fill color: %d %d %d\n", fill_color->r, fill_color->g, fill_color->b));
    }
}

void CairoOutputDev::updateStrokeColor(GfxState *state)
{

    if (inUncoloredPattern) {
        return;
    }

    GfxRGB new_color;
    state->getStrokeRGB(&new_color);
    bool color_match = stroke_color && *stroke_color == new_color;
    if (cairo_pattern_get_type(fill_pattern) != CAIRO_PATTERN_TYPE_SOLID || !color_match) {
        cairo_pattern_destroy(stroke_pattern);
        stroke_pattern = cairo_pattern_create_rgba(colToDbl(new_color.r), colToDbl(new_color.g), colToDbl(new_color.b), stroke_opacity);
        stroke_color = new_color;
        LOG(printf("stroke color: %d %d %d\n", stroke_color->r, stroke_color->g, stroke_color->b));
    }
}

void CairoOutputDev::updateFillOpacity(GfxState *state)
{
    double opacity = fill_opacity;

    if (inUncoloredPattern) {
        return;
    }

    fill_opacity = state->getFillOpacity();
    if (opacity != fill_opacity) {
        if (!fill_color) {
            GfxRGB color;
            state->getFillRGB(&color);
            fill_color = color;
        }
        cairo_pattern_destroy(fill_pattern);
        fill_pattern = cairo_pattern_create_rgba(colToDbl(fill_color->r), colToDbl(fill_color->g), colToDbl(fill_color->b), fill_opacity);

        LOG(printf("fill opacity: %f\n", fill_opacity));
    }
}

void CairoOutputDev::updateStrokeOpacity(GfxState *state)
{
    double opacity = stroke_opacity;

    if (inUncoloredPattern) {
        return;
    }

    stroke_opacity = state->getStrokeOpacity();
    if (opacity != stroke_opacity) {
        if (!stroke_color) {
            GfxRGB color;
            state->getStrokeRGB(&color);
            stroke_color = color;
        }
        cairo_pattern_destroy(stroke_pattern);
        stroke_pattern = cairo_pattern_create_rgba(colToDbl(stroke_color->r), colToDbl(stroke_color->g), colToDbl(stroke_color->b), stroke_opacity);

        LOG(printf("stroke opacity: %f\n", stroke_opacity));
    }
}

void CairoOutputDev::updateFillColorStop(GfxState *state, double offset)
{
    if (inUncoloredPattern) {
        return;
    }

    GfxRGB color;
    state->getFillRGB(&color);

    // If stroke pattern is set then the current fill is clipped
    // to a stroke path.  In that case, the stroke opacity has to be used
    // rather than the fill opacity.
    // See https://gitlab.freedesktop.org/poppler/poppler/issues/178
    auto opacity = (state->getStrokePattern()) ? state->getStrokeOpacity() : state->getFillOpacity();

    cairo_pattern_add_color_stop_rgba(fill_pattern, offset, colToDbl(color.r), colToDbl(color.g), colToDbl(color.b), opacity);
    LOG(printf("fill color stop: %f (%d, %d, %d, %d)\n", offset, color.r, color.g, color.b, dblToCol(opacity)));
}

void CairoOutputDev::updateBlendMode(GfxState *state)
{
    switch (state->getBlendMode()) {
    default:
    case gfxBlendNormal:
        cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
        break;
    case gfxBlendMultiply:
        cairo_set_operator(cairo, CAIRO_OPERATOR_MULTIPLY);
        break;
    case gfxBlendScreen:
        cairo_set_operator(cairo, CAIRO_OPERATOR_SCREEN);
        break;
    case gfxBlendOverlay:
        cairo_set_operator(cairo, CAIRO_OPERATOR_OVERLAY);
        break;
    case gfxBlendDarken:
        cairo_set_operator(cairo, CAIRO_OPERATOR_DARKEN);
        break;
    case gfxBlendLighten:
        cairo_set_operator(cairo, CAIRO_OPERATOR_LIGHTEN);
        break;
    case gfxBlendColorDodge:
        cairo_set_operator(cairo, CAIRO_OPERATOR_COLOR_DODGE);
        break;
    case gfxBlendColorBurn:
        cairo_set_operator(cairo, CAIRO_OPERATOR_COLOR_BURN);
        break;
    case gfxBlendHardLight:
        cairo_set_operator(cairo, CAIRO_OPERATOR_HARD_LIGHT);
        break;
    case gfxBlendSoftLight:
        cairo_set_operator(cairo, CAIRO_OPERATOR_SOFT_LIGHT);
        break;
    case gfxBlendDifference:
        cairo_set_operator(cairo, CAIRO_OPERATOR_DIFFERENCE);
        break;
    case gfxBlendExclusion:
        cairo_set_operator(cairo, CAIRO_OPERATOR_EXCLUSION);
        break;
    case gfxBlendHue:
        cairo_set_operator(cairo, CAIRO_OPERATOR_HSL_HUE);
        break;
    case gfxBlendSaturation:
        cairo_set_operator(cairo, CAIRO_OPERATOR_HSL_SATURATION);
        break;
    case gfxBlendColor:
        cairo_set_operator(cairo, CAIRO_OPERATOR_HSL_COLOR);
        break;
    case gfxBlendLuminosity:
        cairo_set_operator(cairo, CAIRO_OPERATOR_HSL_LUMINOSITY);
        break;
    }
    LOG(printf("blend mode: %d\n", (int)state->getBlendMode()));
}

void CairoOutputDev::updateFont(GfxState *state)
{
    cairo_font_face_t *font_face;
    cairo_matrix_t matrix, invert_matrix;

    LOG(printf("updateFont() font=%s\n", state->getFont()->getName()->c_str()));

    needFontUpdate = false;

    // FIXME: use cairo font engine?
    if (textPage) {
        textPage->updateFont(state);
    }

    currentFont = fontEngine->getFont(state->getFont(), doc, printing, xref);

    if (!currentFont) {
        return;
    }

    font_face = currentFont->getFontFace();
    cairo_set_font_face(cairo, font_face);

    use_show_text_glyphs = state->getFont()->hasToUnicodeCMap() && cairo_surface_has_show_text_glyphs(cairo_get_target(cairo));

    double fontSize = state->getFontSize();
    const double *m = state->getTextMat();
    /* NOTE: adjusting by a constant is hack. The correct solution
     * is probably to use user-fonts and compute the scale on a per
     * glyph basis instead of for the entire font */
    double w = currentFont->getSubstitutionCorrection(state->getFont());
    matrix.xx = m[0] * fontSize * state->getHorizScaling() * w;
    matrix.yx = m[1] * fontSize * state->getHorizScaling() * w;
    matrix.xy = -m[2] * fontSize;
    matrix.yy = -m[3] * fontSize;
    matrix.x0 = 0;
    matrix.y0 = 0;

    LOG(printf("font matrix: %f %f %f %f\n", matrix.xx, matrix.yx, matrix.xy, matrix.yy));

    /* Make sure the font matrix is invertible before setting it.  cairo
     * will blow up if we give it a matrix that's not invertible, so we
     * need to check before passing it to cairo_set_font_matrix. Ignoring it
     * is likely to give better results than not rendering anything at
     * all. See #18254.
     */
    invert_matrix = matrix;
    if (cairo_matrix_invert(&invert_matrix)) {
        error(errSyntaxWarning, -1, "font matrix not invertible");
        text_matrix_valid = false;
        return;
    }

    cairo_set_font_matrix(cairo, &matrix);
    text_matrix_valid = true;
}

/* Tolerance in pixels for checking if strokes are horizontal or vertical
 * lines in device space */
#define STROKE_COORD_TOLERANCE 0.5

/* Align stroke coordinate i if the point is the start or end of a
 * horizontal or vertical line */
void CairoOutputDev::alignStrokeCoords(const GfxSubpath *subpath, int i, double *x, double *y)
{
    double x1, y1, x2, y2;
    bool align = false;

    x1 = subpath->getX(i);
    y1 = subpath->getY(i);
    cairo_user_to_device(cairo, &x1, &y1);

    // Does the current coord and prev coord form a horiz or vert line?
    if (i > 0 && !subpath->getCurve(i - 1)) {
        x2 = subpath->getX(i - 1);
        y2 = subpath->getY(i - 1);
        cairo_user_to_device(cairo, &x2, &y2);
        if (fabs(x2 - x1) < STROKE_COORD_TOLERANCE || fabs(y2 - y1) < STROKE_COORD_TOLERANCE) {
            align = true;
        }
    }

    // Does the current coord and next coord form a horiz or vert line?
    if (i < subpath->getNumPoints() - 1 && !subpath->getCurve(i + 1)) {
        x2 = subpath->getX(i + 1);
        y2 = subpath->getY(i + 1);
        cairo_user_to_device(cairo, &x2, &y2);
        if (fabs(x2 - x1) < STROKE_COORD_TOLERANCE || fabs(y2 - y1) < STROKE_COORD_TOLERANCE) {
            align = true;
        }
    }

    *x = subpath->getX(i);
    *y = subpath->getY(i);
    if (align) {
        /* see http://www.cairographics.org/FAQ/#sharp_lines */
        cairo_user_to_device(cairo, x, y);
        *x = floor(*x) + 0.5;
        *y = floor(*y) + 0.5;
        cairo_device_to_user(cairo, x, y);
    }
}

#undef STROKE_COORD_TOLERANCE

void CairoOutputDev::doPath(cairo_t *c, GfxState *state, const GfxPath *path)
{
    int i, j;
    double x, y;
    cairo_new_path(c);
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        const GfxSubpath *subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > 0) {
            if (align_stroke_coords) {
                alignStrokeCoords(subpath, 0, &x, &y);
            } else {
                x = subpath->getX(0);
                y = subpath->getY(0);
            }
            cairo_move_to(c, x, y);
            j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    if (align_stroke_coords) {
                        alignStrokeCoords(subpath, j + 2, &x, &y);
                    } else {
                        x = subpath->getX(j + 2);
                        y = subpath->getY(j + 2);
                    }
                    cairo_curve_to(c, subpath->getX(j), subpath->getY(j), subpath->getX(j + 1), subpath->getY(j + 1), x, y);

                    j += 3;
                } else {
                    if (align_stroke_coords) {
                        alignStrokeCoords(subpath, j, &x, &y);
                    } else {
                        x = subpath->getX(j);
                        y = subpath->getY(j);
                    }
                    cairo_line_to(c, x, y);
                    ++j;
                }
            }
            if (subpath->isClosed()) {
                LOG(printf("close\n"));
                cairo_close_path(c);
            }
        }
    }
}

void CairoOutputDev::stroke(GfxState *state)
{
    if (t3_render_state == Type3RenderMask) {
        GfxGray gray;
        state->getFillGray(&gray);
        if (colToDbl(gray) > 0.5) {
            return;
        }
    }

    if (adjusted_stroke_width) {
        align_stroke_coords = true;
    }
    doPath(cairo, state, state->getPath());
    align_stroke_coords = false;
    cairo_set_source(cairo, stroke_pattern);
    LOG(printf("stroke\n"));
    if (strokePathClip) {
        cairo_push_group(cairo);
        cairo_stroke(cairo);
        cairo_pop_group_to_source(cairo);
        fillToStrokePathClip(state);
    } else {
        cairo_stroke(cairo);
    }
    if (cairo_shape) {
        doPath(cairo_shape, state, state->getPath());
        cairo_stroke(cairo_shape);
    }
}

void CairoOutputDev::fill(GfxState *state)
{
    if (t3_render_state == Type3RenderMask) {
        GfxGray gray;
        state->getFillGray(&gray);
        if (colToDbl(gray) > 0.5) {
            return;
        }
    }

    doPath(cairo, state, state->getPath());
    cairo_set_fill_rule(cairo, CAIRO_FILL_RULE_WINDING);
    cairo_set_source(cairo, fill_pattern);
    LOG(printf("fill\n"));
    // XXX: how do we get the path
    if (mask) {
        cairo_save(cairo);
        cairo_clip(cairo);
        if (strokePathClip) {
            cairo_push_group(cairo);
            fillToStrokePathClip(state);
            cairo_pop_group_to_source(cairo);
        }
        cairo_set_matrix(cairo, &mask_matrix);
        cairo_mask(cairo, mask);
        cairo_restore(cairo);
    } else if (strokePathClip) {
        fillToStrokePathClip(state);
    } else {
        cairo_fill(cairo);
    }
    if (cairo_shape) {
        cairo_set_fill_rule(cairo_shape, CAIRO_FILL_RULE_WINDING);
        doPath(cairo_shape, state, state->getPath());
        cairo_fill(cairo_shape);
    }
}

void CairoOutputDev::eoFill(GfxState *state)
{
    doPath(cairo, state, state->getPath());
    cairo_set_fill_rule(cairo, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_set_source(cairo, fill_pattern);
    LOG(printf("fill-eo\n"));

    if (mask) {
        cairo_save(cairo);
        cairo_clip(cairo);
        cairo_set_matrix(cairo, &mask_matrix);
        cairo_mask(cairo, mask);
        cairo_restore(cairo);
    } else {
        cairo_fill(cairo);
    }
    if (cairo_shape) {
        cairo_set_fill_rule(cairo_shape, CAIRO_FILL_RULE_EVEN_ODD);
        doPath(cairo_shape, state, state->getPath());
        cairo_fill(cairo_shape);
    }
}

bool CairoOutputDev::tilingPatternFill(GfxState *state, Gfx *gfxA, Catalog *cat, GfxTilingPattern *tPat, const double *mat, int x0, int y0, int x1, int y1, double xStep, double yStep)
{
    PDFRectangle box;
    Gfx *gfx;
    cairo_pattern_t *pattern;
    cairo_surface_t *surface;
    cairo_matrix_t matrix;
    cairo_matrix_t pattern_matrix;
    cairo_t *old_cairo;
    double xMin, yMin, xMax, yMax;
    double width, height;
    double scaleX, scaleY;
    int surface_width, surface_height;
    StrokePathClip *strokePathTmp;
    bool adjusted_stroke_width_tmp;
    cairo_pattern_t *maskTmp;
    const double *bbox = tPat->getBBox();
    const double *pmat = tPat->getMatrix();
    const int paintType = tPat->getPaintType();
    Dict *resDict = tPat->getResDict();
    Object *str = tPat->getContentStream();

    width = bbox[2] - bbox[0];
    height = bbox[3] - bbox[1];

    if (xStep != width || yStep != height) {
        return false;
    }
    /* TODO: implement the other cases here too */

    // Find the width and height of the transformed pattern
    cairo_get_matrix(cairo, &matrix);
    cairo_matrix_init(&pattern_matrix, mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
    cairo_matrix_multiply(&matrix, &matrix, &pattern_matrix);

    double widthX = width, widthY = 0;
    cairo_matrix_transform_distance(&matrix, &widthX, &widthY);
    surface_width = ceil(sqrt(widthX * widthX + widthY * widthY));

    double heightX = 0, heightY = height;
    cairo_matrix_transform_distance(&matrix, &heightX, &heightY);
    surface_height = ceil(sqrt(heightX * heightX + heightY * heightY));
    scaleX = surface_width / width;
    scaleY = surface_height / height;

    surface = cairo_surface_create_similar(cairo_get_target(cairo), CAIRO_CONTENT_COLOR_ALPHA, surface_width, surface_height);
    if (cairo_surface_status(surface)) {
        return false;
    }

    old_cairo = cairo;
    cairo = cairo_create(surface);
    cairo_surface_destroy(surface);
    copyAntialias(cairo, old_cairo);

    box.x1 = bbox[0];
    box.y1 = bbox[1];
    box.x2 = bbox[2];
    box.y2 = bbox[3];
    cairo_scale(cairo, scaleX, scaleY);
    cairo_translate(cairo, -box.x1, -box.y1);

    strokePathTmp = strokePathClip;
    strokePathClip = nullptr;
    adjusted_stroke_width_tmp = adjusted_stroke_width;
    maskTmp = mask;
    mask = nullptr;
    gfx = new Gfx(doc, this, resDict, &box, nullptr, nullptr, nullptr, gfxA);
    if (paintType == 2) {
        inUncoloredPattern = true;
    }
    gfx->display(str);
    if (paintType == 2) {
        inUncoloredPattern = false;
    }
    delete gfx;
    strokePathClip = strokePathTmp;
    adjusted_stroke_width = adjusted_stroke_width_tmp;
    mask = maskTmp;

    pattern = cairo_pattern_create_for_surface(cairo_get_target(cairo));
    cairo_destroy(cairo);
    cairo = old_cairo;
    if (cairo_pattern_status(pattern)) {
        return false;
    }

    // Cairo can fail if the pattern translation is too large. Fix by making the
    // translation smaller.
    const double det = pmat[0] * pmat[3] - pmat[1] * pmat[2];

    // Find the number of repetitions of pattern we need to shift by. Transform
    // the translation component of pmat (pmat[4] and pmat[5]) into the pattern's
    // coordinate system by multiplying by inverse of pmat, then divide by
    // pattern size (xStep and yStep).
    const double xoffset = round((pmat[3] * pmat[4] - pmat[2] * pmat[5]) / (xStep * det));
    const double yoffset = -round((pmat[1] * pmat[4] - pmat[0] * pmat[5]) / (yStep * det));

    if (!std::isfinite(xoffset) || !std::isfinite(yoffset)) {
        error(errSyntaxWarning, -1, "CairoOutputDev: Singular matrix in tilingPatternFill");
        return false;
    }

    // Shift pattern_matrix by multiples of the pattern size.
    pattern_matrix.x0 -= xoffset * pattern_matrix.xx * xStep + yoffset * pattern_matrix.xy * yStep;
    pattern_matrix.y0 -= xoffset * pattern_matrix.yx * xStep + yoffset * pattern_matrix.yy * yStep;

    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
    cairo_rectangle(cairo, xMin, yMin, xMax - xMin, yMax - yMin);

    cairo_matrix_init_scale(&matrix, scaleX, scaleY);
    cairo_matrix_translate(&matrix, -box.x1, -box.y1);
    cairo_pattern_set_matrix(pattern, &matrix);

    cairo_transform(cairo, &pattern_matrix);
    cairo_set_source(cairo, pattern);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    if (strokePathClip) {
        fillToStrokePathClip(state);
    } else {
        cairo_fill(cairo);
    }

    cairo_pattern_destroy(pattern);

    return true;
}

bool CairoOutputDev::functionShadedFill(GfxState *state, GfxFunctionShading *shading)
{
    // Function shaded fills are subdivided to rectangles that are the
    // following size in device space.  Note when printing this size is
    // in points.
    const int subdivide_pixels = 10;

    // Set a minimum step to force upon {x|y}_step, to avoid approximate or reach
    // infinite loop when {x|y}_step approximates to or equals zero - Issue #1520
    const double minimum_step = 0.01;

    double x_begin, x_end, x1, x2;
    double y_begin, y_end, y1, y2;
    double x_step;
    double y_step;
    GfxColor color;
    GfxRGB rgb;
    cairo_matrix_t mat;

    const double *matrix = shading->getMatrix();
    mat.xx = matrix[0];
    mat.yx = matrix[1];
    mat.xy = matrix[2];
    mat.yy = matrix[3];
    mat.x0 = matrix[4];
    mat.y0 = matrix[5];
    if (cairo_matrix_invert(&mat)) {
        error(errSyntaxWarning, -1, "matrix not invertible");
        return false;
    }

    // get cell size in pattern space
    x_step = y_step = subdivide_pixels;
    cairo_matrix_transform_distance(&mat, &x_step, &y_step);

    if (y_step < minimum_step) {
        y_step = minimum_step;
    }
    if (x_step < minimum_step) {
        x_step = minimum_step;
    }

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = cairo_pattern_create_mesh();
    cairo_pattern_set_matrix(fill_pattern, &mat);
    shading->getDomain(&x_begin, &y_begin, &x_end, &y_end);

    for (x1 = x_begin; x1 < x_end; x1 += x_step) {
        x2 = x1 + x_step;
        if (x2 > x_end) {
            x2 = x_end;
        }

        for (y1 = y_begin; y1 < y_end; y1 += y_step) {
            y2 = y1 + y_step;
            if (y2 > y_end) {
                y2 = y_end;
            }

            cairo_mesh_pattern_begin_patch(fill_pattern);
            cairo_mesh_pattern_move_to(fill_pattern, x1, y1);
            cairo_mesh_pattern_line_to(fill_pattern, x2, y1);
            cairo_mesh_pattern_line_to(fill_pattern, x2, y2);
            cairo_mesh_pattern_line_to(fill_pattern, x1, y2);

            shading->getColor(x1, y1, &color);
            shading->getColorSpace()->getRGB(&color, &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, 0, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));

            shading->getColor(x2, y1, &color);
            shading->getColorSpace()->getRGB(&color, &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, 1, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));

            shading->getColor(x2, y2, &color);
            shading->getColorSpace()->getRGB(&color, &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, 2, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));

            shading->getColor(x1, y2, &color);
            shading->getColorSpace()->getRGB(&color, &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, 3, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));

            cairo_mesh_pattern_end_patch(fill_pattern);
        }
    }

    double xMin, yMin, xMax, yMax;
    // get the clip region bbox
    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
    state->moveTo(xMin, yMin);
    state->lineTo(xMin, yMax);
    state->lineTo(xMax, yMax);
    state->lineTo(xMax, yMin);
    state->closePath();
    fill(state);
    state->clearPath();

    return true;
}

bool CairoOutputDev::axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax)
{
    double x0, y0, x1, y1;
    double dx, dy;

    shading->getCoords(&x0, &y0, &x1, &y1);
    dx = x1 - x0;
    dy = y1 - y0;

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = cairo_pattern_create_linear(x0 + tMin * dx, y0 + tMin * dy, x0 + tMax * dx, y0 + tMax * dy);
    if (!shading->getExtend0() && !shading->getExtend1()) {
        cairo_pattern_set_extend(fill_pattern, CAIRO_EXTEND_NONE);
    } else {
        cairo_pattern_set_extend(fill_pattern, CAIRO_EXTEND_PAD);
    }

    LOG(printf("axial-sh\n"));

    // TODO: use the actual stops in the shading in the case
    // of linear interpolation (Type 2 Exponential functions with N=1)
    return false;
}

bool CairoOutputDev::axialShadedSupportExtend(GfxState *state, GfxAxialShading *shading)
{
    return (shading->getExtend0() == shading->getExtend1());
}

bool CairoOutputDev::radialShadedFill(GfxState *state, GfxRadialShading *shading, double sMin, double sMax)
{
    double x0, y0, r0, x1, y1, r1;
    double dx, dy, dr;
    cairo_matrix_t matrix;
    double scale;

    shading->getCoords(&x0, &y0, &r0, &x1, &y1, &r1);
    dx = x1 - x0;
    dy = y1 - y0;
    dr = r1 - r0;

    // Cairo/pixman do not work well with a very large or small scaled
    // matrix.  See cairo bug #81657.
    //
    // As a workaround, scale the pattern by the average of the vertical
    // and horizontal scaling of the current transformation matrix.
    cairo_get_matrix(cairo, &matrix);
    scale = (sqrt(matrix.xx * matrix.xx + matrix.yx * matrix.yx) + sqrt(matrix.xy * matrix.xy + matrix.yy * matrix.yy)) / 2;
    cairo_matrix_init_scale(&matrix, scale, scale);

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = cairo_pattern_create_radial((x0 + sMin * dx) * scale, (y0 + sMin * dy) * scale, (r0 + sMin * dr) * scale, (x0 + sMax * dx) * scale, (y0 + sMax * dy) * scale, (r0 + sMax * dr) * scale);
    cairo_pattern_set_matrix(fill_pattern, &matrix);
    if (shading->getExtend0() && shading->getExtend1()) {
        cairo_pattern_set_extend(fill_pattern, CAIRO_EXTEND_PAD);
    } else {
        cairo_pattern_set_extend(fill_pattern, CAIRO_EXTEND_NONE);
    }

    LOG(printf("radial-sh\n"));

    return false;
}

bool CairoOutputDev::radialShadedSupportExtend(GfxState *state, GfxRadialShading *shading)
{
    return (shading->getExtend0() == shading->getExtend1());
}

bool CairoOutputDev::gouraudTriangleShadedFill(GfxState *state, GfxGouraudTriangleShading *shading)
{
    double x0, y0, x1, y1, x2, y2;
    GfxColor color[3];
    int i, j;
    GfxRGB rgb;

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = cairo_pattern_create_mesh();

    for (i = 0; i < shading->getNTriangles(); i++) {
        if (shading->isParameterized()) {
            double color0, color1, color2;
            shading->getTriangle(i, &x0, &y0, &color0, &x1, &y1, &color1, &x2, &y2, &color2);
            shading->getParameterizedColor(color0, &color[0]);
            shading->getParameterizedColor(color1, &color[1]);
            shading->getParameterizedColor(color2, &color[2]);
        } else {
            shading->getTriangle(i, &x0, &y0, &color[0], &x1, &y1, &color[1], &x2, &y2, &color[2]);
        }

        cairo_mesh_pattern_begin_patch(fill_pattern);

        cairo_mesh_pattern_move_to(fill_pattern, x0, y0);
        cairo_mesh_pattern_line_to(fill_pattern, x1, y1);
        cairo_mesh_pattern_line_to(fill_pattern, x2, y2);

        for (j = 0; j < 3; j++) {
            shading->getColorSpace()->getRGB(&color[j], &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, j, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));
        }

        cairo_mesh_pattern_end_patch(fill_pattern);
    }

    double xMin, yMin, xMax, yMax;
    // get the clip region bbox
    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
    state->moveTo(xMin, yMin);
    state->lineTo(xMin, yMax);
    state->lineTo(xMax, yMax);
    state->lineTo(xMax, yMin);
    state->closePath();
    fill(state);
    state->clearPath();

    return true;
}

bool CairoOutputDev::patchMeshShadedFill(GfxState *state, GfxPatchMeshShading *shading)
{
    int i, j, k;

    cairo_pattern_destroy(fill_pattern);
    fill_pattern = cairo_pattern_create_mesh();

    for (i = 0; i < shading->getNPatches(); i++) {
        const GfxPatch *patch = shading->getPatch(i);
        GfxColor color;
        GfxRGB rgb;

        cairo_mesh_pattern_begin_patch(fill_pattern);

        cairo_mesh_pattern_move_to(fill_pattern, patch->x[0][0], patch->y[0][0]);
        cairo_mesh_pattern_curve_to(fill_pattern, patch->x[0][1], patch->y[0][1], patch->x[0][2], patch->y[0][2], patch->x[0][3], patch->y[0][3]);

        cairo_mesh_pattern_curve_to(fill_pattern, patch->x[1][3], patch->y[1][3], patch->x[2][3], patch->y[2][3], patch->x[3][3], patch->y[3][3]);

        cairo_mesh_pattern_curve_to(fill_pattern, patch->x[3][2], patch->y[3][2], patch->x[3][1], patch->y[3][1], patch->x[3][0], patch->y[3][0]);

        cairo_mesh_pattern_curve_to(fill_pattern, patch->x[2][0], patch->y[2][0], patch->x[1][0], patch->y[1][0], patch->x[0][0], patch->y[0][0]);

        cairo_mesh_pattern_set_control_point(fill_pattern, 0, patch->x[1][1], patch->y[1][1]);
        cairo_mesh_pattern_set_control_point(fill_pattern, 1, patch->x[1][2], patch->y[1][2]);
        cairo_mesh_pattern_set_control_point(fill_pattern, 2, patch->x[2][2], patch->y[2][2]);
        cairo_mesh_pattern_set_control_point(fill_pattern, 3, patch->x[2][1], patch->y[2][1]);

        for (j = 0; j < 4; j++) {
            int u, v;

            switch (j) {
            case 0:
                u = 0;
                v = 0;
                break;
            case 1:
                u = 0;
                v = 1;
                break;
            case 2:
                u = 1;
                v = 1;
                break;
            case 3:
                u = 1;
                v = 0;
                break;
            }

            if (shading->isParameterized()) {
                shading->getParameterizedColor(patch->color[u][v].c[0], &color);
            } else {
                for (k = 0; k < shading->getColorSpace()->getNComps(); k++) {
                    // simply cast to the desired type; that's all what is needed.
                    color.c[k] = GfxColorComp(patch->color[u][v].c[k]);
                }
            }

            shading->getColorSpace()->getRGB(&color, &rgb);
            cairo_mesh_pattern_set_corner_color_rgb(fill_pattern, j, colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b));
        }
        cairo_mesh_pattern_end_patch(fill_pattern);
    }

    double xMin, yMin, xMax, yMax;
    // get the clip region bbox
    state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
    state->moveTo(xMin, yMin);
    state->lineTo(xMin, yMax);
    state->lineTo(xMax, yMax);
    state->lineTo(xMax, yMin);
    state->closePath();
    fill(state);
    state->clearPath();

    return true;
}

void CairoOutputDev::clip(GfxState *state)
{
    doPath(cairo, state, state->getPath());
    cairo_set_fill_rule(cairo, CAIRO_FILL_RULE_WINDING);
    cairo_clip(cairo);
    LOG(printf("clip\n"));
    if (cairo_shape) {
        doPath(cairo_shape, state, state->getPath());
        cairo_set_fill_rule(cairo_shape, CAIRO_FILL_RULE_WINDING);
        cairo_clip(cairo_shape);
    }
}

void CairoOutputDev::eoClip(GfxState *state)
{
    doPath(cairo, state, state->getPath());
    cairo_set_fill_rule(cairo, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(cairo);
    LOG(printf("clip-eo\n"));
    if (cairo_shape) {
        doPath(cairo_shape, state, state->getPath());
        cairo_set_fill_rule(cairo_shape, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_clip(cairo_shape);
    }
}

void CairoOutputDev::clipToStrokePath(GfxState *state)
{
    LOG(printf("clip-to-stroke-path\n"));
    strokePathClip = (StrokePathClip *)gmalloc(sizeof(*strokePathClip));
    strokePathClip->path = state->getPath()->copy();
    cairo_get_matrix(cairo, &strokePathClip->ctm);
    strokePathClip->line_width = cairo_get_line_width(cairo);
    strokePathClip->dash_count = cairo_get_dash_count(cairo);
    if (strokePathClip->dash_count) {
        strokePathClip->dashes = (double *)gmallocn(sizeof(double), strokePathClip->dash_count);
        cairo_get_dash(cairo, strokePathClip->dashes, &strokePathClip->dash_offset);
    } else {
        strokePathClip->dashes = nullptr;
    }
    strokePathClip->cap = cairo_get_line_cap(cairo);
    strokePathClip->join = cairo_get_line_join(cairo);
    strokePathClip->miter = cairo_get_miter_limit(cairo);
    strokePathClip->ref_count = 1;
}

void CairoOutputDev::fillToStrokePathClip(GfxState *state)
{
    cairo_save(cairo);

    cairo_set_matrix(cairo, &strokePathClip->ctm);
    cairo_set_line_width(cairo, strokePathClip->line_width);
    cairo_set_dash(cairo, strokePathClip->dashes, strokePathClip->dash_count, strokePathClip->dash_offset);
    cairo_set_line_cap(cairo, strokePathClip->cap);
    cairo_set_line_join(cairo, strokePathClip->join);
    cairo_set_miter_limit(cairo, strokePathClip->miter);
    doPath(cairo, state, strokePathClip->path);
    cairo_stroke(cairo);

    cairo_restore(cairo);
}

void CairoOutputDev::beginString(GfxState *state, const GooString *s)
{
    int len = s->getLength();

    if (needFontUpdate) {
        updateFont(state);
    }

    if (!currentFont) {
        return;
    }

    glyphs = (cairo_glyph_t *)gmallocn(len, sizeof(cairo_glyph_t));
    glyphCount = 0;
    if (use_show_text_glyphs) {
        clusters = (cairo_text_cluster_t *)gmallocn(len, sizeof(cairo_text_cluster_t));
        clusterCount = 0;
        utf8Max = len * 2; // start with twice the number of glyphs. we will realloc if we need more.
        utf8 = (char *)gmalloc(utf8Max);
        utf8Count = 0;
    }
}

void CairoOutputDev::drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, const Unicode *u, int uLen)
{
    std::optional<int> glyphIndex;

    if (currentFont) {
        glyphIndex = currentFont->getGlyph(code, u, uLen);
        if (glyphIndex) {
            glyphs[glyphCount].index = *glyphIndex;
            glyphs[glyphCount].x = x - originX;
            glyphs[glyphCount].y = y - originY;
            glyphCount++;
        }
        if (use_show_text_glyphs) {
            const UnicodeMap *utf8Map = globalParams->getUtf8Map();
            if (utf8Max - utf8Count < uLen * 6) {
                // utf8 encoded characters can be up to 6 bytes
                if (utf8Max > uLen * 6) {
                    utf8Max *= 2;
                } else {
                    utf8Max += 2 * uLen * 6;
                }
                utf8 = (char *)grealloc(utf8, utf8Max);
            }
            clusters[clusterCount].num_bytes = 0;
            for (int i = 0; i < uLen; i++) {
                int size = utf8Map->mapUnicode(u[i], utf8 + utf8Count, utf8Max - utf8Count);
                utf8Count += size;
                clusters[clusterCount].num_bytes += size;
            }
            clusters[clusterCount].num_glyphs = 1;
            clusterCount++;
        }
    }

    if (!textPage) {
        return;
    }
    actualText->addChar(state, x, y, dx, dy, code, nBytes, u, uLen);
}

void CairoOutputDev::endString(GfxState *state)
{
    int render;

    if (!currentFont) {
        return;
    }

    // endString can be called without a corresponding beginString. If this
    // happens glyphs will be null so don't draw anything, just return.
    // XXX: OutputDevs should probably not have to deal with this...
    if (!glyphs) {
        return;
    }

    // ignore empty strings and invisible text -- this is used by
    // Acrobat Capture
    render = state->getRender();
    if (render == 3 || glyphCount == 0 || !text_matrix_valid) {
        goto finish;
    }

    if (state->getFont()->getType() == fontType3 && render != 7) {
        // If the current font is a type 3 font, we should ignore the text rendering mode
        // (and use the default of 0) as long as we are going to either fill or stroke.
        render = 0;
    }

    if (!(render & 1)) {
        LOG(printf("fill string\n"));
        cairo_set_source(cairo, fill_pattern);
        if (use_show_text_glyphs) {
            cairo_show_text_glyphs(cairo, utf8, utf8Count, glyphs, glyphCount, clusters, clusterCount, (cairo_text_cluster_flags_t)0);
        } else {
            cairo_show_glyphs(cairo, glyphs, glyphCount);
        }
        if (cairo_shape) {
            cairo_show_glyphs(cairo_shape, glyphs, glyphCount);
        }
    }

    // stroke
    if ((render & 3) == 1 || (render & 3) == 2) {
        LOG(printf("stroke string\n"));
        cairo_set_source(cairo, stroke_pattern);
        cairo_glyph_path(cairo, glyphs, glyphCount);
        cairo_stroke(cairo);
        if (cairo_shape) {
            cairo_glyph_path(cairo_shape, glyphs, glyphCount);
            cairo_stroke(cairo_shape);
        }
    }

    // clip
    if ((render & 4)) {
        LOG(printf("clip string\n"));
        // append the glyph path to textClipPath.

        // set textClipPath as the currentPath
        if (textClipPath) {
            cairo_append_path(cairo, textClipPath);
            if (cairo_shape) {
                cairo_append_path(cairo_shape, textClipPath);
            }
            cairo_path_destroy(textClipPath);
        }

        // append the glyph path
        cairo_glyph_path(cairo, glyphs, glyphCount);

        // move the path back into textClipPath
        // and clear the current path
        textClipPath = cairo_copy_path(cairo);
        cairo_new_path(cairo);
        if (cairo_shape) {
            cairo_new_path(cairo_shape);
        }
    }

finish:
    gfree(glyphs);
    glyphs = nullptr;
    if (use_show_text_glyphs) {
        gfree(clusters);
        clusters = nullptr;
        gfree(utf8);
        utf8 = nullptr;
    }
}

bool CairoOutputDev::beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, const Unicode *u, int uLen)
{

    cairo_save(cairo);
    cairo_matrix_t matrix;

    const double *ctm = state->getCTM();
    matrix.xx = ctm[0];
    matrix.yx = ctm[1];
    matrix.xy = ctm[2];
    matrix.yy = ctm[3];
    matrix.x0 = ctm[4];
    matrix.y0 = ctm[5];
    /* Restore the original matrix and then transform to matrix needed for the
     * type3 font. This is ugly but seems to work. Perhaps there is a better way to do it?*/
    cairo_set_matrix(cairo, &orig_matrix);
    cairo_transform(cairo, &matrix);
    if (cairo_shape) {
        cairo_save(cairo_shape);
        cairo_set_matrix(cairo_shape, &orig_matrix);
        cairo_transform(cairo_shape, &matrix);
    }
    cairo_pattern_destroy(stroke_pattern);
    cairo_pattern_reference(fill_pattern);
    stroke_pattern = fill_pattern;
    return false;
}

void CairoOutputDev::endType3Char(GfxState *state)
{
    cairo_restore(cairo);
    if (cairo_shape) {
        cairo_restore(cairo_shape);
    }
}

void CairoOutputDev::type3D0(GfxState *state, double wx, double wy)
{
    t3_glyph_wx = wx;
    t3_glyph_wy = wy;
    t3_glyph_has_color = true;
}

void CairoOutputDev::type3D1(GfxState *state, double wx, double wy, double llx, double lly, double urx, double ury)
{
    t3_glyph_wx = wx;
    t3_glyph_wy = wy;
    t3_glyph_bbox[0] = llx;
    t3_glyph_bbox[1] = lly;
    t3_glyph_bbox[2] = urx;
    t3_glyph_bbox[3] = ury;
    t3_glyph_has_bbox = true;
    t3_glyph_has_color = false;
}

void CairoOutputDev::beginTextObject(GfxState *state) { }

void CairoOutputDev::endTextObject(GfxState *state)
{
    if (textClipPath) {
        // clip the accumulated text path
        cairo_append_path(cairo, textClipPath);
        cairo_clip(cairo);
        if (cairo_shape) {
            cairo_append_path(cairo_shape, textClipPath);
            cairo_clip(cairo_shape);
        }
        cairo_path_destroy(textClipPath);
        textClipPath = nullptr;
    }
}

void CairoOutputDev::beginActualText(GfxState *state, const GooString *text)
{
    if (textPage) {
        actualText->begin(state, text);
    }
}

void CairoOutputDev::endActualText(GfxState *state)
{
    if (textPage) {
        actualText->end(state);
    }
}

static inline int splashRound(SplashCoord x)
{
    return (int)floor(x + 0.5);
}

static inline int splashCeil(SplashCoord x)
{
    return (int)ceil(x);
}

static inline int splashFloor(SplashCoord x)
{
    return (int)floor(x);
}

static cairo_surface_t *cairo_surface_create_similar_clip(cairo_t *cairo, cairo_content_t content)
{
    cairo_pattern_t *pattern;
    cairo_surface_t *surface = nullptr;

    cairo_push_group_with_content(cairo, content);
    pattern = cairo_pop_group(cairo);
    cairo_pattern_get_surface(pattern, &surface);
    cairo_surface_reference(surface);
    cairo_pattern_destroy(pattern);
    return surface;
}

void CairoOutputDev::beginTransparencyGroup(GfxState * /*state*/, const double * /*bbox*/, GfxColorSpace *blendingColorSpace, bool /*isolated*/, bool knockout, bool forSoftMask)
{
    /* push color space */
    ColorSpaceStack *css = new ColorSpaceStack;
    css->cs = blendingColorSpace;
    css->knockout = knockout;
    cairo_get_matrix(cairo, &css->group_matrix);
    css->next = groupColorSpaceStack;
    groupColorSpaceStack = css;

    LOG(printf("begin transparency group. knockout: %s\n", knockout ? "yes" : "no"));

    if (knockout) {
        knockoutCount++;
        if (!cairo_shape) {
            /* create a surface for tracking the shape */
            cairo_surface_t *cairo_shape_surface = cairo_surface_create_similar_clip(cairo, CAIRO_CONTENT_ALPHA);
            cairo_shape = cairo_create(cairo_shape_surface);
            cairo_surface_destroy(cairo_shape_surface);
            copyAntialias(cairo_shape, cairo);

            /* the color doesn't matter as long as it is opaque */
            cairo_set_source_rgb(cairo_shape, 0, 0, 0);
            cairo_matrix_t matrix;
            cairo_get_matrix(cairo, &matrix);
            cairo_set_matrix(cairo_shape, &matrix);
        }
    }
    if (groupColorSpaceStack->next && groupColorSpaceStack->next->knockout) {
        /* we need to track the shape */
        cairo_push_group(cairo_shape);
    }
    if (false && forSoftMask) {
        cairo_push_group_with_content(cairo, CAIRO_CONTENT_ALPHA);
    } else {
        cairo_push_group(cairo);
    }

    /* push_group has an implicit cairo_save() */
    if (knockout) {
        /*XXX: let's hope this matches the semantics needed */
        cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    } else {
        cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
    }
}

void CairoOutputDev::endTransparencyGroup(GfxState * /*state*/)
{
    if (group) {
        cairo_pattern_destroy(group);
    }
    group = cairo_pop_group(cairo);

    LOG(printf("end transparency group\n"));

    if (groupColorSpaceStack->next && groupColorSpaceStack->next->knockout) {
        if (shape) {
            cairo_pattern_destroy(shape);
        }
        shape = cairo_pop_group(cairo_shape);
    }
}

void CairoOutputDev::paintTransparencyGroup(GfxState * /*state*/, const double * /*bbox*/)
{
    LOG(printf("paint transparency group\n"));

    cairo_save(cairo);
    cairo_set_matrix(cairo, &groupColorSpaceStack->group_matrix);

    if (shape) {
        /* OPERATOR_SOURCE w/ a mask is defined as (src IN mask) ADD (dest OUT mask)
         * however our source has already been clipped to mask so we only need to
         * do ADD and OUT */

        /* clear the shape mask */
        cairo_set_source(cairo, shape);
        cairo_set_operator(cairo, CAIRO_OPERATOR_DEST_OUT);
        cairo_paint(cairo);
        cairo_set_operator(cairo, CAIRO_OPERATOR_ADD);
    }
    cairo_set_source(cairo, group);

    if (!mask) {
        cairo_paint_with_alpha(cairo, fill_opacity);
        cairo_status_t status = cairo_status(cairo);
        if (status) {
            printf("BAD status: %s\n", cairo_status_to_string(status));
        }
    } else {
        if (fill_opacity < 1.0) {
            cairo_push_group(cairo);
        }
        cairo_save(cairo);
        cairo_set_matrix(cairo, &mask_matrix);
        cairo_mask(cairo, mask);
        cairo_restore(cairo);
        if (fill_opacity < 1.0) {
            cairo_pop_group_to_source(cairo);
            cairo_paint_with_alpha(cairo, fill_opacity);
        }
        cairo_pattern_destroy(mask);
        mask = nullptr;
    }

    if (shape) {
        if (cairo_shape) {
            cairo_set_source(cairo_shape, shape);
            cairo_paint(cairo_shape);
            cairo_set_source_rgb(cairo_shape, 0, 0, 0);
        }
        cairo_pattern_destroy(shape);
        shape = nullptr;
    }

    popTransparencyGroup();
    cairo_restore(cairo);
}

static int luminocity(uint32_t x)
{
    int r = (x >> 16) & 0xff;
    int g = (x >> 8) & 0xff;
    int b = (x >> 0) & 0xff;
    // an arbitrary integer approximation of .3*r + .59*g + .11*b
    int y = (r * 19661 + g * 38666 + b * 7209 + 32829) >> 16;
    return y;
}

/* XXX: do we need to deal with shape here? */
void CairoOutputDev::setSoftMask(GfxState *state, const double *bbox, bool alpha, Function *transferFunc, GfxColor *backdropColor)
{
    cairo_pattern_destroy(mask);

    LOG(printf("set softMask\n"));

    if (!alpha || transferFunc) {
        /* We need to mask according to the luminocity of the group.
         * So we paint the group to an image surface convert it to a luminocity map
         * and then use that as the mask. */

        /* Get clip extents in device space */
        double x1, y1, x2, y2, x_min, y_min, x_max, y_max;
        cairo_clip_extents(cairo, &x1, &y1, &x2, &y2);
        cairo_user_to_device(cairo, &x1, &y1);
        cairo_user_to_device(cairo, &x2, &y2);
        x_min = MIN(x1, x2);
        y_min = MIN(y1, y2);
        x_max = MAX(x1, x2);
        y_max = MAX(y1, y2);
        cairo_clip_extents(cairo, &x1, &y1, &x2, &y2);
        cairo_user_to_device(cairo, &x1, &y2);
        cairo_user_to_device(cairo, &x2, &y1);
        x_min = MIN(x_min, MIN(x1, x2));
        y_min = MIN(y_min, MIN(y1, y2));
        x_max = MAX(x_max, MAX(x1, x2));
        y_max = MAX(y_max, MAX(y1, y2));

        int width = (int)(ceil(x_max) - floor(x_min));
        int height = (int)(ceil(y_max) - floor(y_min));

        /* Get group device offset */
        double x_offset, y_offset;
        if (cairo_get_group_target(cairo) == cairo_get_target(cairo)) {
            cairo_surface_get_device_offset(cairo_get_group_target(cairo), &x_offset, &y_offset);
        } else {
            cairo_surface_t *pats;
            cairo_pattern_get_surface(group, &pats);
            cairo_surface_get_device_offset(pats, &x_offset, &y_offset);
        }

        /* Adjust extents by group offset */
        x_min += x_offset;
        y_min += y_offset;

        cairo_surface_t *source = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cairo_t *maskCtx = cairo_create(source);
        copyAntialias(maskCtx, cairo);

        // XXX: hopefully this uses the correct color space */
        if (!alpha && groupColorSpaceStack->cs) {
            GfxRGB backdropColorRGB;
            groupColorSpaceStack->cs->getRGB(backdropColor, &backdropColorRGB);
            /* paint the backdrop */
            cairo_set_source_rgb(maskCtx, colToDbl(backdropColorRGB.r), colToDbl(backdropColorRGB.g), colToDbl(backdropColorRGB.b));
        }
        cairo_paint(maskCtx);

        /* Copy source ctm to mask ctm and translate origin so that the
         * mask appears it the same location on the source surface.  */
        cairo_matrix_t mat, tmat;
        cairo_matrix_init_translate(&tmat, -x_min, -y_min);
        cairo_get_matrix(cairo, &mat);
        cairo_matrix_multiply(&mat, &mat, &tmat);
        cairo_set_matrix(maskCtx, &mat);

        /* make the device offset of the new mask match that of the group */
        cairo_surface_set_device_offset(source, x_offset, y_offset);

        /* paint the group */
        cairo_set_source(maskCtx, group);
        cairo_paint(maskCtx);

        /* XXX status = cairo_status(maskCtx); */
        cairo_destroy(maskCtx);

        /* convert to a luminocity map */
        uint32_t *source_data = reinterpret_cast<uint32_t *>(cairo_image_surface_get_data(source));
        if (source_data) {
            /* get stride in units of 32 bits */
            ptrdiff_t stride = cairo_image_surface_get_stride(source) / 4;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int lum = alpha ? fill_opacity : luminocity(source_data[y * stride + x]);
                    if (transferFunc) {
                        double lum_in, lum_out;
                        lum_in = lum / 256.0;
                        transferFunc->transform(&lum_in, &lum_out);
                        lum = (int)(lum_out * 255.0 + 0.5);
                    }
                    source_data[y * stride + x] = lum << 24;
                }
            }
            cairo_surface_mark_dirty(source);
        }

        /* setup the new mask pattern */
        mask = cairo_pattern_create_for_surface(source);
        cairo_get_matrix(cairo, &mask_matrix);

        if (cairo_get_group_target(cairo) == cairo_get_target(cairo)) {
            cairo_pattern_set_matrix(mask, &mat);
        } else {
            cairo_matrix_t patMatrix;
            cairo_pattern_get_matrix(group, &patMatrix);
            /* Apply x_min, y_min offset to it appears in the same location as source. */
            cairo_matrix_multiply(&patMatrix, &patMatrix, &tmat);
            cairo_pattern_set_matrix(mask, &patMatrix);
        }

        cairo_surface_destroy(source);
    } else if (alpha) {
        mask = cairo_pattern_reference(group);
        cairo_get_matrix(cairo, &mask_matrix);
    }

    popTransparencyGroup();
}

void CairoOutputDev::popTransparencyGroup()
{
    /* pop color space */
    ColorSpaceStack *css = groupColorSpaceStack;
    if (css->knockout) {
        knockoutCount--;
        if (!knockoutCount) {
            /* we don't need to track the shape anymore because
             * we are not above any knockout groups */
            cairo_destroy(cairo_shape);
            cairo_shape = nullptr;
        }
    }
    groupColorSpaceStack = css->next;
    delete css;
}

void CairoOutputDev::clearSoftMask(GfxState * /*state*/)
{
    if (mask) {
        cairo_pattern_destroy(mask);
    }
    mask = nullptr;
}

/* Taken from cairo/doc/tutorial/src/singular.c */
static void get_singular_values(const cairo_matrix_t *matrix, double *major, double *minor)
{
    double xx = matrix->xx, xy = matrix->xy;
    double yx = matrix->yx, yy = matrix->yy;

    double a = xx * xx + yx * yx;
    double b = xy * xy + yy * yy;
    double k = xx * xy + yx * yy;

    double f = (a + b) * .5;
    double g = (a - b) * .5;
    double delta = sqrt(g * g + k * k);

    if (major) {
        *major = sqrt(f + delta);
    }
    if (minor) {
        *minor = sqrt(f - delta);
    }
}

void CairoOutputDev::getScaledSize(const cairo_matrix_t *matrix, int orig_width, int orig_height, int *scaledWidth, int *scaledHeight)
{
    double xScale;
    double yScale;
    if (orig_width > orig_height) {
        get_singular_values(matrix, &xScale, &yScale);
    } else {
        get_singular_values(matrix, &yScale, &xScale);
    }

    int tx, tx2, ty, ty2; /* the integer co-ordinates of the resulting image */
    if (xScale >= 0) {
        tx = splashRound(matrix->x0 - 0.01);
        tx2 = splashRound(matrix->x0 + xScale + 0.01) - 1;
    } else {
        tx = splashRound(matrix->x0 + 0.01) - 1;
        tx2 = splashRound(matrix->x0 + xScale - 0.01);
    }
    *scaledWidth = abs(tx2 - tx) + 1;
    // scaledWidth = splashRound(fabs(xScale));
    if (*scaledWidth == 0) {
        // technically, this should draw nothing, but it generally seems
        // better to draw a one-pixel-wide stripe rather than throwing it
        // away
        *scaledWidth = 1;
    }
    if (yScale >= 0) {
        ty = splashFloor(matrix->y0 + 0.01);
        ty2 = splashCeil(matrix->y0 + yScale - 0.01);
    } else {
        ty = splashCeil(matrix->y0 - 0.01);
        ty2 = splashFloor(matrix->y0 + yScale + 0.01);
    }
    *scaledHeight = abs(ty2 - ty);
    if (*scaledHeight == 0) {
        *scaledHeight = 1;
    }
}

cairo_filter_t CairoOutputDev::getFilterForSurface(cairo_surface_t *image, bool interpolate)
{
    if (interpolate) {
        return CAIRO_FILTER_GOOD;
    }

    int orig_width = cairo_image_surface_get_width(image);
    int orig_height = cairo_image_surface_get_height(image);
    if (orig_width == 0 || orig_height == 0) {
        return CAIRO_FILTER_NEAREST;
    }

    /* When printing, don't change the interpolation. */
    if (printing) {
        return CAIRO_FILTER_NEAREST;
    }

    cairo_matrix_t matrix;
    cairo_get_matrix(cairo, &matrix);
    int scaled_width, scaled_height;
    getScaledSize(&matrix, orig_width, orig_height, &scaled_width, &scaled_height);

    /* When scale factor is >= 400% we don't interpolate. See bugs #25268, #9860 */
    if (scaled_width / orig_width >= 4 || scaled_height / orig_height >= 4) {
        return CAIRO_FILTER_NEAREST;
    }

    return CAIRO_FILTER_GOOD;
}

void CairoOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool interpolate, bool inlineImg)
{

    /* FIXME: Doesn't the image mask support any colorspace? */
    cairo_set_source(cairo, fill_pattern);

    /* work around a cairo bug when scaling 1x1 surfaces */
    if (width == 1 && height == 1) {
        ImageStream *imgStr;
        unsigned char pix;
        int invert_bit;

        imgStr = new ImageStream(str, width, 1, 1);
        imgStr->reset();
        imgStr->getPixel(&pix);
        imgStr->close();
        delete imgStr;

        invert_bit = invert ? 1 : 0;
        if (pix ^ invert_bit) {
            return;
        }

        cairo_save(cairo);
        cairo_rectangle(cairo, 0., 0., width, height);
        cairo_fill(cairo);
        cairo_restore(cairo);
        if (cairo_shape) {
            cairo_save(cairo_shape);
            cairo_rectangle(cairo_shape, 0., 0., width, height);
            cairo_fill(cairo_shape);
            cairo_restore(cairo_shape);
        }
        return;
    }

    /* shape is 1.0 for painted areas, 0.0 for unpainted ones */

    cairo_matrix_t matrix;
    cairo_get_matrix(cairo, &matrix);
    // XXX: it is possible that we should only do sub pixel positioning if
    // we are rendering fonts */
    drawImageMaskRegular(state, ref, str, width, height, invert, interpolate, inlineImg);
}

void CairoOutputDev::setSoftMaskFromImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool inlineImg, double *baseMatrix)
{

    /* FIXME: Doesn't the image mask support any colorspace? */
    cairo_set_source(cairo, fill_pattern);

    /* work around a cairo bug when scaling 1x1 surfaces */
    if (width == 1 && height == 1) {
        ImageStream *imgStr;
        unsigned char pix;
        int invert_bit;

        imgStr = new ImageStream(str, width, 1, 1);
        imgStr->reset();
        imgStr->getPixel(&pix);
        imgStr->close();
        delete imgStr;

        invert_bit = invert ? 1 : 0;
        if (!(pix ^ invert_bit)) {
            cairo_save(cairo);
            cairo_rectangle(cairo, 0., 0., width, height);
            cairo_fill(cairo);
            cairo_restore(cairo);
            if (cairo_shape) {
                cairo_save(cairo_shape);
                cairo_rectangle(cairo_shape, 0., 0., width, height);
                cairo_fill(cairo_shape);
                cairo_restore(cairo_shape);
            }
        }
    } else {
        cairo_push_group_with_content(cairo, CAIRO_CONTENT_ALPHA);

        /* shape is 1.0 for painted areas, 0.0 for unpainted ones */

        cairo_matrix_t matrix;
        cairo_get_matrix(cairo, &matrix);
        // XXX: it is possible that we should only do sub pixel positioning if
        // we are rendering fonts */
        drawImageMaskRegular(state, ref, str, width, height, invert, false, inlineImg);

        if (state->getFillColorSpace()->getMode() == csPattern) {
            cairo_set_source_rgb(cairo, 1, 1, 1);
            cairo_set_matrix(cairo, &mask_matrix);
            cairo_mask(cairo, mask);
        }

        if (mask) {
            cairo_pattern_destroy(mask);
        }
        mask = cairo_pop_group(cairo);
    }

    saveState(state);
    double bbox[4] = { 0, 0, 1, 1 }; // dummy
    beginTransparencyGroup(state, bbox, state->getFillColorSpace(), true, false, false);
}

void CairoOutputDev::unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix)
{
    double bbox[4] = { 0, 0, 1, 1 }; // dummy

    endTransparencyGroup(state);
    restoreState(state);
    paintTransparencyGroup(state, bbox);
    clearSoftMask(state);
}

void CairoOutputDev::drawImageMaskRegular(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool interpolate, bool inlineImg)
{
    unsigned char *buffer;
    unsigned char *dest;
    cairo_surface_t *image;
    cairo_pattern_t *pattern;
    int x, y, i, bit;
    ImageStream *imgStr;
    unsigned char *pix;
    cairo_matrix_t matrix;
    int invert_bit;
    ptrdiff_t row_stride;
    cairo_filter_t filter;

    /* TODO: Do we want to cache these? */
    imgStr = new ImageStream(str, width, 1, 1);
    imgStr->reset();

    image = cairo_image_surface_create(CAIRO_FORMAT_A1, width, height);
    if (cairo_surface_status(image)) {
        goto cleanup;
    }

    buffer = cairo_image_surface_get_data(image);
    row_stride = cairo_image_surface_get_stride(image);

    invert_bit = invert ? 1 : 0;

    for (y = 0; y < height; y++) {
        pix = imgStr->getLine();
        dest = buffer + y * row_stride;
        i = 0;
        bit = 0;
        for (x = 0; x < width; x++) {
            if (bit == 0) {
                dest[i] = 0;
            }
            if (!(pix[x] ^ invert_bit)) {
#ifdef WORDS_BIGENDIAN
                dest[i] |= (1 << (7 - bit));
#else
                dest[i] |= (1 << bit);
#endif
            }
            bit++;
            if (bit > 7) {
                bit = 0;
                i++;
            }
        }
    }

    filter = getFilterForSurface(image, interpolate);

    cairo_surface_mark_dirty(image);
    pattern = cairo_pattern_create_for_surface(image);
    cairo_surface_destroy(image);
    if (cairo_pattern_status(pattern)) {
        goto cleanup;
    }

    LOG(printf("drawImageMask %dx%d\n", width, height));

    cairo_pattern_set_filter(pattern, filter);

    cairo_matrix_init_translate(&matrix, 0, height);
    cairo_matrix_scale(&matrix, width, -height);
    cairo_pattern_set_matrix(pattern, &matrix);
    if (cairo_pattern_status(pattern)) {
        cairo_pattern_destroy(pattern);
        goto cleanup;
    }

    if (state->getFillColorSpace()->getMode() == csPattern) {
        mask = cairo_pattern_reference(pattern);
        cairo_get_matrix(cairo, &mask_matrix);
    } else if (!printing) {
        cairo_save(cairo);
        cairo_rectangle(cairo, 0., 0., 1., 1.);
        cairo_clip(cairo);
        if (strokePathClip) {
            cairo_push_group(cairo);
            fillToStrokePathClip(state);
            cairo_pop_group_to_source(cairo);
        }
        cairo_mask(cairo, pattern);
        cairo_restore(cairo);
    } else {
        cairo_mask(cairo, pattern);
    }

    if (cairo_shape) {
        cairo_save(cairo_shape);
        cairo_set_source(cairo_shape, pattern);
        if (!printing) {
            cairo_rectangle(cairo_shape, 0., 0., 1., 1.);
            cairo_fill(cairo_shape);
        } else {
            cairo_mask(cairo_shape, pattern);
        }
        cairo_restore(cairo_shape);
    }

    cairo_pattern_destroy(pattern);

cleanup:
    imgStr->close();
    delete imgStr;
}

void CairoOutputDev::drawMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, bool maskInvert, bool maskInterpolate)
{
    ImageStream *maskImgStr, *imgStr;
    ptrdiff_t row_stride;
    unsigned char *maskBuffer, *buffer;
    unsigned char *maskDest;
    unsigned int *dest;
    cairo_surface_t *maskImage, *image;
    cairo_pattern_t *maskPattern, *pattern;
    cairo_matrix_t matrix;
    cairo_matrix_t maskMatrix;
    unsigned char *pix;
    int x, y;
    int invert_bit;
    cairo_filter_t filter;
    cairo_filter_t maskFilter;

    maskImgStr = new ImageStream(maskStr, maskWidth, 1, 1);
    maskImgStr->reset();

    maskImage = cairo_image_surface_create(CAIRO_FORMAT_A8, maskWidth, maskHeight);
    if (cairo_surface_status(maskImage)) {
        maskImgStr->close();
        delete maskImgStr;
        return;
    }

    maskBuffer = cairo_image_surface_get_data(maskImage);
    row_stride = cairo_image_surface_get_stride(maskImage);

    invert_bit = maskInvert ? 1 : 0;

    for (y = 0; y < maskHeight; y++) {
        pix = maskImgStr->getLine();
        maskDest = maskBuffer + y * row_stride;
        for (x = 0; x < maskWidth; x++) {
            if (pix[x] ^ invert_bit) {
                *maskDest++ = 0;
            } else {
                *maskDest++ = 255;
            }
        }
    }

    maskImgStr->close();
    delete maskImgStr;

    maskFilter = getFilterForSurface(maskImage, maskInterpolate);

    cairo_surface_mark_dirty(maskImage);
    maskPattern = cairo_pattern_create_for_surface(maskImage);
    cairo_surface_destroy(maskImage);
    if (cairo_pattern_status(maskPattern)) {
        return;
    }

#if 0
  /* ICCBased color space doesn't do any color correction
   * so check its underlying color space as well */
  int is_identity_transform;
  is_identity_transform = colorMap->getColorSpace()->getMode() == csDeviceRGB ||
		  (colorMap->getColorSpace()->getMode() == csICCBased &&
		   ((GfxICCBasedColorSpace*)colorMap->getColorSpace())->getAlt()->getMode() == csDeviceRGB);
#endif

    /* TODO: Do we want to cache these? */
    imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    imgStr->reset();

    image = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    if (cairo_surface_status(image)) {
        goto cleanup;
    }

    buffer = cairo_image_surface_get_data(image);
    row_stride = cairo_image_surface_get_stride(image);
    for (y = 0; y < height; y++) {
        dest = reinterpret_cast<unsigned int *>(buffer + y * row_stride);
        pix = imgStr->getLine();
        colorMap->getRGBLine(pix, dest, width);
    }

    filter = getFilterForSurface(image, interpolate);

    cairo_surface_mark_dirty(image);
    pattern = cairo_pattern_create_for_surface(image);
    cairo_surface_destroy(image);
    if (cairo_pattern_status(pattern)) {
        goto cleanup;
    }

    LOG(printf("drawMaskedImage %dx%d\n", width, height));

    cairo_pattern_set_filter(pattern, filter);
    cairo_pattern_set_filter(maskPattern, maskFilter);

    if (!printing) {
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
        cairo_pattern_set_extend(maskPattern, CAIRO_EXTEND_PAD);
    }

    cairo_matrix_init_translate(&matrix, 0, height);
    cairo_matrix_scale(&matrix, width, -height);
    cairo_pattern_set_matrix(pattern, &matrix);
    if (cairo_pattern_status(pattern)) {
        cairo_pattern_destroy(pattern);
        cairo_pattern_destroy(maskPattern);
        goto cleanup;
    }

    cairo_matrix_init_translate(&maskMatrix, 0, maskHeight);
    cairo_matrix_scale(&maskMatrix, maskWidth, -maskHeight);
    cairo_pattern_set_matrix(maskPattern, &maskMatrix);
    if (cairo_pattern_status(maskPattern)) {
        cairo_pattern_destroy(maskPattern);
        cairo_pattern_destroy(pattern);
        goto cleanup;
    }

    if (!printing) {
        cairo_save(cairo);
        cairo_set_source(cairo, pattern);
        cairo_rectangle(cairo, 0., 0., 1., 1.);
        cairo_clip(cairo);
        cairo_mask(cairo, maskPattern);
        cairo_restore(cairo);
    } else {
        cairo_set_source(cairo, pattern);
        cairo_mask(cairo, maskPattern);
    }

    if (cairo_shape) {
        cairo_save(cairo_shape);
        cairo_set_source(cairo_shape, pattern);
        if (!printing) {
            cairo_rectangle(cairo_shape, 0., 0., 1., 1.);
            cairo_fill(cairo_shape);
        } else {
            cairo_mask(cairo_shape, pattern);
        }
        cairo_restore(cairo_shape);
    }

    cairo_pattern_destroy(maskPattern);
    cairo_pattern_destroy(pattern);

cleanup:
    imgStr->close();
    delete imgStr;
}

static inline void getMatteColorRgb(GfxImageColorMap *colorMap, const GfxColor *matteColorIn, GfxRGB *matteColorRgb)
{
    colorMap->getColorSpace()->getRGB(matteColorIn, matteColorRgb);
    matteColorRgb->r = colToByte(matteColorRgb->r);
    matteColorRgb->g = colToByte(matteColorRgb->g);
    matteColorRgb->b = colToByte(matteColorRgb->b);
}

static inline void applyMask(unsigned int *imagePointer, int length, GfxRGB matteColor, unsigned char *alphaPointer)
{
    unsigned char *p, r, g, b;
    int i;

    for (i = 0, p = (unsigned char *)imagePointer; i < length; i++, p += 4, alphaPointer++) {
        if (*alphaPointer) {
            b = std::clamp(matteColor.b + (int)(p[0] - matteColor.b) * 255 / *alphaPointer, 0, 255);
            g = std::clamp(matteColor.g + (int)(p[1] - matteColor.g) * 255 / *alphaPointer, 0, 255);
            r = std::clamp(matteColor.r + (int)(p[2] - matteColor.r) * 255 / *alphaPointer, 0, 255);
            imagePointer[i] = (r << 16) | (g << 8) | (b << 0);
        }
    }
}

// XXX: is this affect by AIS(alpha is shape)?
void CairoOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, GfxImageColorMap *maskColorMap,
                                         bool maskInterpolate)
{
    ImageStream *maskImgStr, *imgStr;
    ptrdiff_t row_stride, mask_row_stride;
    unsigned char *maskBuffer, *buffer;
    unsigned char *maskDest;
    unsigned int *dest;
    cairo_surface_t *maskImage, *image;
    cairo_pattern_t *maskPattern, *pattern;
    cairo_matrix_t maskMatrix, matrix;
    unsigned char *pix;
    int y;
    cairo_filter_t filter;
    cairo_filter_t maskFilter;
    GfxRGB matteColorRgb;

    // Clamp heights to what Cairo can handle - Issue #991
    if (height >= MAX_CAIRO_IMAGE_SIZE) {
        error(errInternal, -1, "Reducing image height from {0:d} to {1:d} because of Cairo limits", height, MAX_CAIRO_IMAGE_SIZE - 1);
        height = MAX_CAIRO_IMAGE_SIZE - 1;
    }

    if (maskHeight >= MAX_CAIRO_IMAGE_SIZE) {
        error(errInternal, -1, "Reducing maskImage height from {0:d} to {1:d} because of Cairo limits", maskHeight, MAX_CAIRO_IMAGE_SIZE - 1);
        maskHeight = MAX_CAIRO_IMAGE_SIZE - 1;
    }

    const GfxColor *matteColor = maskColorMap->getMatteColor();
    if (matteColor != nullptr) {
        getMatteColorRgb(colorMap, matteColor, &matteColorRgb);
    }

    maskImgStr = new ImageStream(maskStr, maskWidth, maskColorMap->getNumPixelComps(), maskColorMap->getBits());
    maskImgStr->reset();

    maskImage = cairo_image_surface_create(CAIRO_FORMAT_A8, maskWidth, maskHeight);
    if (cairo_surface_status(maskImage)) {
        maskImgStr->close();
        delete maskImgStr;
        return;
    }

    maskBuffer = cairo_image_surface_get_data(maskImage);
    mask_row_stride = cairo_image_surface_get_stride(maskImage);
    for (y = 0; y < maskHeight; y++) {
        maskDest = (unsigned char *)(maskBuffer + y * mask_row_stride);
        pix = maskImgStr->getLine();
        if (likely(pix != nullptr)) {
            maskColorMap->getGrayLine(pix, maskDest, maskWidth);
        }
    }

    maskImgStr->close();
    delete maskImgStr;

    maskFilter = getFilterForSurface(maskImage, maskInterpolate);

    cairo_surface_mark_dirty(maskImage);
    maskPattern = cairo_pattern_create_for_surface(maskImage);
    cairo_surface_destroy(maskImage);
    if (cairo_pattern_status(maskPattern)) {
        return;
    }

#if 0
  /* ICCBased color space doesn't do any color correction
   * so check its underlying color space as well */
  int is_identity_transform;
  is_identity_transform = colorMap->getColorSpace()->getMode() == csDeviceRGB ||
		  (colorMap->getColorSpace()->getMode() == csICCBased &&
		   ((GfxICCBasedColorSpace*)colorMap->getColorSpace())->getAlt()->getMode() == csDeviceRGB);
#endif

    /* TODO: Do we want to cache these? */
    imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    imgStr->reset();

    image = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    if (cairo_surface_status(image)) {
        goto cleanup;
    }

    buffer = cairo_image_surface_get_data(image);
    row_stride = cairo_image_surface_get_stride(image);
    for (y = 0; y < height; y++) {
        dest = reinterpret_cast<unsigned int *>(buffer + y * row_stride);
        pix = imgStr->getLine();
        if (likely(pix != nullptr)) {
            colorMap->getRGBLine(pix, dest, width);
            if (matteColor != nullptr) {
                maskDest = (unsigned char *)(maskBuffer + y * mask_row_stride);
                applyMask(dest, width, matteColorRgb, maskDest);
            }
        }
    }

    filter = getFilterForSurface(image, interpolate);

    cairo_surface_mark_dirty(image);

    if (matteColor == nullptr) {
        setMimeData(state, str, ref, colorMap, image, height);
    }

    pattern = cairo_pattern_create_for_surface(image);
    cairo_surface_destroy(image);
    if (cairo_pattern_status(pattern)) {
        goto cleanup;
    }

    LOG(printf("drawSoftMaskedImage %dx%d\n", width, height));

    cairo_pattern_set_filter(pattern, filter);
    cairo_pattern_set_filter(maskPattern, maskFilter);

    if (!printing) {
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
        cairo_pattern_set_extend(maskPattern, CAIRO_EXTEND_PAD);
    }

    cairo_matrix_init_translate(&matrix, 0, height);
    cairo_matrix_scale(&matrix, width, -height);
    cairo_pattern_set_matrix(pattern, &matrix);
    if (cairo_pattern_status(pattern)) {
        cairo_pattern_destroy(pattern);
        cairo_pattern_destroy(maskPattern);
        goto cleanup;
    }

    cairo_matrix_init_translate(&maskMatrix, 0, maskHeight);
    cairo_matrix_scale(&maskMatrix, maskWidth, -maskHeight);
    cairo_pattern_set_matrix(maskPattern, &maskMatrix);
    if (cairo_pattern_status(maskPattern)) {
        cairo_pattern_destroy(maskPattern);
        cairo_pattern_destroy(pattern);
        goto cleanup;
    }

    if (fill_opacity != 1.0) {
        cairo_push_group(cairo);
    } else {
        cairo_save(cairo);
    }

    cairo_set_source(cairo, pattern);
    if (!printing) {
        cairo_rectangle(cairo, 0., 0., 1., 1.);
        cairo_clip(cairo);
    }
    cairo_mask(cairo, maskPattern);

    if (fill_opacity != 1.0) {
        cairo_pop_group_to_source(cairo);
        cairo_save(cairo);
        if (!printing) {
            cairo_rectangle(cairo, 0., 0., 1., 1.);
            cairo_clip(cairo);
        }
        cairo_paint_with_alpha(cairo, fill_opacity);
    }
    cairo_restore(cairo);

    if (cairo_shape) {
        cairo_save(cairo_shape);
        cairo_set_source(cairo_shape, pattern);
        if (!printing) {
            cairo_rectangle(cairo_shape, 0., 0., 1., 1.);
            cairo_fill(cairo_shape);
        } else {
            cairo_mask(cairo_shape, pattern);
        }
        cairo_restore(cairo_shape);
    }

    cairo_pattern_destroy(maskPattern);
    cairo_pattern_destroy(pattern);

cleanup:
    imgStr->close();
    delete imgStr;
}

bool CairoOutputDev::getStreamData(Stream *str, char **buffer, int *length)
{
    int len, i;
    char *strBuffer;

    len = 0;
    str->close();
    str->reset();
    while (str->getChar() != EOF) {
        len++;
    }
    if (len == 0) {
        return false;
    }

    strBuffer = (char *)gmalloc(len);

    str->close();
    str->reset();
    for (i = 0; i < len; ++i) {
        strBuffer[i] = str->getChar();
    }

    *buffer = strBuffer;
    *length = len;

    return true;
}

static bool colorMapHasIdentityDecodeMap(GfxImageColorMap *colorMap)
{
    for (int i = 0; i < colorMap->getNumPixelComps(); i++) {
        if (colorMap->getDecodeLow(i) != 0.0 || colorMap->getDecodeHigh(i) != 1.0) {
            return false;
        }
    }
    return true;
}

static cairo_status_t setMimeIdFromRef(cairo_surface_t *surface, const char *mime_type, const char *mime_id_prefix, Ref ref)
{
    GooString *mime_id;
    char *idBuffer;
    cairo_status_t status;

    mime_id = new GooString;

    if (mime_id_prefix) {
        mime_id->append(mime_id_prefix);
    }

    mime_id->appendf("{0:d}-{1:d}", ref.gen, ref.num);

    idBuffer = copyString(mime_id->c_str());
    status = cairo_surface_set_mime_data(surface, mime_type, (const unsigned char *)idBuffer, mime_id->getLength(), gfree, idBuffer);
    delete mime_id;
    if (status) {
        gfree(idBuffer);
    }
    return status;
}

bool CairoOutputDev::setMimeDataForJBIG2Globals(Stream *str, cairo_surface_t *image)
{
    JBIG2Stream *jb2Str = static_cast<JBIG2Stream *>(str);
    Object *globalsStr = jb2Str->getGlobalsStream();
    char *globalsBuffer;
    int globalsLength;

    // nothing to do for JBIG2 stream without Globals
    if (!globalsStr->isStream()) {
        return true;
    }

    if (setMimeIdFromRef(image, CAIRO_MIME_TYPE_JBIG2_GLOBAL_ID, nullptr, jb2Str->getGlobalsStreamRef())) {
        return false;
    }

    if (!getStreamData(globalsStr->getStream(), &globalsBuffer, &globalsLength)) {
        return false;
    }

    if (cairo_surface_set_mime_data(image, CAIRO_MIME_TYPE_JBIG2_GLOBAL, (const unsigned char *)globalsBuffer, globalsLength, gfree, (void *)globalsBuffer)) {
        gfree(globalsBuffer);
        return false;
    }

    return true;
}

bool CairoOutputDev::setMimeDataForCCITTParams(Stream *str, cairo_surface_t *image, int height)
{
    CCITTFaxStream *ccittStr = static_cast<CCITTFaxStream *>(str);

    GooString params;
    params.appendf("Columns={0:d}", ccittStr->getColumns());
    params.appendf(" Rows={0:d}", height);
    params.appendf(" K={0:d}", ccittStr->getEncoding());
    params.appendf(" EndOfLine={0:d}", ccittStr->getEndOfLine() ? 1 : 0);
    params.appendf(" EncodedByteAlign={0:d}", ccittStr->getEncodedByteAlign() ? 1 : 0);
    params.appendf(" EndOfBlock={0:d}", ccittStr->getEndOfBlock() ? 1 : 0);
    params.appendf(" BlackIs1={0:d}", ccittStr->getBlackIs1() ? 1 : 0);
    params.appendf(" DamagedRowsBeforeError={0:d}", ccittStr->getDamagedRowsBeforeError());

    char *p = strdup(params.c_str());
    if (cairo_surface_set_mime_data(image, CAIRO_MIME_TYPE_CCITT_FAX_PARAMS, (const unsigned char *)p, params.getLength(), gfree, (void *)p)) {
        gfree(p);
        return false;
    }

    return true;
}

void CairoOutputDev::setMimeData(GfxState *state, Stream *str, Object *ref, GfxImageColorMap *colorMap, cairo_surface_t *image, int height)
{
    char *strBuffer;
    int len;
    Object obj;
    StreamKind strKind = str->getKind();
    const char *mime_type;
    cairo_status_t status;

    if (!printing) {
        return;
    }

    // The cairo PS backend stores images with UNIQUE_ID in PS memory so the
    // image can be re-used multiple times. As we don't know how large the images are or
    // how many times they are used, there is no benefit in enabling this. Issue #106
    if (cairo_surface_get_type(cairo_get_target(cairo)) != CAIRO_SURFACE_TYPE_PS) {
        if (ref && ref->isRef()) {
            status = setMimeIdFromRef(image, CAIRO_MIME_TYPE_UNIQUE_ID, "poppler-surface-", ref->getRef());
            if (status) {
                return;
            }
        }
    }

    switch (strKind) {
    case strDCT:
        mime_type = CAIRO_MIME_TYPE_JPEG;
        break;
    case strJPX:
        mime_type = CAIRO_MIME_TYPE_JP2;
        break;
    case strJBIG2:
        mime_type = CAIRO_MIME_TYPE_JBIG2;
        break;
    case strCCITTFax:
        mime_type = CAIRO_MIME_TYPE_CCITT_FAX;
        break;
    default:
        mime_type = nullptr;
        break;
    }

    obj = str->getDict()->lookup("ColorSpace");
    std::unique_ptr<GfxColorSpace> colorSpace = GfxColorSpace::parse(nullptr, &obj, this, state);

    // colorspace in stream dict may be different from colorspace in jpx
    // data
    if (strKind == strJPX && colorSpace) {
        return;
    }

    // only embed mime data for gray, rgb, and cmyk colorspaces.
    if (colorSpace) {
        GfxColorSpaceMode mode = colorSpace->getMode();
        switch (mode) {
        case csDeviceGray:
        case csCalGray:
        case csDeviceRGB:
        case csDeviceRGBA:
        case csCalRGB:
        case csDeviceCMYK:
        case csICCBased:
            break;

        case csLab:
        case csIndexed:
        case csSeparation:
        case csDeviceN:
        case csPattern:
            return;
        }
    }

    if (!colorMapHasIdentityDecodeMap(colorMap)) {
        return;
    }

    if (strKind == strJBIG2 && !setMimeDataForJBIG2Globals(str, image)) {
        return;
    }

    if (strKind == strCCITTFax && !setMimeDataForCCITTParams(str, image, height)) {
        return;
    }

    if (mime_type) {
        if (getStreamData(str->getNextStream(), &strBuffer, &len)) {
            status = cairo_surface_set_mime_data(image, mime_type, (const unsigned char *)strBuffer, len, gfree, strBuffer);
        }

        if (status) {
            gfree(strBuffer);
        }
    }
}

class RescaleDrawImage : public CairoRescaleBox
{
private:
    ImageStream *imgStr;
    GfxRGB *lookup;
    int width;
    GfxImageColorMap *colorMap;
    const int *maskColors;
    int current_row;
    bool imageError;
    bool fromRGBA;

public:
    ~RescaleDrawImage() override;
    cairo_surface_t *getSourceImage(Stream *str, int widthA, int height, int scaledWidth, int scaledHeight, bool printing, GfxImageColorMap *colorMapA, const int *maskColorsA)
    {
        cairo_surface_t *image = nullptr;
        int i;

        lookup = nullptr;
        colorMap = colorMapA;
        maskColors = maskColorsA;
        width = widthA;
        current_row = -1;
        imageError = false;
        fromRGBA = colorMap->getColorSpace()->getMode() == csDeviceRGBA;

        /* TODO: Do we want to cache these? */
        imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
        imgStr->reset();

#if 0
    /* ICCBased color space doesn't do any color correction
     * so check its underlying color space as well */
    int is_identity_transform;
    is_identity_transform = colorMap->getColorSpace()->getMode() == csDeviceRGB ||
      (colorMap->getColorSpace()->getMode() == csICCBased &&
       ((GfxICCBasedColorSpace*)colorMap->getColorSpace())->getAlt()->getMode() == csDeviceRGB);
#endif

        // special case for one-channel (monochrome/gray/separation) images:
        // build a lookup table here
        if (colorMap->getNumPixelComps() == 1) {
            int n;
            unsigned char pix;

            n = 1 << colorMap->getBits();
            lookup = (GfxRGB *)gmallocn(n, sizeof(GfxRGB));
            for (i = 0; i < n; ++i) {
                pix = (unsigned char)i;

                colorMap->getRGB(&pix, &lookup[i]);
            }
        }

        bool needsCustomDownscaling = (width > MAX_CAIRO_IMAGE_SIZE || height > MAX_CAIRO_IMAGE_SIZE);

        if (printing) {
            if (width > MAX_PRINT_IMAGE_SIZE || height > MAX_PRINT_IMAGE_SIZE) {
                if (width > height) {
                    scaledWidth = MAX_PRINT_IMAGE_SIZE;
                    scaledHeight = MAX_PRINT_IMAGE_SIZE * (double)height / width;
                } else {
                    scaledHeight = MAX_PRINT_IMAGE_SIZE;
                    scaledWidth = MAX_PRINT_IMAGE_SIZE * (double)width / height;
                }
                needsCustomDownscaling = true;

                if (scaledWidth == 0) {
                    scaledWidth = 1;
                }
                if (scaledHeight == 0) {
                    scaledHeight = 1;
                }
            }
        }

        if (!needsCustomDownscaling || scaledWidth >= width || scaledHeight >= height) {
            // No downscaling. Create cairo image containing the source image data.
            unsigned char *buffer;
            ptrdiff_t stride;

            image = cairo_image_surface_create(maskColors || fromRGBA ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width, height);

            if (cairo_surface_status(image)) {
                goto cleanup;
            }

            buffer = cairo_image_surface_get_data(image);
            stride = cairo_image_surface_get_stride(image);
            for (int y = 0; y < height; y++) {
                uint32_t *dest = reinterpret_cast<uint32_t *>(buffer + y * stride);
                getRow(y, dest);
            }
        } else {
            // Downscaling required. Create cairo image the size of the
            // rescaled image and downscale the source image data into
            // the cairo image. downScaleImage() will call getRow() to read
            // source image data from the image stream. This avoids having
            // to create an image the size of the source image which may
            // exceed cairo's 32767x32767 image size limit (and also saves a
            // lot of memory).
            image = cairo_image_surface_create(maskColors || fromRGBA ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, scaledWidth, scaledHeight);
            if (cairo_surface_status(image)) {
                goto cleanup;
            }

            downScaleImage(width, height, scaledWidth, scaledHeight, 0, 0, scaledWidth, scaledHeight, image);
        }
        cairo_surface_mark_dirty(image);

    cleanup:
        gfree(lookup);
        imgStr->close();
        delete imgStr;
        return image;
    }

    void getRow(int row_num, uint32_t *row_data) override
    {
        unsigned char *pix;

        if (row_num <= current_row) {
            return;
        }

        while (current_row < row_num) {
            pix = imgStr->getLine();
            current_row++;
        }

        if (unlikely(pix == nullptr)) {
            memset(row_data, 0, width * 4);
            if (!imageError) {
                error(errInternal, -1, "Bad image stream");
                imageError = true;
            }
        } else if (lookup) {
            unsigned char *p = pix;
            GfxRGB rgb;

            for (int i = 0; i < width; i++) {
                rgb = lookup[*p];
                row_data[i] = ((int)colToByte(rgb.r) << 16) | ((int)colToByte(rgb.g) << 8) | ((int)colToByte(rgb.b) << 0);
                p++;
            }
        } else if (fromRGBA) {
            // Case of transparent JPX images, they contain RGBA data · Issue #1486
            GfxDeviceRGBAColorSpace *rgbaCS = dynamic_cast<GfxDeviceRGBAColorSpace *>(colorMap->getColorSpace());
            if (rgbaCS) {
                rgbaCS->getARGBPremultipliedLine(pix, row_data, width);
            } else {
                error(errSyntaxWarning, -1, "CairoOutputDev: Unexpected fallback from RGBA to RGB");
                colorMap->getRGBLine(pix, row_data, width);
            }
        } else {
            colorMap->getRGBLine(pix, row_data, width);
        }

        if (maskColors) {
            for (int x = 0; x < width; x++) {
                bool is_opaque = false;
                for (int i = 0; i < colorMap->getNumPixelComps(); ++i) {
                    if (pix[i] < maskColors[2 * i] || pix[i] > maskColors[2 * i + 1]) {
                        is_opaque = true;
                        break;
                    }
                }
                if (is_opaque) {
                    *row_data |= 0xff000000;
                } else {
                    *row_data = 0;
                }
                row_data++;
                pix += colorMap->getNumPixelComps();
            }
        }
    }
};

RescaleDrawImage::~RescaleDrawImage() = default;

void CairoOutputDev::drawImage(GfxState *state, Object *ref, Stream *str, int widthA, int heightA, GfxImageColorMap *colorMap, bool interpolate, const int *maskColors, bool inlineImg)
{
    cairo_surface_t *image;
    cairo_pattern_t *pattern, *maskPattern;
    cairo_matrix_t matrix;
    int width, height;
    int scaledWidth, scaledHeight;
    cairo_filter_t filter = CAIRO_FILTER_GOOD;
    RescaleDrawImage rescale;

    LOG(printf("drawImage %dx%d\n", widthA, heightA));

    cairo_get_matrix(cairo, &matrix);
    getScaledSize(&matrix, widthA, heightA, &scaledWidth, &scaledHeight);
    image = rescale.getSourceImage(str, widthA, heightA, scaledWidth, scaledHeight, printing, colorMap, maskColors);
    if (!image) {
        return;
    }

    width = cairo_image_surface_get_width(image);
    height = cairo_image_surface_get_height(image);
    if (width == widthA && height == heightA) {
        filter = getFilterForSurface(image, interpolate);
    }

    if (!inlineImg) { /* don't read stream twice if it is an inline image */
        setMimeData(state, str, ref, colorMap, image, heightA);
    }

    pattern = cairo_pattern_create_for_surface(image);
    cairo_surface_destroy(image);
    if (cairo_pattern_status(pattern)) {
        return;
    }

    cairo_pattern_set_filter(pattern, filter);

    if (!printing) {
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
    }

    cairo_matrix_init_translate(&matrix, 0, height);
    cairo_matrix_scale(&matrix, width, -height);
    cairo_pattern_set_matrix(pattern, &matrix);
    if (cairo_pattern_status(pattern)) {
        cairo_pattern_destroy(pattern);
        return;
    }

    if (!mask && fill_opacity != 1.0) {
        maskPattern = cairo_pattern_create_rgba(1., 1., 1., fill_opacity);
    } else if (mask) {
        maskPattern = cairo_pattern_reference(mask);
    } else {
        maskPattern = nullptr;
    }

    cairo_save(cairo);
    cairo_set_source(cairo, pattern);
    if (!printing) {
        cairo_rectangle(cairo, 0., 0., 1., 1.);
    }
    if (maskPattern) {
        if (!printing) {
            cairo_clip(cairo);
        }
        if (mask) {
            cairo_set_matrix(cairo, &mask_matrix);
        }
        cairo_mask(cairo, maskPattern);
    } else {
        if (printing) {
            cairo_paint(cairo);
        } else {
            cairo_fill(cairo);
        }
    }
    cairo_restore(cairo);

    cairo_pattern_destroy(maskPattern);

    if (cairo_shape) {
        cairo_save(cairo_shape);
        cairo_set_source(cairo_shape, pattern);
        if (printing) {
            cairo_paint(cairo_shape);
        } else {
            cairo_rectangle(cairo_shape, 0., 0., 1., 1.);
            cairo_fill(cairo_shape);
        }
        cairo_restore(cairo_shape);
    }

    cairo_pattern_destroy(pattern);
}

void CairoOutputDev::beginMarkedContent(const char *name, Dict *properties)
{
    if (!logicalStruct || !isPDF()) {
        return;
    }

    if (strcmp(name, "Artifact") == 0) {
        markedContentStack.emplace_back(name);
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
        cairo_tag_begin(cairo, name, nullptr);
#endif
        return;
    }

    int mcid = -1;
    if (properties) {
        properties->lookupInt("MCID", nullptr, &mcid);
    }

    if (mcid == -1) {
        return;
    }

    GooString attribs;
    attribs.appendf("tag_name='{0:s}' id='{1:d}_{2:d}'", name, currentStructParents, mcid);
    mcidEmitted.insert(std::pair<int, int>(currentStructParents, mcid));

    std::string tag;
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
    tag = CAIRO_TAG_CONTENT;
    cairo_tag_begin(cairo, CAIRO_TAG_CONTENT, attribs.c_str());
#endif

    markedContentStack.push_back(tag);
}

void CairoOutputDev::endMarkedContent(GfxState *state)
{
    if (!logicalStruct || !isPDF()) {
        return;
    }

    if (markedContentStack.empty()) {
        return;
    }

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
    cairo_tag_end(cairo, markedContentStack.back().c_str());
#endif
    markedContentStack.pop_back();
}

//------------------------------------------------------------------------
// ImageOutputDev
//------------------------------------------------------------------------

CairoImageOutputDev::CairoImageOutputDev()
{
    images = nullptr;
    numImages = 0;
    size = 0;
    imgDrawCbk = nullptr;
    imgDrawCbkData = nullptr;
}

CairoImageOutputDev::~CairoImageOutputDev()
{
    int i;

    for (i = 0; i < numImages; i++) {
        delete images[i];
    }
    gfree(static_cast<void *>(images));
}

void CairoImageOutputDev::saveImage(CairoImage *image)
{
    if (numImages >= size) {
        size += 16;
        images = (CairoImage **)greallocn(static_cast<void *>(images), size, sizeof(CairoImage *));
    }
    images[numImages++] = image;
}

void CairoImageOutputDev::getBBox(GfxState *state, int width, int height, double *x1, double *y1, double *x2, double *y2)
{
    const double *ctm = state->getCTM();
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, ctm[0], ctm[1], -ctm[2], -ctm[3], ctm[2] + ctm[4], ctm[3] + ctm[5]);

    int scaledWidth, scaledHeight;
    getScaledSize(&matrix, width, height, &scaledWidth, &scaledHeight);

    if (matrix.xx >= 0) {
        *x1 = matrix.x0;
    } else {
        *x1 = matrix.x0 - scaledWidth;
    }
    *x2 = *x1 + scaledWidth;

    if (matrix.yy >= 0) {
        *y1 = matrix.y0;
    } else {
        *y1 = matrix.y0 - scaledHeight;
    }
    *y2 = *y1 + scaledHeight;
}

void CairoImageOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool interpolate, bool inlineImg)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    double x1, y1, x2, y2;
    CairoImage *image;

    getBBox(state, width, height, &x1, &y1, &x2, &y2);

    image = new CairoImage(x1, y1, x2, y2);
    saveImage(image);

    if (imgDrawCbk && imgDrawCbk(numImages - 1, imgDrawCbkData)) {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        setCairo(cr);
        cairo_translate(cr, 0, height);
        cairo_scale(cr, width, -height);

        CairoOutputDev::drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
        image->setImage(surface);

        setCairo(nullptr);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}

void CairoImageOutputDev::setSoftMaskFromImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool inlineImg, double *baseMatrix)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    double x1, y1, x2, y2;
    CairoImage *image;

    getBBox(state, width, height, &x1, &y1, &x2, &y2);

    image = new CairoImage(x1, y1, x2, y2);
    saveImage(image);

    if (imgDrawCbk && imgDrawCbk(numImages - 1, imgDrawCbkData)) {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        setCairo(cr);
        cairo_translate(cr, 0, height);
        cairo_scale(cr, width, -height);

        CairoOutputDev::drawImageMask(state, ref, str, width, height, invert, inlineImg, false);
        if (state->getFillColorSpace()->getMode() == csPattern) {
            cairo_mask(cairo, mask);
        }
        image->setImage(surface);

        setCairo(nullptr);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}

void CairoImageOutputDev::drawImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, const int *maskColors, bool inlineImg)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    double x1, y1, x2, y2;
    CairoImage *image;

    getBBox(state, width, height, &x1, &y1, &x2, &y2);

    image = new CairoImage(x1, y1, x2, y2);
    saveImage(image);

    if (imgDrawCbk && imgDrawCbk(numImages - 1, imgDrawCbkData)) {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        setCairo(cr);
        cairo_translate(cr, 0, height);
        cairo_scale(cr, width, -height);

        CairoOutputDev::drawImage(state, ref, str, width, height, colorMap, interpolate, maskColors, inlineImg);
        image->setImage(surface);

        setCairo(nullptr);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}

void CairoImageOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, GfxImageColorMap *maskColorMap,
                                              bool maskInterpolate)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    double x1, y1, x2, y2;
    CairoImage *image;

    getBBox(state, width, height, &x1, &y1, &x2, &y2);

    image = new CairoImage(x1, y1, x2, y2);
    saveImage(image);

    if (imgDrawCbk && imgDrawCbk(numImages - 1, imgDrawCbkData)) {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        setCairo(cr);
        cairo_translate(cr, 0, height);
        cairo_scale(cr, width, -height);

        CairoOutputDev::drawSoftMaskedImage(state, ref, str, width, height, colorMap, interpolate, maskStr, maskWidth, maskHeight, maskColorMap, maskInterpolate);
        image->setImage(surface);

        setCairo(nullptr);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}

void CairoImageOutputDev::drawMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, bool maskInvert, bool maskInterpolate)
{
    cairo_t *cr;
    cairo_surface_t *surface;
    double x1, y1, x2, y2;
    CairoImage *image;

    getBBox(state, width, height, &x1, &y1, &x2, &y2);

    image = new CairoImage(x1, y1, x2, y2);
    saveImage(image);

    if (imgDrawCbk && imgDrawCbk(numImages - 1, imgDrawCbkData)) {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        setCairo(cr);
        cairo_translate(cr, 0, height);
        cairo_scale(cr, width, -height);

        CairoOutputDev::drawMaskedImage(state, ref, str, width, height, colorMap, interpolate, maskStr, maskWidth, maskHeight, maskInvert, maskInterpolate);
        image->setImage(surface);

        setCairo(nullptr);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}
