//========================================================================
//
// fontsubsetting-basic-test.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2026 Ojas Maheshwari <workonlyojas@gmail.com>
//========================================================================

#include "GlobalParams.h"
#include "PDFDocFactory.h"
#include "goo/GooString.h"
#include <Form.h>
#include <FontSubsetter.h>
#include <ft2build.h>
#include FT_FREETYPE_H

static std::vector<unsigned long> convertUnicodetoNormalStr(const std::string &s)
{
    std::vector<unsigned long> res;
    size_t i = 0;

    if (s.size() > 2) {
        if (s.starts_with("\xFE\xFF")) {
            i = 2;
        }
    }

    while (i < s.size()) {
        unsigned long unicodeByte = (static_cast<unsigned char>(s[i]) << 8) | (static_cast<unsigned char>(s[i + 1]));
        res.emplace_back(unicodeByte);

        i += 2;
    }
    return res;
}

static std::string getFontDataFromFont(std::shared_ptr<GfxFont> &font, XRef *xref)
{
    Ref streamRef;
    bool isEmbedded = font->getEmbeddedFontID(&streamRef);

    if (!isEmbedded) {
        // Font may be externally linked
        return "";
    }

    Object streamObj = xref->fetch(streamRef);
    Stream *stream = streamObj.getStream();

    std::string s;
    stream->fillString(s);

    return s;
}

static void setDefaultAppearanceToNative(AnnotFreeText *ftextann, Page *pdfPage)
{
    const double pointSize = 12;
    auto textColor = std::make_unique<AnnotColor>(0, 0, 0);
    std::string fontName = "Invalid_font";
    std::string textFont = "Noto Sans";

    Form *form = pdfPage->getDoc()->getCatalog()->getCreateForm();
    if (form) {
        fontName = form->findFontInDefaultResources(textFont, "");
        if (fontName.empty()) {
            fontName = form->addFontToDefaultResources(textFont, "").fontName;
        }

        if (!fontName.empty()) {
            form->ensureFontsForAllCharacters(ftextann->getContents().toStr(), fontName);
        } else {
            fontName = "Invalid_font";
        }
    }

    assert(fontName != "Invalid_font");

    DefaultAppearance da { fontName, pointSize, std::move(textColor) };
    ftextann->setDefaultAppearance(da);
}

struct GlyphsResult
{
    bool ok;
    int numGlyphs = 0;
    std::vector<unsigned long> codepoints;
};
static GlyphsResult getGlyphsInfo(std::string &s)
{
    FT_Library ft_library;
    FT_Error ft_error = FT_Init_FreeType(&ft_library);
    if (ft_error) {
        return { .ok = false, .numGlyphs = 0, .codepoints = {} };
    }
    const std::unique_ptr<FT_Library, void (*)(FT_Library *)> freetypeLibDeleter(&ft_library, [](FT_Library *l) { FT_Done_FreeType(*l); });

    const auto *data = const_cast<const FT_Byte *>(reinterpret_cast<FT_Byte *>(s.data()));

    FT_Face face;
    ft_error = FT_New_Memory_Face(ft_library, data, s.size(), 0, &face);
    if (ft_error) {
        return { .ok = false, .numGlyphs = -1, .codepoints = {} };
    }
    const std::unique_ptr<FT_Face, void (*)(FT_Face *)> faceDeleter(&face, [](FT_Face *f) { FT_Done_Face(*f); });

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    FT_ULong charcode;
    FT_UInt gid;

    charcode = FT_Get_First_Char(face, &gid);

    int numGlyphs = 0;
    std::vector<unsigned long> codepoints;
    while (gid != 0) {
        numGlyphs++;
        codepoints.emplace_back(charcode);
        charcode = FT_Get_Next_Char(face, charcode, &gid);
    }

    return { .ok = true, .numGlyphs = numGlyphs, .codepoints = codepoints };
}

static std::vector<std::shared_ptr<GfxFont>> getAllFontsFromResources(const Dict *fontDict, XRef *xref)
{
    std::vector<std::shared_ptr<GfxFont>> result;

    for (int i = 0; i < fontDict->getLength(); i++) {
        Object fontObj = fontDict->getVal(i);
        Ref fontId = fontDict->getValNF(i).getRef();

        auto font = GfxFont::makeFont(xref, fontDict->getKey(i), fontId, *fontObj.getDict());

        result.emplace_back(std::shared_ptr<GfxFont>(std::move(font)));
    }

    return result;
}

static bool match(std::vector<unsigned long> &v1, std::vector<unsigned long> &v2)
{
    std::set<unsigned long> s1(v1.begin(), v1.end());
    std::set<unsigned long> s2(v2.begin(), v2.end());

    if (s1.size() != s2.size()) {
        return false;
    }

    auto it1 = s1.begin();
    auto it2 = s2.begin();

    while (it1 != s1.end() && it2 != s2.end()) {
        if (*it1 != *it2) {
            return false;
        }
        it1++;
        it2++;
    }

    return true;
}

/*
 * Checks if font subsetting is properly done for an annotation
 * Goes over all the fonts used by the annotation and collects all the glyphs
 * Checks if the glyphs are only what we need
 */
static bool subsettingWorksInAnnot(Annot *annot, const std::string &text, XRef *xref, bool transparency)
{
    Object resources = annot->getAppearanceResDict();
    const Dict *fontDict = getFontDictFromResourcesDict(resources, xref, transparency);
    if (!fontDict) {
        return false;
    }

    auto allFonts = getAllFontsFromResources(fontDict, xref);

    std::vector<unsigned long> allChars;
    for (auto &font : allFonts) {
        std::string data = getFontDataFromFont(font, xref);
        if (data.empty()) {
            continue;
        }

        auto result = getGlyphsInfo(data);
        if (!result.ok) {
            return false;
        }

        for (unsigned long code : result.codepoints) {
            allChars.emplace_back(code);
        }
    }

    std::vector<unsigned long> neededChars = convertUnicodetoNormalStr(text);
    return match(allChars, neededChars);
}

// Test functions

/*
 * - Add a freetext annotation with some text.
 * - Save the file.
 * - Check if the font used by the annotation only has characters for the text we entered.
 */
static bool testSingleAnnotationSubsetting(std::shared_ptr<GooString> &filename)
{
    /*
     * This is a very temporary workaround to use unicode characters such as japanese and chinese for the test
     * If we simply do const char *TEXT = "Hello 1234 日本語";
     * This is bad because the last 3 characters simply can't be represented in a single byte
     * unicodeVals here is an integer list representation of an already unicode encoded string containing "Hello 1234 日本語"
     * TODO: Change this after making proper to-and-from unicode conversion functions
     */
    std::string UNICODE_TEXT;
    int unicodeVals[] = { -2, -1, 0, 72, 0, 101, 0, 108, 0, 108, 0, 111, 0, 32, 0, 49, 0, 50, 0, 51, 0, 52, 0, 32, 101, -27, 103, 44, -118, -98 };
    for (int val : unicodeVals) {
        UNICODE_TEXT += static_cast<char>(val);
    }

    auto doc = PDFDocFactory().createPDFDoc(*filename);

    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    Page *page = doc->getPage(1);

    auto ftextann = std::make_shared<AnnotFreeText>(doc.get(), PDFRectangle(50, 50, 100, 100));
    ftextann->setContents(std::make_unique<GooString>(UNICODE_TEXT));
    setDefaultAppearanceToNative(ftextann.get(), page);

    page->addAnnot(ftextann);

    auto tempFilePath = std::make_unique<GooString>(std::filesystem::current_path() / "temp.pdf");

    doc->saveAs(tempFilePath->toStr());

    // Re-open the file and check if the fonts only contain the needed glyphs
    doc = PDFDocFactory().createPDFDoc(*tempFilePath);

    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    page = doc->getPage(1);

    Annots *annotsPtr = page->getAnnots();
    if (!annotsPtr) {
        return false;
    }

    auto annots = annotsPtr->getAnnots();
    if (annots.size() != 1) {
        return false;
    }

    auto &annot = annots[0];
    if (annot->getType() != Annot::typeFreeText || annot->getContents().toStr() != UNICODE_TEXT) {
        return false;
    }

    return subsettingWorksInAnnot(annot.get(), annot->getContents().toStr(), doc->getXRef(), false);
}

/*
 * Same as testSingleAnnotationSubsetting but with added transparency
 */
static bool testSingleAnnotationSubsettingWithOpacity(std::shared_ptr<GooString> &filename)
{
    std::string UNICODE_TEXT;
    int unicodeVals[] = { -2, -1, 0, 72, 0, 101, 0, 108, 0, 108, 0, 111, 0, 32, 0, 49, 0, 50, 0, 51, 0, 52, 0, 32, 101, -27, 103, 44, -118, -98 };
    for (int val : unicodeVals) {
        UNICODE_TEXT += static_cast<char>(val);
    }

    auto doc = PDFDocFactory().createPDFDoc(*filename);
    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    Page *page = doc->getPage(1);

    auto ftextann = std::make_shared<AnnotFreeText>(doc.get(), PDFRectangle(50, 50, 100, 100));
    ftextann->setContents(std::make_unique<GooString>(UNICODE_TEXT));
    ftextann->setOpacity(0.5);
    setDefaultAppearanceToNative(ftextann.get(), page);

    page->addAnnot(ftextann);

    auto tempFilePath = std::make_unique<GooString>(std::filesystem::current_path() / "temp.pdf");

    doc->saveAs(tempFilePath->toStr());

    // Re-open the file and check if the fonts only contain the needed glyphs
    doc = PDFDocFactory().createPDFDoc(*tempFilePath);
    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    page = doc->getPage(1);

    Annots *annotsPtr = page->getAnnots();
    if (!annotsPtr) {
        return false;
    }

    auto annots = annotsPtr->getAnnots();
    if (annots.size() != 1) {
        return false;
    }

    auto &annot = annots[0];
    if (annot->getType() != Annot::typeFreeText || annot->getContents().toStr() != UNICODE_TEXT) {
        return false;
    }

    return subsettingWorksInAnnot(annot.get(), annot->getContents().toStr(), doc->getXRef(), true);
}

static bool testFormFieldSubsetting(std::shared_ptr<GooString> &filename)
{
    std::string UNICODE_TEXT;
    int unicodeVals[] = { -2, -1, 101, -27, 103, 44, -118, -98 }; // "日本語"
    for (int val : unicodeVals) {
        UNICODE_TEXT += static_cast<char>(val);
    }

    auto doc = PDFDocFactory().createPDFDoc(*filename);
    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    Form *form = doc->getCatalog()->getCreateForm();

    int numFields = form->getNumFields();

    if (numFields != 1) {
        return false;
    }

    FormField *field = form->getRootField(0);
    if (field->getNumChildren() != 0 || field->getType() != FormFieldType::formText) {
        return false;
    }

    auto *textField = dynamic_cast<FormFieldText *>(field);

    textField->setContent(std::make_unique<GooString>(UNICODE_TEXT));

    auto tempFilePath = std::make_unique<GooString>(std::filesystem::current_path() / "temp.pdf");

    doc->saveAs(tempFilePath->toStr());

    doc = PDFDocFactory().createPDFDoc(*tempFilePath);
    if (!doc->isOk() || doc->getNumPages() < 1) {
        return false;
    }

    form = doc->getCatalog()->getCreateForm();

    numFields = form->getNumFields();
    if (numFields != 1) {
        return false;
    }

    field = form->getRootField(0);
    if (field->getNumChildren() != 0 || field->getType() != FormFieldType::formText) {
        return false;
    }

    textField = dynamic_cast<FormFieldText *>(field);
    std::string newText = textField->getContent()->toStr();

    if (newText != UNICODE_TEXT) {
        return false;
    }

    int numWidgets = textField->getNumWidgets();
    if (numWidgets != 1) {
        return false;
    }

    FormWidget *widget = textField->getWidget(0);

    std::shared_ptr<AnnotWidget> wdgAnnot = widget->getWidgetAnnotation();

    return subsettingWorksInAnnot(wdgAnnot.get(), newText, doc->getXRef(), false);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        return 1;
    }

    globalParams = std::make_unique<GlobalParams>();

    auto filename1 = std::make_shared<GooString>(argv[1]);
    auto filename2 = std::make_shared<GooString>(argv[2]);

    auto test1 = testSingleAnnotationSubsetting(filename1);
    auto test2 = testSingleAnnotationSubsettingWithOpacity(filename1);
    auto test3 = testFormFieldSubsetting(filename2);

    // Check and remove the temporary pdf file we save for the tests
    if (std::filesystem::exists(std::filesystem::current_path() / "temp.pdf")) {
        std::filesystem::remove(std::filesystem::current_path() / "temp.pdf");
    }

    if (!(test1 && test2 && test3)) {
        return 1;
    }

    return 0;
}
