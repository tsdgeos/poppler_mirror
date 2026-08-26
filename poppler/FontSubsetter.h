#pragma once

#include "GfxFont.h"
#include "Object.h"
#include <vector>

struct hb_blob_t;

class PDFDoc;

class FontSubsetter
{
public:
    explicit FontSubsetter(PDFDoc *doc);

    void subsetAll() const;

    using FontStringMap = std::unordered_map<std::shared_ptr<const GfxFont>, std::vector<Unicode>>;

    struct GetSubsetFontsResult
    {
        std::vector<std::shared_ptr<const GfxFont>> fontsToRemove;
        std::unordered_map<std::string, Ref> tagRefMappings;
    };
    GetSubsetFontsResult getSubsetFonts(const FontStringMap &fontStringMap) const;

private:
    PDFDoc *doc;

    struct SubsetFontResult
    {
        bool success;
        std::vector<char> data;
    };

    static std::string getTaggedNameForFont(const GfxFont *font);
    Object createFontStreamFromData(std::vector<char> &&data, Ref &ref, const std::string &subtype) const;

    Object createNewSubsetFont(const GfxFont *oldFont, SubsetFontResult &subsettingResult, Ref &newFontRef) const;

    // Harfbuzz related APIs
    static SubsetFontResult hbSubsetFont(std::string &fontStream, const std::vector<Unicode> &unicodeValues);
};
