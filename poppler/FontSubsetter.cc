//========================================================================
//
// FontSubsetter.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2026 Ojas Maheshwari <workonlyojas@gmail.com>
// Copyright 2026 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#include <cstddef>
#include <optional>
#include <random>
#include "FontSubsetter.h"
#include "CharTypes.h"
#include "Error.h"
#include "PDFDoc.h"
#include "Stream.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-subset.h>
#include <unordered_set>
#include <GlobalParams.h>
#include <Form.h>
#include <Gfx.h>
#include <CharCodeToUnicode.h>
#include <fofi/FoFiTrueType.h>
#include <CIDFontsWidthsBuilder.h>

// helper for using std::visit to get a dependent false for static_asserts
// to help get compile errors if one ever extends variants
template<class>
inline constexpr bool always_false_v = false;

FontSubsetter::FontSubsetter(PDFDoc *docA) : doc(docA) { }

static void removeOldFont(const std::shared_ptr<const GfxFont> &font, XRef *xref)
{
    // We have to remove 5 things
    // - Font object
    // - Font Descriptor object
    // - Font Stream
    // - The old font refs stored in the global resources.
    // - CIDToGIDMap

    const Ref *fontObjRef = font->getID();
    Object fontObj = xref->fetch(*fontObjRef);

    Ref fontStreamRef = Ref::INVALID();
    font->getEmbeddedFontID(&fontStreamRef);

    // The Font Descriptor may be embedded directly or it might be an indirect object
    // See https://github.com/pdf-association/pdf-issues/issues/106#issuecomment-1118270213
    // If it's embedded directly inside the Font then deleting the font deletes the Font Descriptor as well
    // Otherwise we have to manually delete it ourselves
    std::optional<Ref> fontDescriptorRef;

    if (font->isCIDFont()) {
        Object descendantFontsObj = fontObj.dictLookup("DescendantFonts");

        // DescendantFonts must contain only 1 font according to spec
        if (!descendantFontsObj.isArray() || !descendantFontsObj.isArrayOfLength(1)) {
            error(errSyntaxError, -1, "removeOldFont: Invalid DescendantFonts array");
            return;
        }

        Object embeddedFont = descendantFontsObj.arrayGet(0).fetch(xref);

        if (!embeddedFont.isDict()) {
            error(errSyntaxError, -1, "removeOldFont: Invalid font inside DescendantFonts array");
            return;
        }

        const Object &fontDescValue = embeddedFont.dictLookupNF("FontDescriptor");

        if (fontDescValue.isRef()) {
            fontDescriptorRef = fontDescValue.getRef();
        }

        if (const Object &cidToGidMapRef = embeddedFont.dictLookupNF("CIDToGIDMap"); cidToGidMapRef.isRef()) {
            xref->removeIndirectObject(cidToGidMapRef.getRef());
        }
    } else {
        // Simple font, so FontDescriptor is directly inside the font object
        const Object &fontDescValue = fontObj.dictLookupNF("FontDescriptor");

        if (fontDescValue.isRef()) {
            fontDescriptorRef = fontDescValue.getRef();
        }
    }

    xref->removeIndirectObject(*fontObjRef);
    if (fontDescriptorRef.has_value()) {
        xref->removeIndirectObject(fontDescriptorRef.value());
    }
    xref->removeIndirectObject(fontStreamRef);
}

static void removeFontsFromFontDict(Dict *fontDict, std::unordered_set<Ref> &refSet)
{
    std::vector<std::string> keysToRemove;

    for (int i = 0; i < fontDict->getLength(); i++) {
        const std::string &key = fontDict->getKey(i);
        const Object &value = fontDict->getValNF(i);

        if (!value.isRef()) {
            continue;
        }

        Ref ref = value.getRef();
        if (refSet.contains(ref)) {
            keysToRemove.emplace_back(key);
        }
    }

    for (const std::string &key : keysToRemove) {
        fontDict->remove(key);
    }
}

static bool formFieldModified(const FormField *field, XRef *xref)
{
    int numChildren = field->getNumChildren();
    if (numChildren == 0) {
        int numWidgets = field->getNumWidgets();

        for (int i = 0; i < numWidgets; ++i) {
            if (FormWidget *wdg = field->getWidget(i); wdg != nullptr) {
                if (std::shared_ptr<AnnotWidget> wdgAnnot = wdg->getWidgetAnnotation(); wdgAnnot != nullptr) {
                    unsigned int id = wdgAnnot->getId();
                    if (xref->getEntry(id)->getFlag(XRefEntry::Updated)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    for (int i = 0; i < numChildren; ++i) {
        if (const FormField *childField = field->getChildren(i); childField != nullptr) {
            if (formFieldModified(childField, xref)) {
                return true;
            }
        }
    }

    return false;
}

void FontSubsetter::subsetAll() const
{
    XRef *xref = doc->getXRef();
    Form *form = doc->getCatalog()->getCreateForm();
    int numPages = doc->getNumPages();
    Object *acroForm = doc->getCatalog()->getAcroForm();

    // refSet contains refs of all unique fonts we want to remove
    // We are hashing by Ref and not GfxFont* because it might be that 2 different GfxFont* internally use the same font
    // We want to avoid double deletes
    std::unordered_set<Ref> refSet;
    std::vector<std::shared_ptr<const GfxFont>> fontsToRemove;

    /*
     * Do not perform font subsetting if a form field is also modified
     * This is because we might remove a font that is already getting used in a form field
     * TODO: Remove this code when we have font subsetting ready for form fields
     */
    bool anyFormFieldModified = false;
    for (int i = 0; i < form->getNumFields(); i++) {
        if (FormField *field = form->getRootField(i); field != nullptr) {
            if (formFieldModified(field, xref)) {
                anyFormFieldModified = true;
                break;
            }
        }
    }
    if (anyFormFieldModified) {
        return;
    }

    bool subsettingSuccessful = true;
    for (int i = 1; i <= numPages; i++) {
        Page *page = doc->getPage(i);

        Annots *annotsPtr = page->getAnnots();
        if (!annotsPtr) {
            continue;
        }

        auto annots = annotsPtr->getAnnots();

        for (const auto &annot : annots) {
            int annotId = annot->getId();
            if (!xref->getEntry(annotId)->getFlag(XRefEntry::Updated)) {
                continue;
            }

            if (annot->getType() == Annot::typeFreeText) {
                auto *ftAnnot = static_cast<AnnotFreeText *>(annot.get());

                auto fontsToRemoveLocal = ftAnnot->subsetFonts(this);
                if (fontsToRemoveLocal.empty()) {
                    subsettingSuccessful = false;
                }

                for (const auto &font : fontsToRemoveLocal) {
                    if (!refSet.contains(*font->getID())) {
                        fontsToRemove.emplace_back(font);
                        refSet.insert(*font->getID());
                    }
                }
            }
        }
    }

    /*
     * If subsetting fails for even one entity (annotation or form), it may not be safe to remove the original fonts since the entity might be using the old fonts
     */
    if (!subsettingSuccessful) {
        return;
    }

    // Remove all old font refs from the global resources
    if (acroForm && acroForm->isDict()) {
        if (Object dr = acroForm->dictLookup("DR"); dr.isDict()) {
            if (Object fontDictObj = dr.dictLookup("Font"); fontDictObj.isDict()) {
                Dict *fontDict = fontDictObj.getDict();
                removeFontsFromFontDict(fontDict, refSet);
            }
        }
    }

    // Scan local resources of all modified annotations and remove the old font refs
    for (int i = 1; i <= numPages; i++) {
        Page *page = doc->getPage(i);

        Annots *annotsPtr = page->getAnnots();
        if (!annotsPtr) {
            continue;
        }

        auto annots = annotsPtr->getAnnots();

        for (const auto &annot : annots) {
            int annotId = annot->getId();
            if (!xref->getEntry(annotId)->getFlag(XRefEntry::Updated)) {
                continue;
            }

            if (annot->getType() == Annot::typeFreeText) {
                auto *ftAnnot = static_cast<AnnotFreeText *>(annot.get());

                Object resourcesCopy = ftAnnot->getAppearanceResDict().deepCopy();
                if (!resourcesCopy.isDict()) {
                    continue;
                }

                Dict *fontDict = getFontDictFromResourcesDict(resourcesCopy, xref, (ftAnnot->getOpacity() != 1));
                if (!fontDict) {
                    continue;
                }

                removeFontsFromFontDict(fontDict, refSet);

                Object appearance = annot->getAppearance().fetch(xref);
                if (!appearance.isStream()) {
                    continue;
                }

                Dict *apDict = appearance.getStream()->getDict();
                apDict->set("Resources", std::move(resourcesCopy));
            }
        }
    }

    // Remove the actual font and it's associated objects
    for (const auto &font : fontsToRemove) {
        removeOldFont(font, xref);
    }
}

FontSubsetter::GetSubsetFontsResult FontSubsetter::getSubsetFonts(const FontStringMap &fontStringMap) const
{
    GetSubsetFontsResult result;
    XRef *xref = doc->getXRef();

    for (const auto &[font, codepoints] : fontStringMap) {
        if (!font || font->isSubset() || font->getTag().empty()) {
            continue;
        }

        Ref fontStreamRef = Ref::INVALID();
        font->getEmbeddedFontID(&fontStreamRef);

        if (fontStreamRef.num == -1 && fontStreamRef.gen == -1) {
            // The font is likely externally linked by name and not embedded
            continue;
        }

        const Ref *fontId = font->getID();
        if (!fontId) {
            error(errInternal, -1, "FontSubsetter::getSubsetFonts, font doesn't have an id");
            continue;
        }

        Object fontObj = xref->fetch(*fontId);

        if (!xref->getEntry(fontId->num)->getFlag(XRefEntry::Updated)) {
            // Since this is not a font we have added ourselves, we can skip it
            continue;
        }

        Object fontStreamObj = xref->fetch(fontStreamRef);
        if (!fontStreamObj.isStream()) {
            error(errInternal, -1, "FontSubsetter::getSubsetFonts, Invalid font stream");
            continue;
        }
        Stream *fontStream = fontStreamObj.getStream();

        std::string streamData;
        fontStream->fillString(streamData);

        SubsetFontResult subsettingResult = hbSubsetFont(streamData, codepoints);
        if (!subsettingResult.success) {
            error(errInternal, -1, "FontSubsetter::getSubsetFonts Unable to subset font");
            continue;
        }

        Ref newFontRef = Ref::INVALID();
        Object newFontObj = createNewSubsetFont(font.get(), std::move(subsettingResult), codepoints, newFontRef);

        if (newFontObj.isNull()) {
            continue;
        }

        result.fontsToRemove.emplace_back(font);
        result.tagRefMappings[font->getTag()] = newFontRef;
    }

    return result;
}

std::string FontSubsetter::getTaggedNameForFont(const GfxFont *font)
{
    const std::string &fontName = font->getEmbeddedFontName()->toStr();
    if (font->isSubset()) {
        return fontName;
    }

    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string tag;
    for (int i = 0; i < 6; ++i) {
        tag += chars[dist(gen)];
    }

    return tag + "+" + fontName;
}

static Ref createCIDToGIDMap(const std::shared_ptr<FoFiTrueType> &fft, const int unicodeBMPCMap, XRef *xref)
{
    static const int basicMultilingualMaxCode = 65535;
    std::vector<char> mapData;
    mapData.reserve(2 * basicMultilingualMaxCode);
    for (int code = 0; code <= basicMultilingualMaxCode; ++code) {
        const int glyph = fft->mapCodeToGID(unicodeBMPCMap, code);
        mapData.push_back(static_cast<char>(glyph >> 8));
        mapData.push_back(static_cast<char>(glyph & 0xff));
    }
    const Ref cidToGidMapStream = xref->addStreamObject(std::make_unique<Dict>(xref), std::move(mapData), StreamCompression::Compress);

    return cidToGidMapStream;
}

static std::unique_ptr<Array> createWidthArray(const GfxFont *oldFont, XRef *xref, const std::vector<Unicode> &unicodeValues)
{
    CIDFontsWidthsBuilder fontsWidths;

    const std::vector<Unicode> sortedUniqueUnicodeValues = [&unicodeValues]() {
        auto copy = unicodeValues;
        std::ranges::sort(copy);
        const auto [first, last] = std::ranges::unique(copy);
        copy.erase(first, last);
        return copy;
    }();

    for (Unicode code : sortedUniqueUnicodeValues) {
        fontsWidths.addWidth(code, oldFont->getWidth(code) * 1000);
    }

    auto widths = std::make_unique<Array>(xref);
    for (const auto &segment : fontsWidths.takeSegments()) {
        std::visit(
                [&widths, &xref](auto &&s) {
                    using T = std::decay_t<decltype(s)>;
                    if constexpr (std::is_same_v<T, CIDFontsWidthsBuilder::ListSegment>) {
                        widths->add(Object(s.first));
                        auto widthsInner = std::make_unique<Array>(xref);
                        for (const auto &w : s.widths) {
                            widthsInner->add(Object(w));
                        }
                        widths->add(Object(std::move(widthsInner)));
                    } else if constexpr (std::is_same_v<T, CIDFontsWidthsBuilder::RangeSegment>) {
                        widths->add(Object(s.first));
                        widths->add(Object(s.last));
                        widths->add(Object(s.width));
                    } else {
                        static_assert(always_false_v<T>, "non-exhaustive visitor");
                    }
                },
                segment);
    }

    return widths;
}

Object FontSubsetter::createNewSubsetFont(const GfxFont *oldFont, SubsetFontResult &&subsettingResult, const std::vector<Unicode> &unicodeValues, Ref &newFontRef) const
{
    XRef *xref = doc->getXRef();

    const Ref *fontObjRef = oldFont->getID();
    if (!fontObjRef) {
        error(errInternal, -1, "FontSubsetter::createNewSubsetFont, font doesn't have an id\n");
        return Object::null();
    }

    Object fontObj = xref->fetch(*fontObjRef);

    Object newFontObj = fontObj.deepCopy();
    if (!xref->getEntry(fontObjRef->num)->getFlag(XRefEntry::Updated)) {
        error(errInternal, -1, "FontSubsetter::createNewSubsetFont, Attempted to subset a font not added by poppler. This should not happen. Skipping..");
        return Object::null();
    }

    Ref cidToGidMapStream = Ref::INVALID();
    std::unique_ptr<Array> widths = nullptr;
    {
        const std::vector<char> &data = subsettingResult.data;
        std::vector<unsigned char> data_uc(data.size());
        for (size_t i = 0; i < data_uc.size(); i++) {
            data_uc[i] = static_cast<unsigned char>(data[i]);
        }

        std::span<unsigned char> data_span(data_uc);

        const std::shared_ptr<FoFiTrueType> fft = FoFiTrueType::make(data_span, 0);
        if (!fft) {
            error(errInternal, -1, "FontSubsetter::createNewSubsetFont, Unable to create FoFiTrueType from subset font data");
            return Object::null();
        }

        int unicodeBMPCMap = fft->findCmap(0, 3);
        if (unicodeBMPCMap < 0) {
            unicodeBMPCMap = fft->findCmap(3, 1);
        }
        if (unicodeBMPCMap < 0) {
            error(errInternal, -1, "FontSubsetter::createNewSubsetFont, Font subset does not have an unicode BMP cmap");
            return Object::null();
        }

        cidToGidMapStream = createCIDToGIDMap(fft, unicodeBMPCMap, xref);
        widths = createWidthArray(oldFont, xref, unicodeValues);
    }

    if (cidToGidMapStream == Ref::INVALID()) {
        error(errInternal, -1, "FontSubsetter::createNewSubsetFont, Unable to re-write the CIDToGIDMap. Skipping subsetting for font {0:s}.", oldFont->getName().value_or("NULL").c_str());
        return Object::null();
    }
    if (!widths) {
        error(errInternal, -1, "FontSubsetter::createNewSubsetFont, Unable to create new width array for subset font data");
        return Object::null();
    }

    Ref fontStreamRef;
    oldFont->getEmbeddedFontID(&fontStreamRef);
    Object fontStreamObj = xref->fetch(fontStreamRef);
    Dict *streamDict = fontStreamObj.getStream()->getDict();
    Object subtype = streamDict->lookup("Subtype");

    Ref newFontStreamRef;
    Object newFontStreamObj = createFontStreamFromData(std::move(subsettingResult.data), newFontStreamRef, subtype.isName() ? subtype.getNameString() : "");

    std::string taggedName = getTaggedNameForFont(oldFont);

    Object newFontDescObj;

    // Find font descriptor
    // Copy it and use it instead
    // Copying is needed to decouple the original font and subsetted font completely
    // otherwise they will be sharing the same FontDescriptor
    if (oldFont->isCIDFont()) {
        // FontDescriptor is inside DescendantFonts
        Object descendantFontsObj = newFontObj.dictLookup("DescendantFonts");

        // DescendantFonts must contain only 1 font according to spec
        if (!descendantFontsObj.isArray() || !descendantFontsObj.isArrayOfLength(1)) {
            error(errSyntaxError, -1, "createNewSubsetFont: Invalid DescendantFonts array");
            return Object::null();
        }

        Object embeddedFont = descendantFontsObj.arrayGet(0);

        if (Object baseFont = embeddedFont.dictLookup("BaseFont"); !baseFont.isNull()) {
            embeddedFont.dictSet("BaseFont", Object::name(taggedName));
        }

        const Object &fontDescValue = embeddedFont.dictLookupNF("FontDescriptor");

        // The value might be an indirect reference or embedded directly
        // Whether the spec mandates it to be an indirect reference is slightly unclear as of now
        // See https://github.com/pdf-association/pdf-issues/issues/106#issuecomment-1118270213
        if (fontDescValue.isRef()) {
            Object fontDescObj = embeddedFont.dictLookup("FontDescriptor");

            newFontDescObj = fontDescObj.deepCopy();
            Ref newFontDescRef = xref->addIndirectObject(newFontDescObj);

            embeddedFont.dictSet("FontDescriptor", Object(newFontDescRef));
        } else {
            Object fontDescObj = embeddedFont.dictLookup("FontDescriptor");
            newFontDescObj = std::move(fontDescObj);
        }

        embeddedFont.dictSet("CIDToGIDMap", Object(cidToGidMapStream));
        embeddedFont.dictSet("W", Object(std::move(widths)));
    } else {
        // Simple font, so FontDescriptor is directly inside the font object
        const Object &fontDescValue = newFontObj.dictLookupNF("FontDescriptor");

        if (fontDescValue.isRef()) {
            Object fontDescObj = newFontObj.dictLookup("FontDescriptor");

            newFontDescObj = fontDescObj.deepCopy();
            Ref newFontDescRef = xref->addIndirectObject(newFontDescObj);

            newFontObj.dictSet("FontDescriptor", Object(newFontDescRef));
        } else {
            Object fontDescObj = newFontObj.dictLookup("FontDescriptor");
            newFontDescObj = std::move(fontDescObj);
        }
    }

    if (newFontDescObj.isNull() || newFontDescObj.isNone()) {
        return Object::null();
    }

    newFontDescObj.dictSet("BaseFont", Object::name(taggedName));

    std::string fontFileName;
    GfxFontType fontType = oldFont->getType();
    if (fontType == GfxFontType::fontType1) {
        fontFileName = "FontFile";
    } else if (fontType == GfxFontType::fontTrueType || fontType == GfxFontType::fontTrueTypeOT || fontType == GfxFontType::fontCIDType2 || fontType == GfxFontType::fontCIDType2OT) {
        fontFileName = "FontFile2";
    } else {
        fontFileName = "FontFile3";
    }

    if (fontFileName.empty()) {
        error(errSyntaxError, -1, "createNewSubsetFont: Invalid or unsupported fontType for subsetting");
        return Object::null();
    }

    newFontDescObj.dictSet(fontFileName, Object(newFontStreamRef));
    newFontObj.dictSet("BaseFont", Object::name(taggedName));
    newFontDescObj.dictSet("FontName", Object::name(taggedName));

    newFontRef = xref->addIndirectObject(newFontObj);

    return newFontObj;
}

Object FontSubsetter::createFontStreamFromData(std::vector<char> &&data, Ref &ref, const std::string &subtype) const
{
    XRef *xref = doc->getXRef();

    auto streamDict = std::make_unique<Dict>(xref);
    streamDict->set("Length", Object(static_cast<long long>(data.size())));
    if (!subtype.empty()) {
        streamDict->set("Subtype", Object::name(subtype));
    }

    ref = xref->addStreamObject(std::move(streamDict), std::move(data), StreamCompression::Compress);
    Object streamObj = xref->fetch(ref);

    return streamObj;
}

FontSubsetter::SubsetFontResult FontSubsetter::hbSubsetFont(std::string &fontStream, const std::vector<Unicode> &unicodeValues)
{
    hb_blob_t *blob = hb_blob_create(fontStream.data(), fontStream.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr);

    hb_face_t *face = hb_face_create(blob, 0);

    hb_subset_input_t *input = hb_subset_input_create_or_fail();

    if (!input) {
        hb_blob_destroy(blob);
        hb_face_destroy(face);
        return { .success = false, .data = std::vector<char>() };
    }

    hb_set_t *unicodes = hb_subset_input_unicode_set(input);

    for (Unicode val : unicodeValues) {
        hb_set_add(unicodes, val);
    }

    hb_face_t *subset = hb_subset_or_fail(face, input);

    if (!subset) {
        hb_blob_destroy(blob);
        hb_face_destroy(face);
        hb_subset_input_destroy(input);
        return { .success = false, .data = std::vector<char>() };
    }

    hb_blob_t *subsetBlob = hb_face_reference_blob(subset);

    unsigned int length = 0;
    const char *subsetData = hb_blob_get_data(subsetBlob, &length);

    std::vector<char> data(subsetData, subsetData + length);

    hb_blob_destroy(blob);
    hb_face_destroy(face);
    hb_subset_input_destroy(input);
    hb_face_destroy(subset);
    hb_blob_destroy(subsetBlob);

    return { .success = true, .data = data };
}
