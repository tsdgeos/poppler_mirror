//========================================================================
//
// Page.cc
//
// Copyright 1996-2007 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2005-2013, 2016-2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006-2008 Pino Toscano <pino@kde.org>
// Copyright (C) 2006 Nickolay V. Shmyrev <nshmyrev@yandex.ru>
// Copyright (C) 2006 Scott Turner <scotty1024@mac.com>
// Copyright (C) 2006-2011, 2015 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2007 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2008 Iñigo Martínez <inigomartinez@gmail.com>
// Copyright (C) 2008 Brad Hards <bradh@kde.org>
// Copyright (C) 2008 Ilya Gorenbein <igorenbein@finjan.com>
// Copyright (C) 2012, 2013 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013, 2014 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2013, 2017, 2023 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2015 Philipp Reinkemeier <philipp.reinkemeier@offis.de>
// Copyright (C) 2018, 2019 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2020 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2020, 2021, 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2020 Philipp Knechtges <philipp-dev@knechtges.com>
// Copyright (C) 2024 Pablo Correa Gómez <ablocorrea@hotmail.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <climits>
#include "GlobalParams.h"
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "PDFDoc.h"
#include "XRef.h"
#include "Link.h"
#include "OutputDev.h"
#include "Gfx.h"
#include "GfxState.h"
#include "Annot.h"
#include "TextOutputDev.h"
#include "Form.h"
#include "Error.h"
#include "Page.h"
#include "Catalog.h"
#include "goo/gmem.h"

//------------------------------------------------------------------------
// PDFRectangle
//------------------------------------------------------------------------

namespace {
namespace testing {
static_assert(PDFRectangle {} == PDFRectangle {});
static_assert(PDFRectangle { 0, 0, 1, 1 } == PDFRectangle { 0, 0, 1, 1 });
static_assert(PDFRectangle { 0, 0, 1, 1 }.isValid());
static_assert(!PDFRectangle { 0, 0, 0, 0 }.isValid());
static_assert(PDFRectangle { 0, 0, 2, 2 }.contains(1, 1));
static_assert(!PDFRectangle { 0, 0, 1, 2 }.contains(2, 1));

constexpr PDFRectangle r1 { 0, 0, 5, 5 };
constexpr PDFRectangle r2 { 2, 2, 4, 4 };
constexpr PDFRectangle r3 { 0, 3, 3, 3 };
constexpr PDFRectangle r4 { 2, 3, 3, 3 };
static_assert(r1.isValid());
static_assert([]() {
    auto r = r1;
    r.clipTo(r1);
    return r == r1;
}());
static_assert([]() {
    auto r = r1;
    r.clipTo(r2);
    return r == r2;
}());
static_assert([]() {
    auto r = r2;
    r.clipTo(r1);
    return r == r2;
}());
static_assert([]() {
    auto r = r2;
    r.clipTo(r3);
    return r == r4;
}());
} // namespace testing
} // namespace

//------------------------------------------------------------------------
// PageAttrs
//------------------------------------------------------------------------

PageAttrs::PageAttrs(const PageAttrs *attrs, Dict *dict)
{
    Object obj1;
    PDFRectangle mBox;
    const bool isPage = dict->is("Page");

    // get old/default values
    if (attrs) {
        mediaBox = attrs->mediaBox;
        cropBox = attrs->cropBox;
        haveCropBox = attrs->haveCropBox;
        rotate = attrs->rotate;
        resources = attrs->resources.copy();
    } else {
        // set default MediaBox to 8.5" x 11" -- this shouldn't be necessary
        // but some (non-compliant) PDF files don't specify a MediaBox
        mediaBox.x1 = 0;
        mediaBox.y1 = 0;
        mediaBox.x2 = 612;
        mediaBox.y2 = 792;
        cropBox.x1 = cropBox.y1 = cropBox.x2 = cropBox.y2 = 0;
        haveCropBox = false;
        rotate = 0;
        resources.setToNull();
    }

    // media box
    if (readBox(dict, "MediaBox", &mBox)) {
        mediaBox = mBox;
    }

    // crop box
    if (readBox(dict, "CropBox", &cropBox)) {
        haveCropBox = true;
    }
    if (!haveCropBox) {
        cropBox = mediaBox;
    }

    if (isPage) {
        // cropBox can not be bigger than mediaBox
        if (cropBox.x2 - cropBox.x1 > mediaBox.x2 - mediaBox.x1) {
            cropBox.x1 = mediaBox.x1;
            cropBox.x2 = mediaBox.x2;
        }
        if (cropBox.y2 - cropBox.y1 > mediaBox.y2 - mediaBox.y1) {
            cropBox.y1 = mediaBox.y1;
            cropBox.y2 = mediaBox.y2;
        }
    }

    // other boxes
    bleedBox = cropBox;
    readBox(dict, "BleedBox", &bleedBox);
    trimBox = cropBox;
    readBox(dict, "TrimBox", &trimBox);
    artBox = cropBox;
    readBox(dict, "ArtBox", &artBox);

    // rotate
    obj1 = dict->lookup("Rotate");
    if (obj1.isInt()) {
        rotate = obj1.getInt();
    }
    while (rotate < 0) {
        rotate += 360;
    }
    while (rotate >= 360) {
        rotate -= 360;
    }

    // misc attributes
    lastModified = dict->lookup("LastModified");
    boxColorInfo = dict->lookup("BoxColorInfo");
    group = dict->lookup("Group");
    metadata = dict->lookup("Metadata");
    pieceInfo = dict->lookup("PieceInfo");
    separationInfo = dict->lookup("SeparationInfo");

    // resource dictionary
    Object objResources = dict->lookup("Resources");
    if (objResources.isDict()) {
        resources = std::move(objResources);
    }
}

PageAttrs::~PageAttrs() = default;

void PageAttrs::clipBoxes()
{
    cropBox.clipTo(mediaBox);
    bleedBox.clipTo(mediaBox);
    trimBox.clipTo(mediaBox);
    artBox.clipTo(mediaBox);
}

bool PageAttrs::readBox(Dict *dict, const char *key, PDFRectangle *box)
{
    PDFRectangle tmp;
    double t;
    Object obj1, obj2;
    bool ok;

    obj1 = dict->lookup(key);
    if (obj1.isArray() && obj1.arrayGetLength() == 4) {
        ok = true;
        obj2 = obj1.arrayGet(0);
        if (obj2.isNum()) {
            tmp.x1 = obj2.getNum();
        } else {
            ok = false;
        }
        obj2 = obj1.arrayGet(1);
        if (obj2.isNum()) {
            tmp.y1 = obj2.getNum();
        } else {
            ok = false;
        }
        obj2 = obj1.arrayGet(2);
        if (obj2.isNum()) {
            tmp.x2 = obj2.getNum();
        } else {
            ok = false;
        }
        obj2 = obj1.arrayGet(3);
        if (obj2.isNum()) {
            tmp.y2 = obj2.getNum();
        } else {
            ok = false;
        }
        if (tmp.x1 == 0 && tmp.x2 == 0 && tmp.y1 == 0 && tmp.y2 == 0) {
            ok = false;
        }
        if (ok) {
            if (tmp.x1 > tmp.x2) {
                t = tmp.x1;
                tmp.x1 = tmp.x2;
                tmp.x2 = t;
            }
            if (tmp.y1 > tmp.y2) {
                t = tmp.y1;
                tmp.y1 = tmp.y2;
                tmp.y2 = t;
            }
            *box = tmp;
        }
    } else {
        ok = false;
    }
    return ok;
}

//------------------------------------------------------------------------
// Page
//------------------------------------------------------------------------

#define pageLocker() const std::scoped_lock locker(mutex)

Page::Page(PDFDoc *docA, int numA, Object &&pageDict, Ref pageRefA, std::unique_ptr<PageAttrs> attrsA) : pageRef(pageRefA), attrs(std::move(attrsA))
{
    ok = true;
    doc = docA;
    xref = doc->getXRef();
    num = numA;
    duration = -1;
    annots = nullptr;
    structParents = -1;

    pageObj = std::move(pageDict);

    // get attributes
    attrs->clipBoxes();

    // transtion
    trans = pageObj.dictLookupNF("Trans").copy();
    if (!(trans.isRef() || trans.isDict() || trans.isNull())) {
        error(errSyntaxError, -1, "Page transition object (page {0:d}) is wrong type ({1:s})", num, trans.getTypeName());
        trans = Object();
    }

    // duration
    const Object &tmp = pageObj.dictLookupNF("Dur");
    if (!(tmp.isNum() || tmp.isNull())) {
        error(errSyntaxError, -1, "Page duration object (page {0:d}) is wrong type ({1:s})", num, tmp.getTypeName());
    } else if (tmp.isNum()) {
        duration = tmp.getNum();
    }

    // structParents
    const Object &tmp2 = pageObj.dictLookup("StructParents");
    if (!(tmp2.isInt() || tmp2.isNull())) {
        error(errSyntaxError, -1, "Page StructParents object (page {0:d}) is wrong type ({1:s})", num, tmp2.getTypeName());
    } else if (tmp2.isInt()) {
        structParents = tmp2.getInt();
    }

    // annotations
    annotsObj = pageObj.dictLookupNF("Annots").copy();
    if (!(annotsObj.isRef() || annotsObj.isArray() || annotsObj.isNull())) {
        error(errSyntaxError, -1, "Page annotations object (page {0:d}) is wrong type ({1:s})", num, annotsObj.getTypeName());
        annotsObj.setToNull();
    }

    if (annotsObj.isArray() && annotsObj.arrayGetLength() > 10000) {
        error(errSyntaxError, -1, "Page annotations object (page {0:d}) is likely malformed. Too big: ({1:d})", num, annotsObj.arrayGetLength());
        goto err2;
    }
    if (annotsObj.isRef()) {
        auto resolvedObj = getAnnotsObject();
        if (resolvedObj.isArray() && resolvedObj.arrayGetLength() > 10000) {
            error(errSyntaxError, -1, "Page annotations object (page {0:d}) is likely malformed. Too big: ({1:d})", num, resolvedObj.arrayGetLength());
            goto err2;
        }
        if (!resolvedObj.isArray() && !resolvedObj.isNull()) {
            error(errSyntaxError, -1, "Page annotations object (page {0:d}) is wrong type ({1:s})", num, resolvedObj.getTypeName());
            annotsObj.setToNull();
        }
    }

    // contents
    contents = pageObj.dictLookupNF("Contents").copy();
    if (!(contents.isRef() || contents.isArray() || contents.isNull())) {
        error(errSyntaxError, -1, "Page contents object (page {0:d}) is wrong type ({1:s})", num, contents.getTypeName());
        goto err1;
    }

    // thumb
    thumb = pageObj.dictLookupNF("Thumb").copy();
    if (!(thumb.isStream() || thumb.isNull() || thumb.isRef())) {
        error(errSyntaxError, -1, "Page thumb object (page {0:d}) is wrong type ({1:s})", num, thumb.getTypeName());
        thumb.setToNull();
    }

    // actions
    actions = pageObj.dictLookupNF("AA").copy();
    if (!(actions.isDict() || actions.isNull())) {
        error(errSyntaxError, -1, "Page additional action object (page {0:d}) is wrong type ({1:s})", num, actions.getTypeName());
        actions.setToNull();
    }

    return;

err2:
    annotsObj.setToNull();
err1:
    contents.setToNull();
    ok = false;
}

Page::~Page() = default;

Dict *Page::getResourceDict()
{
    return attrs->getResourceDict();
}

Object *Page::getResourceDictObject()
{
    return attrs->getResourceDictObject();
}

Dict *Page::getResourceDictCopy(XRef *xrefA)
{
    pageLocker();
    Dict *dict = attrs->getResourceDict();
    return dict ? dict->copy(xrefA) : nullptr;
}

void Page::replaceXRef(XRef *xrefA)
{
    Dict *pageDict = pageObj.getDict()->copy(xrefA);
    xref = xrefA;
    trans = pageDict->lookupNF("Trans").copy();
    annotsObj = pageDict->lookupNF("Annots").copy();
    contents = pageDict->lookupNF("Contents").copy();
    if (contents.isArray()) {
        contents = Object(contents.getArray()->copy(xrefA));
    }
    thumb = pageDict->lookupNF("Thumb").copy();
    actions = pageDict->lookupNF("AA").copy();
    Object resources = pageDict->lookup("Resources");
    if (resources.isDict()) {
        attrs->replaceResource(std::move(resources));
    }
    delete pageDict;
}

/* Loads standalone fields into Page, should be called once per page only */
void Page::loadStandaloneFields(Form *form)
{
    /* Look for standalone annots, identified by being: 1) of type Widget
     * 2) not referenced from the Catalog's Form Field array */
    for (const std::shared_ptr<Annot> &annot : annots->getAnnots()) {

        if (annot->getType() != Annot::typeWidget || !annot->getHasRef()) {
            continue;
        }

        const Ref r = annot->getRef();
        if (form && form->findWidgetByRef(r)) {
            continue; // this annot is referenced inside Form, skip it
        }

        std::set<int> parents;
        std::unique_ptr<FormField> field = Form::createFieldFromDict(annot->getAnnotObj().copy(), annot->getDoc(), r, nullptr, &parents);

        if (field && field->getNumWidgets() == 1) {

            auto aw = std::static_pointer_cast<AnnotWidget>(annot);
            aw->setField(field.get());

            field->setStandAlone(true);
            FormWidget *formWidget = field->getWidget(0);

            if (!formWidget->getWidgetAnnotation()) {
                formWidget->setWidgetAnnotation(aw);
            }

            standaloneFields.push_back(std::move(field));
        }
    }
}

Annots *Page::getAnnots(XRef *xrefA)
{
    if (!annots) {
        Object obj = getAnnotsObject(xrefA);
        annots = std::make_unique<Annots>(doc, num, &obj);
        // Load standalone fields once for the page
        loadStandaloneFields(doc->getCatalog()->getForm());
    }

    return annots.get();
}

bool Page::addAnnot(const std::shared_ptr<Annot> &annot)
{
    if (unlikely(xref->getEntry(pageRef.num)->type == xrefEntryFree)) {
        // something very wrong happened if we're here
        error(errInternal, -1, "Can not addAnnot to page with an invalid ref");
        return false;
    }

    const Ref annotRef = annot->getRef();

    // Make sure we have annots before adding the new one
    // even if it's an empty list so that we can safely
    // call annots->appendAnnot(annot)
    pageLocker();
    getAnnots();

    if (annotsObj.isNull()) {
        Ref annotsRef;
        // page doesn't have annots array,
        // we have to create it

        auto *annotsArray = new Array(xref);
        annotsArray->add(Object(annotRef));

        annotsRef = xref->addIndirectObject(Object(annotsArray));
        annotsObj = Object(annotsRef);
        pageObj.dictSet("Annots", Object(annotsRef));
        xref->setModifiedObject(&pageObj, pageRef);
    } else {
        Object obj1 = getAnnotsObject();
        if (obj1.isArray()) {
            obj1.arrayAdd(Object(annotRef));
            if (annotsObj.isRef()) {
                xref->setModifiedObject(&obj1, annotsObj.getRef());
            } else {
                xref->setModifiedObject(&pageObj, pageRef);
            }
        }
    }

    // Popup annots are already handled by markup annots,
    // so add to the list only Popup annots without a
    // markup annotation associated.
    if (annot->getType() != Annot::typePopup || !static_cast<AnnotPopup *>(annot.get())->hasParent()) {
        annots->appendAnnot(annot);
    }
    annot->setPage(num, true);

    auto *annotMarkup = dynamic_cast<AnnotMarkup *>(annot.get());
    if (annotMarkup) {
        std::shared_ptr<AnnotPopup> annotPopup = annotMarkup->getPopup();
        if (annotPopup) {
            addAnnot(annotPopup);
        }
    }

    return true;
}

void Page::removeAnnot(const std::shared_ptr<Annot> &annot)
{
    Ref annotRef = annot->getRef();

    pageLocker();
    Object annArray = getAnnotsObject();
    if (annArray.isArray()) {
        int idx = -1;
        // Get annotation position
        for (int i = 0; idx == -1 && i < annArray.arrayGetLength(); ++i) {
            const Object &tmp = annArray.arrayGetNF(i);
            if (tmp.isRef()) {
                const Ref currAnnot = tmp.getRef();
                if (currAnnot == annotRef) {
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            error(errInternal, -1, "Annotation doesn't belong to this page");
            return;
        }
        annots->removeAnnot(annot); // Gracefully fails on popup windows
        annArray.arrayRemove(idx);

        if (annotsObj.isRef()) {
            xref->setModifiedObject(&annArray, annotsObj.getRef());
        } else {
            xref->setModifiedObject(&pageObj, pageRef);
        }
    }
    annot->removeReferencedObjects(); // Note: Might recurse in removeAnnot again
    if (annArray.isArray()) {
        xref->removeIndirectObject(annotRef);
    }
    annot->setPage(0, false);
}

std::unique_ptr<Links> Page::getLinks()
{
    return std::make_unique<Links>(getAnnots());
}

std::unique_ptr<FormPageWidgets> Page::getFormWidgets()
{
    auto frmPageWidgets = std::make_unique<FormPageWidgets>(getAnnots(), num, doc->getCatalog()->getForm());
    frmPageWidgets->addWidgets(standaloneFields, num);

    return frmPageWidgets;
}

void Page::display(OutputDev *out, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, bool printing, bool (*abortCheckCbk)(void *data), void *abortCheckCbkData, bool (*annotDisplayDecideCbk)(Annot *annot, void *user_data),
                   void *annotDisplayDecideCbkData, bool copyXRef)
{
    displaySlice(out, hDPI, vDPI, rotate, useMediaBox, crop, -1, -1, -1, -1, printing, abortCheckCbk, abortCheckCbkData, annotDisplayDecideCbk, annotDisplayDecideCbkData, copyXRef);
}

std::unique_ptr<Gfx> Page::createGfx(OutputDev *out, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, int sliceX, int sliceY, int sliceW, int sliceH, bool (*abortCheckCbk)(void *data), void *abortCheckCbkData, XRef *xrefA)
{
    const PDFRectangle *mediaBox, *cropBox;
    PDFRectangle box;

    rotate += getRotate();
    if (rotate >= 360) {
        rotate -= 360;
    } else if (rotate < 0) {
        rotate += 360;
    }

    makeBox(hDPI, vDPI, rotate, useMediaBox, out->upsideDown(), sliceX, sliceY, sliceW, sliceH, &box, &crop);
    cropBox = getCropBox();
    mediaBox = getMediaBox();

    if (globalParams->getPrintCommands()) {
        printf("***** MediaBox = ll:%g,%g ur:%g,%g\n", mediaBox->x1, mediaBox->y1, mediaBox->x2, mediaBox->y2);
        printf("***** CropBox = ll:%g,%g ur:%g,%g\n", cropBox->x1, cropBox->y1, cropBox->x2, cropBox->y2);
        printf("***** Rotate = %d\n", attrs->getRotate());
    }

    if (!crop) {
        crop = (box == *cropBox) && out->needClipToCropBox();
    }
    return std::make_unique<Gfx>(doc, out, num, attrs->getResourceDict(), hDPI, vDPI, &box, crop ? cropBox : nullptr, rotate, abortCheckCbk, abortCheckCbkData, xrefA);
}

void Page::displaySlice(OutputDev *out, double hDPI, double vDPI, int rotate, bool useMediaBox, bool crop, int sliceX, int sliceY, int sliceW, int sliceH, bool printing, bool (*abortCheckCbk)(void *data), void *abortCheckCbkData,
                        bool (*annotDisplayDecideCbk)(Annot *annot, void *user_data), void *annotDisplayDecideCbkData, bool copyXRef)
{
    Annots *annotList;

    if (!out->checkPageSlice(this, hDPI, vDPI, rotate, useMediaBox, crop, sliceX, sliceY, sliceW, sliceH, printing, abortCheckCbk, abortCheckCbkData, annotDisplayDecideCbk, annotDisplayDecideCbkData)) {
        return;
    }
    pageLocker();
    XRef *localXRef = (copyXRef) ? xref->copy() : xref;
    if (copyXRef) {
        replaceXRef(localXRef);
    }

    std::unique_ptr<Gfx> gfx = createGfx(out, hDPI, vDPI, rotate, useMediaBox, crop, sliceX, sliceY, sliceW, sliceH, abortCheckCbk, abortCheckCbkData, localXRef);

    Object obj = contents.fetch(localXRef);
    if (!obj.isNull()) {
        gfx->saveState();
        gfx->display(&obj);
        gfx->restoreState();
    } else {
        // empty pages need to call dump to do any setup required by the
        // OutputDev
        out->dump();
    }

    // draw annotations
    annotList = getAnnots();

    if (!annotList->getAnnots().empty()) {
        if (globalParams->getPrintCommands()) {
            printf("***** Annotations\n");
        }
        for (const std::shared_ptr<Annot> &annot : annots->getAnnots()) {
            if ((annotDisplayDecideCbk && (*annotDisplayDecideCbk)(annot.get(), annotDisplayDecideCbkData)) || !annotDisplayDecideCbk) {
                annot->draw(gfx.get(), printing);
            }
        }
        out->dump();
    }

    if (copyXRef) {
        replaceXRef(doc->getXRef());
        delete localXRef;
    }
}

void Page::display(Gfx *gfx)
{
    Object obj = contents.fetch(xref);
    if (!obj.isNull()) {
        gfx->saveState();
        gfx->display(&obj);
        gfx->restoreState();
    }
}

bool Page::loadThumb(unsigned char **data_out, int *width_out, int *height_out, int *rowstride_out)
{
    unsigned int pixbufdatasize;
    int width, height, bits;
    Object obj1;
    Dict *dict;
    Stream *str;

    /* Get stream dict */
    pageLocker();
    Object fetched_thumb = thumb.fetch(xref);
    if (!fetched_thumb.isStream()) {
        return false;
    }

    dict = fetched_thumb.streamGetDict();
    str = fetched_thumb.getStream();

    if (!dict->lookupInt("Width", "W", &width)) {
        return false;
    }
    if (!dict->lookupInt("Height", "H", &height)) {
        return false;
    }
    if (!dict->lookupInt("BitsPerComponent", "BPC", &bits)) {
        return false;
    }

    /* Check for invalid dimensions and integer overflow. */
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (width > INT_MAX / 3 / height) {
        return false;
    }
    pixbufdatasize = width * height * 3;

    /* Get color space */
    obj1 = dict->lookup("ColorSpace");
    if (obj1.isNull()) {
        obj1 = dict->lookup("CS");
    }
    // Just initialize some dummy GfxState for GfxColorSpace::parse.
    // This will set a sRGB profile for ICC-based colorspaces.
    auto pdfrectangle = std::make_shared<PDFRectangle>();
    auto state = std::make_shared<GfxState>(72.0, 72.0, pdfrectangle.get(), 0, false);
    std::unique_ptr<GfxColorSpace> colorSpace = GfxColorSpace::parse(nullptr, &obj1, nullptr, state.get());
    if (!colorSpace) {
        fprintf(stderr, "Error: Cannot parse color space\n");
        return false;
    }

    obj1 = dict->lookup("Decode");
    if (obj1.isNull()) {
        obj1 = dict->lookup("D");
    }
    GfxImageColorMap colorMap(bits, &obj1, std::move(colorSpace));
    if (!colorMap.isOk()) {
        fprintf(stderr, "Error: invalid colormap\n");
        return false;
    }

    if (data_out) {
        ImageStream imgstr { str, width, colorMap.getNumPixelComps(), colorMap.getBits() };
        if (!imgstr.rewind()) {
            return false;
        }
        auto *pixbufdata = (unsigned char *)gmalloc(pixbufdatasize);
        unsigned char *p = pixbufdata;
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                unsigned char pix[gfxColorMaxComps];
                GfxRGB rgb;

                imgstr.getPixel(pix);
                colorMap.getRGB(pix, &rgb);

                *p++ = colToByte(rgb.r);
                *p++ = colToByte(rgb.g);
                *p++ = colToByte(rgb.b);
            }
        }
        *data_out = pixbufdata;
        imgstr.close();
    }

    if (width_out) {
        *width_out = width;
    }
    if (height_out) {
        *height_out = height;
    }
    if (rowstride_out) {
        *rowstride_out = width * 3;
    }
    return true;
}

void Page::makeBox(double hDPI, double vDPI, int rotate, bool useMediaBox, bool upsideDown, double sliceX, double sliceY, double sliceW, double sliceH, PDFRectangle *box, bool *crop) const
{
    const PDFRectangle *mediaBox, *cropBox, *baseBox;
    double kx, ky;

    mediaBox = getMediaBox();
    cropBox = getCropBox();
    if (sliceW >= 0 && sliceH >= 0) {
        baseBox = useMediaBox ? mediaBox : cropBox;
        kx = 72.0 / hDPI;
        ky = 72.0 / vDPI;
        if (rotate == 90) {
            if (upsideDown) {
                box->x1 = baseBox->x1 + ky * sliceY;
                box->x2 = baseBox->x1 + ky * (sliceY + sliceH);
            } else {
                box->x1 = baseBox->x2 - ky * (sliceY + sliceH);
                box->x2 = baseBox->x2 - ky * sliceY;
            }
            box->y1 = baseBox->y1 + kx * sliceX;
            box->y2 = baseBox->y1 + kx * (sliceX + sliceW);
        } else if (rotate == 180) {
            box->x1 = baseBox->x2 - kx * (sliceX + sliceW);
            box->x2 = baseBox->x2 - kx * sliceX;
            if (upsideDown) {
                box->y1 = baseBox->y1 + ky * sliceY;
                box->y2 = baseBox->y1 + ky * (sliceY + sliceH);
            } else {
                box->y1 = baseBox->y2 - ky * (sliceY + sliceH);
                box->y2 = baseBox->y2 - ky * sliceY;
            }
        } else if (rotate == 270) {
            if (upsideDown) {
                box->x1 = baseBox->x2 - ky * (sliceY + sliceH);
                box->x2 = baseBox->x2 - ky * sliceY;
            } else {
                box->x1 = baseBox->x1 + ky * sliceY;
                box->x2 = baseBox->x1 + ky * (sliceY + sliceH);
            }
            box->y1 = baseBox->y2 - kx * (sliceX + sliceW);
            box->y2 = baseBox->y2 - kx * sliceX;
        } else {
            box->x1 = baseBox->x1 + kx * sliceX;
            box->x2 = baseBox->x1 + kx * (sliceX + sliceW);
            if (upsideDown) {
                box->y1 = baseBox->y2 - ky * (sliceY + sliceH);
                box->y2 = baseBox->y2 - ky * sliceY;
            } else {
                box->y1 = baseBox->y1 + ky * sliceY;
                box->y2 = baseBox->y1 + ky * (sliceY + sliceH);
            }
        }
    } else if (useMediaBox) {
        *box = *mediaBox;
    } else {
        *box = *cropBox;
        *crop = false;
    }
}

void Page::processLinks(OutputDev *out)
{
    std::unique_ptr<Links> links = getLinks();
    for (const std::shared_ptr<AnnotLink> &link : links->getLinks()) {
        out->processLink(link.get());
    }
}

void Page::getDefaultCTM(double *ctm, double hDPI, double vDPI, int rotate, bool useMediaBox, bool upsideDown) const
{
    GfxState *state;
    int i;
    rotate += getRotate();
    if (rotate >= 360) {
        rotate -= 360;
    } else if (rotate < 0) {
        rotate += 360;
    }
    state = new GfxState(hDPI, vDPI, useMediaBox ? getMediaBox() : getCropBox(), rotate, upsideDown);
    for (i = 0; i < 6; ++i) {
        ctm[i] = state->getCTM()[i];
    }
    delete state;
}

std::unique_ptr<LinkAction> Page::getAdditionalAction(PageAdditionalActionsType type)
{
    Object additionalActionsObject = actions.fetch(doc->getXRef());
    if (additionalActionsObject.isDict()) {
        const char *key = (type == actionOpenPage ? "O" : type == actionClosePage ? "C" : nullptr);

        Object actionObject = additionalActionsObject.dictLookup(key);
        if (actionObject.isDict()) {
            return LinkAction::parseAction(&actionObject, doc->getCatalog()->getBaseURI());
        }
    }

    return nullptr;
}
