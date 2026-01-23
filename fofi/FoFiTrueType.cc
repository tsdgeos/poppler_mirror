//========================================================================
//
// FoFiTrueType.cc
//
// Copyright 1999-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2007 Koji Otani <sho@bbr.jp>
// Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2008, 2009, 2012, 2014-2022, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2008 Tomas Are Haavet <tomasare@gmail.com>
// Copyright (C) 2012 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2012, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2014 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2015 Aleksei Volkov <Aleksei Volkov>
// Copyright (C) 2015, 2016 William Bader <williambader@hotmail.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2022 Zachary Travis <ztravis@everlaw.com>
// Copyright (C) 2022, 2024 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdlib>
#include <cstring>
#include <climits>
#include <algorithm>
#include <array>
#include "goo/GooLikely.h"
#include "goo/GooString.h"
#include "goo/GooCheckedOps.h"
#include "FoFiType1C.h"
#include "FoFiTrueType.h"
#include "poppler/Error.h"

//
// Terminology
// -----------
//
// character code = number used as an element of a text string
//
// character name = glyph name = name for a particular glyph within a
//                  font
//
// glyph index = GID = position (within some internal table in the font)
//               where the instructions to draw a particular glyph are
//               stored
//
// Type 1 fonts
// ------------
//
// Type 1 fonts contain:
//
// Encoding: array of glyph names, maps char codes to glyph names
//
//           Encoding[charCode] = charName
//
// CharStrings: dictionary of instructions, keyed by character names,
//              maps character name to glyph data
//
//              CharStrings[charName] = glyphData
//
// TrueType fonts
// --------------
//
// TrueType fonts contain:
//
// 'cmap' table: mapping from character code to glyph index; there may
//               be multiple cmaps in a TrueType font
//
//               cmap[charCode] = gid
//
// 'post' table: mapping from glyph index to glyph name
//
//               post[gid] = glyphName
//
// Type 42 fonts
// -------------
//
// Type 42 fonts contain:
//
// Encoding: array of glyph names, maps char codes to glyph names
//
//           Encoding[charCode] = charName
//
// CharStrings: dictionary of glyph indexes, keyed by character names,
//              maps character name to glyph index
//
//              CharStrings[charName] = gid
//

//------------------------------------------------------------------------

const unsigned int ttcfTag = 0x74746366;

//------------------------------------------------------------------------

struct TrueTypeTable
{
    unsigned int tag = 0;
    unsigned int checksum = 0;
    int offset = 0;
    int origOffset = 0;
    int len = 0;
};

struct TrueTypeCmap
{
    int platform = 0;
    int encoding = 0;
    int offset = 0;
    int len = 0;
    int fmt = 0;
};

struct TrueTypeLoca
{
    int idx = 0;
    int origOffset = 0;
    int newOffset = 0;
    int len = 0;
};

const unsigned int vrt2Tag = 0x76727432;
const unsigned int vertTag = 0x76657274;

struct cmpTrueTypeLocaOffsetFunctor
{
    bool operator()(const TrueTypeLoca loca1, const TrueTypeLoca loca2)
    {
        if (loca1.origOffset == loca2.origOffset) {
            return loca1.idx < loca2.idx;
        }
        return loca1.origOffset < loca2.origOffset;
    }
};

struct cmpTrueTypeLocaIdxFunctor
{
    bool operator()(const TrueTypeLoca loca1, const TrueTypeLoca loca2) { return loca1.idx < loca2.idx; }
};

struct cmpTrueTypeTableTagFunctor
{
    bool operator()(const TrueTypeTable &tab1, const TrueTypeTable &tab2) { return tab1.tag < tab2.tag; }
};

//------------------------------------------------------------------------

struct T42Table
{
    const char *tag; // 4-byte tag
    bool required; // required by the TrueType spec?
};

// TrueType tables to be embedded in Type 42 fonts.
// NB: the table names must be in alphabetical order here.
static const T42Table t42Tables[] = { { .tag = "cvt ", .required = true }, { .tag = "fpgm", .required = true },  { .tag = "glyf", .required = true }, { .tag = "head", .required = true },
                                      { .tag = "hhea", .required = true }, { .tag = "hmtx", .required = true },  { .tag = "loca", .required = true }, { .tag = "maxp", .required = true },
                                      { .tag = "prep", .required = true }, { .tag = "vhea", .required = false }, { .tag = "vmtx", .required = false } };
constexpr int nT42Tables = sizeof(t42Tables) / sizeof(T42Table);

constexpr int t42HeadTable = 3;
constexpr int t42LocaTable = 6;
constexpr int t42GlyfTable = 2;
constexpr int t42VheaTable = 9;
constexpr int t42VmtxTable = 10;

//------------------------------------------------------------------------

// Glyph names in some arbitrary standard order that Apple uses for
// their TrueType fonts.
static const char *macGlyphNames[258] = { ".notdef",
                                          "null",
                                          "CR",
                                          "space",
                                          "exclam",
                                          "quotedbl",
                                          "numbersign",
                                          "dollar",
                                          "percent",
                                          "ampersand",
                                          "quotesingle",
                                          "parenleft",
                                          "parenright",
                                          "asterisk",
                                          "plus",
                                          "comma",
                                          "hyphen",
                                          "period",
                                          "slash",
                                          "zero",
                                          "one",
                                          "two",
                                          "three",
                                          "four",
                                          "five",
                                          "six",
                                          "seven",
                                          "eight",
                                          "nine",
                                          "colon",
                                          "semicolon",
                                          "less",
                                          "equal",
                                          "greater",
                                          "question",
                                          "at",
                                          "A",
                                          "B",
                                          "C",
                                          "D",
                                          "E",
                                          "F",
                                          "G",
                                          "H",
                                          "I",
                                          "J",
                                          "K",
                                          "L",
                                          "M",
                                          "N",
                                          "O",
                                          "P",
                                          "Q",
                                          "R",
                                          "S",
                                          "T",
                                          "U",
                                          "V",
                                          "W",
                                          "X",
                                          "Y",
                                          "Z",
                                          "bracketleft",
                                          "backslash",
                                          "bracketright",
                                          "asciicircum",
                                          "underscore",
                                          "grave",
                                          "a",
                                          "b",
                                          "c",
                                          "d",
                                          "e",
                                          "f",
                                          "g",
                                          "h",
                                          "i",
                                          "j",
                                          "k",
                                          "l",
                                          "m",
                                          "n",
                                          "o",
                                          "p",
                                          "q",
                                          "r",
                                          "s",
                                          "t",
                                          "u",
                                          "v",
                                          "w",
                                          "x",
                                          "y",
                                          "z",
                                          "braceleft",
                                          "bar",
                                          "braceright",
                                          "asciitilde",
                                          "Adieresis",
                                          "Aring",
                                          "Ccedilla",
                                          "Eacute",
                                          "Ntilde",
                                          "Odieresis",
                                          "Udieresis",
                                          "aacute",
                                          "agrave",
                                          "acircumflex",
                                          "adieresis",
                                          "atilde",
                                          "aring",
                                          "ccedilla",
                                          "eacute",
                                          "egrave",
                                          "ecircumflex",
                                          "edieresis",
                                          "iacute",
                                          "igrave",
                                          "icircumflex",
                                          "idieresis",
                                          "ntilde",
                                          "oacute",
                                          "ograve",
                                          "ocircumflex",
                                          "odieresis",
                                          "otilde",
                                          "uacute",
                                          "ugrave",
                                          "ucircumflex",
                                          "udieresis",
                                          "dagger",
                                          "degree",
                                          "cent",
                                          "sterling",
                                          "section",
                                          "bullet",
                                          "paragraph",
                                          "germandbls",
                                          "registered",
                                          "copyright",
                                          "trademark",
                                          "acute",
                                          "dieresis",
                                          "notequal",
                                          "AE",
                                          "Oslash",
                                          "infinity",
                                          "plusminus",
                                          "lessequal",
                                          "greaterequal",
                                          "yen",
                                          "mu",
                                          "partialdiff",
                                          "summation",
                                          "product",
                                          "pi",
                                          "integral",
                                          "ordfeminine",
                                          "ordmasculine",
                                          "Omega",
                                          "ae",
                                          "oslash",
                                          "questiondown",
                                          "exclamdown",
                                          "logicalnot",
                                          "radical",
                                          "florin",
                                          "approxequal",
                                          "increment",
                                          "guillemotleft",
                                          "guillemotright",
                                          "ellipsis",
                                          "nbspace",
                                          "Agrave",
                                          "Atilde",
                                          "Otilde",
                                          "OE",
                                          "oe",
                                          "endash",
                                          "emdash",
                                          "quotedblleft",
                                          "quotedblright",
                                          "quoteleft",
                                          "quoteright",
                                          "divide",
                                          "lozenge",
                                          "ydieresis",
                                          "Ydieresis",
                                          "fraction",
                                          "currency",
                                          "guilsinglleft",
                                          "guilsinglright",
                                          "fi",
                                          "fl",
                                          "daggerdbl",
                                          "periodcentered",
                                          "quotesinglbase",
                                          "quotedblbase",
                                          "perthousand",
                                          "Acircumflex",
                                          "Ecircumflex",
                                          "Aacute",
                                          "Edieresis",
                                          "Egrave",
                                          "Iacute",
                                          "Icircumflex",
                                          "Idieresis",
                                          "Igrave",
                                          "Oacute",
                                          "Ocircumflex",
                                          "applelogo",
                                          "Ograve",
                                          "Uacute",
                                          "Ucircumflex",
                                          "Ugrave",
                                          "dotlessi",
                                          "circumflex",
                                          "tilde",
                                          "overscore",
                                          "breve",
                                          "dotaccent",
                                          "ring",
                                          "cedilla",
                                          "hungarumlaut",
                                          "ogonek",
                                          "caron",
                                          "Lslash",
                                          "lslash",
                                          "Scaron",
                                          "scaron",
                                          "Zcaron",
                                          "zcaron",
                                          "brokenbar",
                                          "Eth",
                                          "eth",
                                          "Yacute",
                                          "yacute",
                                          "Thorn",
                                          "thorn",
                                          "minus",
                                          "multiply",
                                          "onesuperior",
                                          "twosuperior",
                                          "threesuperior",
                                          "onehalf",
                                          "onequarter",
                                          "threequarters",
                                          "franc",
                                          "Gbreve",
                                          "gbreve",
                                          "Idot",
                                          "Scedilla",
                                          "scedilla",
                                          "Cacute",
                                          "cacute",
                                          "Ccaron",
                                          "ccaron",
                                          "dmacron" };

//------------------------------------------------------------------------
// FoFiTrueType
//------------------------------------------------------------------------

std::unique_ptr<FoFiTrueType> FoFiTrueType::make(std::span<const unsigned char> data, int faceIndexA)
{
    auto ff = std::make_unique<FoFiTrueType>(data, faceIndexA);
    if (!ff->parsedOk) {
        return nullptr;
    }
    return ff;
}

std::unique_ptr<FoFiTrueType> FoFiTrueType::load(const char *fileName, int faceIndexA)
{
    std::optional<std::vector<unsigned char>> fileA;

    if (!(fileA = FoFiBase::readFile(fileName))) {
        return nullptr;
    }
    auto ff = std::make_unique<FoFiTrueType>(std::move(fileA.value()), faceIndexA);
    if (!ff->parsedOk) {
        return nullptr;
    }
    return ff;
}

FoFiTrueType::FoFiTrueType(std::vector<unsigned char> &&fileA, int faceIndexA, PrivateTag /*unused*/) : FoFiBase(std::move(fileA))
{
    parsedOk = false;
    faceIndex = faceIndexA;
    gsubFeatureTable = 0;
    gsubLookupList = 0;

    parse();
}

FoFiTrueType::FoFiTrueType(std::span<const unsigned char> data, int faceIndexA, PrivateTag /*unused*/) : FoFiBase(data)
{
    parsedOk = false;
    faceIndex = faceIndexA;
    gsubFeatureTable = 0;
    gsubLookupList = 0;

    parse();
}

FoFiTrueType::~FoFiTrueType() = default;

int FoFiTrueType::getNumCmaps() const
{
    return cmaps.size();
}

int FoFiTrueType::getCmapPlatform(int i) const
{
    return cmaps[i].platform;
}

int FoFiTrueType::getCmapEncoding(int i) const
{
    return cmaps[i].encoding;
}

int FoFiTrueType::findCmap(int platform, int encoding) const
{
    const int nCmaps = cmaps.size();
    for (int i = 0; i < nCmaps; ++i) {
        if (cmaps[i].platform == platform && cmaps[i].encoding == encoding) {
            return i;
        }
    }
    return -1;
}

int FoFiTrueType::mapCodeToGID(int i, unsigned int c) const
{
    int gid;
    unsigned int segCnt, segEnd, segStart, segDelta, segOffset;
    unsigned int cmapFirst, cmapLen;
    int pos, a, b, m;
    unsigned int high, low, idx;
    bool ok;

    if (i < 0 || static_cast<size_t>(i) >= cmaps.size()) {
        return 0;
    }
    ok = true;
    pos = cmaps[i].offset;
    switch (cmaps[i].fmt) {
    case 0:
        if (c + 6 >= (unsigned int)cmaps[i].len) {
            return 0;
        }
        gid = getU8(cmaps[i].offset + 6 + c, &ok);
        break;
    case 2:
        high = c >> 8;
        low = c & 0xFFU;
        idx = getU16BE(pos + 6 + high * 2, &ok);
        segStart = getU16BE(pos + 6 + 512 + idx, &ok);
        segCnt = getU16BE(pos + 6 + 512 + idx + 2, &ok);
        segDelta = getS16BE(pos + 6 + 512 + idx + 4, &ok);
        segOffset = getU16BE(pos + 6 + 512 + idx + 6, &ok);
        if (low < segStart || low >= segStart + segCnt) {
            gid = 0;
        } else {
            int val = getU16BE(pos + 6 + 512 + idx + 6 + segOffset + (low - segStart) * 2, &ok);
            gid = val == 0 ? 0 : (val + segDelta) & 0xFFFFU;
        }
        break;
    case 4:
        segCnt = getU16BE(pos + 6, &ok) / 2;
        a = -1;
        b = segCnt - 1;
        segEnd = getU16BE(pos + 14 + 2 * b, &ok);
        if (c > segEnd) {
            // malformed font -- the TrueType spec requires the last segEnd
            // to be 0xffff
            return 0;
        }
        // invariant: seg[a].end < code <= seg[b].end
        while (b - a > 1 && ok) {
            m = (a + b) / 2;
            segEnd = getU16BE(pos + 14 + 2 * m, &ok);
            if (segEnd < c) {
                a = m;
            } else {
                b = m;
            }
        }
        segStart = getU16BE(pos + 16 + 2 * segCnt + 2 * b, &ok);
        segDelta = getU16BE(pos + 16 + 4 * segCnt + 2 * b, &ok);
        segOffset = getU16BE(pos + 16 + 6 * segCnt + 2 * b, &ok);
        if (c < segStart) {
            return 0;
        }
        if (segOffset == 0) {
            gid = (c + segDelta) & 0xffff;
        } else {
            gid = getU16BE(pos + 16 + 6 * segCnt + 2 * b + segOffset + 2 * (c - segStart), &ok);
            if (gid != 0) {
                gid = (gid + segDelta) & 0xffff;
            }
        }
        break;
    case 6:
        cmapFirst = getU16BE(pos + 6, &ok);
        cmapLen = getU16BE(pos + 8, &ok);
        if (c < cmapFirst || c >= cmapFirst + cmapLen) {
            return 0;
        }
        gid = getU16BE(pos + 10 + 2 * (c - cmapFirst), &ok);
        break;
    case 12:
    case 13:
        segCnt = getU32BE(pos + 12, &ok);
        a = -1;
        b = segCnt - 1;
        if (b > std::numeric_limits<int>::max() / 12) {
            return 0;
        }
        segEnd = getU32BE(pos + 16 + 12 * b + 4, &ok);
        if (c > segEnd) {
            return 0;
        }
        // invariant: seg[a].end < code <= seg[b].end
        while (b - a > 1 && ok) {
            m = (a + b) / 2;
            segEnd = getU32BE(pos + 16 + 12 * m + 4, &ok);
            if (segEnd < c) {
                a = m;
            } else {
                b = m;
            }
        }
        segStart = getU32BE(pos + 16 + 12 * b, &ok);
        segDelta = getU32BE(pos + 16 + 12 * b + 8, &ok);
        if (c < segStart) {
            return 0;
        }
        // In format 12, the glyph codes increment through
        // each segment; in format 13 the same glyph code is used
        // for an entire segment.
        gid = segDelta + (cmaps[i].fmt == 12 ? (c - segStart) : 0);
        break;
    default:
        return 0;
    }
    if (!ok) {
        return 0;
    }
    return gid;
}

int FoFiTrueType::mapNameToGID(const char *name) const
{
    const auto gid = nameToGID.find(name);
    if (gid == nameToGID.end()) {
        return 0;
    }
    return gid->second;
}

std::optional<std::span<const unsigned char>> FoFiTrueType::getCFFBlock() const
{
    if (!openTypeCFF || tables.empty()) {
        return std::nullopt;
    }
    int i = seekTable("CFF ");
    if (i < 0 || !checkRegion(tables[i].offset, tables[i].len)) {
        return std::nullopt;
    }
    return std::span { file.data() + tables[i].offset, size_t(tables[i].len) };
}

std::vector<int> FoFiTrueType::getCIDToGIDMap() const
{
    auto cffBlock = getCFFBlock();
    if (!cffBlock) {
        return {};
    }
    auto ff = FoFiType1C::make(cffBlock.value());
    if (!ff) {
        return {};
    }
    std::vector<int> map = ff->getCIDToGIDMap();
    return map;
}

int FoFiTrueType::getEmbeddingRights() const
{
    int i, fsType;
    bool ok;

    if ((i = seekTable("OS/2")) < 0) {
        return 4;
    }
    ok = true;
    fsType = getU16BE(tables[i].offset + 8, &ok);
    if (!ok) {
        return 4;
    }
    if (fsType & 0x0008) {
        return 2;
    }
    if (fsType & 0x0004) {
        return 1;
    }
    if (fsType & 0x0002) {
        return 0;
    }
    return 3;
}

void FoFiTrueType::convertToType42(const char *psName, char **encoding, const std::vector<int> &codeToGID, FoFiOutputFunc outputFunc, void *outputStream) const
{
    int maxUsedGlyph;
    bool ok;

    if (openTypeCFF) {
        return;
    }

    // write the header
    ok = true;
    std::string buf = GooString::format("%!PS-TrueTypeFont-{0:2g}\n", (double)getS32BE(0, &ok) / 65536.0);
    (*outputFunc)(outputStream, buf.c_str(), buf.size());

    // begin the font dictionary
    (*outputFunc)(outputStream, "10 dict begin\n", 14);
    (*outputFunc)(outputStream, "/FontName /", 11);
    (*outputFunc)(outputStream, psName, strlen(psName));
    (*outputFunc)(outputStream, " def\n", 5);
    (*outputFunc)(outputStream, "/FontType 42 def\n", 17);
    (*outputFunc)(outputStream, "/FontMatrix [1 0 0 1 0 0] def\n", 30);
    buf = GooString::format("/FontBBox [{0:d} {1:d} {2:d} {3:d}] def\n", bbox[0], bbox[1], bbox[2], bbox[3]);
    (*outputFunc)(outputStream, buf.c_str(), buf.size());
    (*outputFunc)(outputStream, "/PaintType 0 def\n", 17);

    // write the guts of the dictionary
    cvtEncoding(encoding, outputFunc, outputStream);
    cvtCharStrings(encoding, codeToGID, outputFunc, outputStream);
    cvtSfnts(outputFunc, outputStream, std::nullopt, false, &maxUsedGlyph);

    // end the dictionary and define the font
    (*outputFunc)(outputStream, "FontName currentdict end definefont pop\n", 40);
}

void FoFiTrueType::convertToType1(const char *psName, const char **newEncoding, bool ascii, FoFiOutputFunc outputFunc, void *outputStream) const
{
    auto cffBlock = getCFFBlock();
    if (!cffBlock) {
        return;
    }
    auto ff = FoFiType1C::make(cffBlock.value());
    if (!ff) {
        return;
    }
    ff->convertToType1(psName, newEncoding, ascii, outputFunc, outputStream);
}

void FoFiTrueType::convertToCIDType2(const char *psName, const std::vector<int> &cidMap, bool needVerticalMetrics, FoFiOutputFunc outputFunc, void *outputStream) const
{
    bool ok;

    if (openTypeCFF) {
        return;
    }

    // write the header
    ok = true;
    std::string buf = GooString::format("%!PS-TrueTypeFont-{0:2g}\n", (double)getS32BE(0, &ok) / 65536.0);
    (*outputFunc)(outputStream, buf.c_str(), buf.size());

    // begin the font dictionary
    (*outputFunc)(outputStream, "20 dict begin\n", 14);
    (*outputFunc)(outputStream, "/CIDFontName /", 14);
    (*outputFunc)(outputStream, psName, strlen(psName));
    (*outputFunc)(outputStream, " def\n", 5);
    (*outputFunc)(outputStream, "/CIDFontType 2 def\n", 19);
    (*outputFunc)(outputStream, "/FontType 42 def\n", 17);
    (*outputFunc)(outputStream, "/CIDSystemInfo 3 dict dup begin\n", 32);
    (*outputFunc)(outputStream, "  /Registry (Adobe) def\n", 24);
    (*outputFunc)(outputStream, "  /Ordering (Identity) def\n", 27);
    (*outputFunc)(outputStream, "  /Supplement 0 def\n", 20);
    (*outputFunc)(outputStream, "  end def\n", 10);
    (*outputFunc)(outputStream, "/GDBytes 2 def\n", 15);
    if (!cidMap.empty()) {
        buf = GooString::format("/CIDCount {0:d} def\n", int(cidMap.size()));
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
        if (cidMap.size() > 32767) {
            (*outputFunc)(outputStream, "/CIDMap [", 9);
            for (size_t i = 0; i < cidMap.size(); i += 32768 - 16) {
                (*outputFunc)(outputStream, "<\n", 2);
                for (size_t j = 0; j < 32768 - 16 && i + j < cidMap.size(); j += 16) {
                    (*outputFunc)(outputStream, "  ", 2);
                    for (size_t k = 0; k < 16 && i + j + k < cidMap.size(); ++k) {
                        const int cid = cidMap[i + j + k];
                        buf = GooString::format("{0:02x}{1:02x}", (cid >> 8) & 0xff, cid & 0xff);
                        (*outputFunc)(outputStream, buf.c_str(), buf.size());
                    }
                    (*outputFunc)(outputStream, "\n", 1);
                }
                (*outputFunc)(outputStream, "  >", 3);
            }
            (*outputFunc)(outputStream, "\n", 1);
            (*outputFunc)(outputStream, "] def\n", 6);
        } else {
            (*outputFunc)(outputStream, "/CIDMap <\n", 10);
            for (size_t i = 0; i < cidMap.size(); i += 16) {
                (*outputFunc)(outputStream, "  ", 2);
                for (size_t j = 0; j < 16 && i + j < cidMap.size(); ++j) {
                    const int cid = cidMap[i + j];
                    buf = GooString::format("{0:02x}{1:02x}", (cid >> 8) & 0xff, cid & 0xff);
                    (*outputFunc)(outputStream, buf.c_str(), buf.size());
                }
                (*outputFunc)(outputStream, "\n", 1);
            }
            (*outputFunc)(outputStream, "> def\n", 6);
        }
    } else {
        // direct mapping - just fill the string(s) with s[i]=i
        buf = GooString::format("/CIDCount {0:d} def\n", nGlyphs);
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
        if (nGlyphs > 32767) {
            (*outputFunc)(outputStream, "/CIDMap [\n", 10);
            for (int i = 0; i < nGlyphs; i += 32767) {
                const int j = nGlyphs - i < 32767 ? nGlyphs - i : 32767;
                buf = GooString::format("  {0:d} string 0 1 {1:d} {{\n", 2 * j, j - 1);
                (*outputFunc)(outputStream, buf.c_str(), buf.size());
                buf = GooString::format("    2 copy dup 2 mul exch {0:d} add -8 bitshift put\n", i);
                (*outputFunc)(outputStream, buf.c_str(), buf.size());
                buf = GooString::format("    1 index exch dup 2 mul 1 add exch {0:d} add"
                                        " 255 and put\n",
                                        i);
                (*outputFunc)(outputStream, buf.c_str(), buf.size());
                (*outputFunc)(outputStream, "  } for\n", 8);
            }
            (*outputFunc)(outputStream, "] def\n", 6);
        } else {
            buf = GooString::format("/CIDMap {0:d} string\n", 2 * nGlyphs);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
            buf = GooString::format("  0 1 {0:d} {{\n", nGlyphs - 1);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
            (*outputFunc)(outputStream, "    2 copy dup 2 mul exch -8 bitshift put\n", 42);
            (*outputFunc)(outputStream, "    1 index exch dup 2 mul 1 add exch 255 and put\n", 50);
            (*outputFunc)(outputStream, "  } for\n", 8);
            (*outputFunc)(outputStream, "def\n", 4);
        }
    }
    (*outputFunc)(outputStream, "/FontMatrix [1 0 0 1 0 0] def\n", 30);
    buf = GooString::format("/FontBBox [{0:d} {1:d} {2:d} {3:d}] def\n", bbox[0], bbox[1], bbox[2], bbox[3]);
    (*outputFunc)(outputStream, buf.c_str(), buf.size());
    (*outputFunc)(outputStream, "/PaintType 0 def\n", 17);
    (*outputFunc)(outputStream, "/Encoding [] readonly def\n", 26);
    (*outputFunc)(outputStream, "/CharStrings 1 dict dup begin\n", 30);
    (*outputFunc)(outputStream, "  /.notdef 0 def\n", 17);
    (*outputFunc)(outputStream, "  end readonly def\n", 19);

    // write the guts of the dictionary
    int unusedMaxUsedGlyph;
    cvtSfnts(outputFunc, outputStream, std::nullopt, needVerticalMetrics, &unusedMaxUsedGlyph);

    // end the dictionary and define the font
    (*outputFunc)(outputStream, "CIDFontName currentdict end /CIDFont defineresource pop\n", 56);
}

void FoFiTrueType::convertToCIDType0(const std::string &psName, const std::vector<int> &cidMap, FoFiOutputFunc outputFunc, void *outputStream) const
{
    auto cffBlock = getCFFBlock();
    if (!cffBlock) {
        return;
    }
    auto ff = FoFiType1C::make(cffBlock.value());
    if (!ff) {
        return;
    }
    ff->convertToCIDType0(psName, cidMap, outputFunc, outputStream);
}

void FoFiTrueType::convertToType0(const std::string &psName, const std::vector<int> &cidMap, bool needVerticalMetrics, int *maxValidGlyph, FoFiOutputFunc outputFunc, void *outputStream) const
{
    int maxUsedGlyph, n, i, j;

    *maxValidGlyph = -1;

    if (openTypeCFF) {
        return;
    }

    // write the Type 42 sfnts array
    const std::string sfntsName = psName + "_sfnts";
    cvtSfnts(outputFunc, outputStream, sfntsName, needVerticalMetrics, &maxUsedGlyph);

    // write the descendant Type 42 fonts
    // (The following is a kludge: nGlyphs is the glyph count from the
    // maxp table; maxUsedGlyph is the max glyph number that has a
    // non-zero-length description, from the loca table.  The problem is
    // that some TrueType font subsets fail to change the glyph count,
    // i.e., nGlyphs is much larger than maxUsedGlyph+1, which results
    // in an unnecessarily huge Type 0 font.  But some other PDF files
    // have fonts with only zero or one used glyph, and a content stream
    // that refers to one of the unused glyphs -- this results in PS
    // errors if we simply use maxUsedGlyph+1 for the Type 0 font.  So
    // we compromise by always defining at least 256 glyphs.)
    // Some fonts have a large nGlyphs but maxUsedGlyph of 0.
    // These fonts might reference any glyph.
    // Return the last written glyph number in maxValidGlyph.
    // PSOutputDev::drawString() can use maxValidGlyph to avoid
    // referencing zero-length glyphs that we trimmed.
    // This allows pdftops to avoid writing huge files while still
    // handling the rare PDF that uses a zero-length glyph.
    if (!cidMap.empty()) {
        n = cidMap.size();
    } else if (nGlyphs > maxUsedGlyph + 256) {
        if (maxUsedGlyph <= 255) {
            n = 256;
        } else {
            n = maxUsedGlyph + 1;
        }
    } else {
        n = nGlyphs;
    }
    *maxValidGlyph = n - 1;
    for (i = 0; i < n; i += 256) {
        (*outputFunc)(outputStream, "10 dict begin\n", 14);
        (*outputFunc)(outputStream, "/FontName /", 11);
        (*outputFunc)(outputStream, psName.c_str(), psName.length());
        std::string buf = GooString::format("_{0:02x} def\n", i >> 8);
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
        (*outputFunc)(outputStream, "/FontType 42 def\n", 17);
        (*outputFunc)(outputStream, "/FontMatrix [1 0 0 1 0 0] def\n", 30);
        buf = GooString::format("/FontBBox [{0:d} {1:d} {2:d} {3:d}] def\n", bbox[0], bbox[1], bbox[2], bbox[3]);
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
        (*outputFunc)(outputStream, "/PaintType 0 def\n", 17);
        (*outputFunc)(outputStream, "/sfnts ", 7);
        (*outputFunc)(outputStream, psName.c_str(), psName.length());
        (*outputFunc)(outputStream, "_sfnts def\n", 11);
        (*outputFunc)(outputStream, "/Encoding 256 array\n", 20);
        for (j = 0; j < 256 && i + j < n; ++j) {
            buf = GooString::format("dup {0:d} /c{1:02x} put\n", j, j);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
        }
        (*outputFunc)(outputStream, "readonly def\n", 13);
        (*outputFunc)(outputStream, "/CharStrings 257 dict dup begin\n", 32);
        (*outputFunc)(outputStream, "/.notdef 0 def\n", 15);
        for (j = 0; j < 256 && i + j < n; ++j) {
            buf = GooString::format("/c{0:02x} {1:d} def\n", j, !cidMap.empty() ? cidMap[i + j] : i + j);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
        }
        (*outputFunc)(outputStream, "end readonly def\n", 17);
        (*outputFunc)(outputStream, "FontName currentdict end definefont pop\n", 40);
    }

    // write the Type 0 parent font
    (*outputFunc)(outputStream, "16 dict begin\n", 14);
    (*outputFunc)(outputStream, "/FontName /", 11);
    (*outputFunc)(outputStream, psName.c_str(), psName.length());
    (*outputFunc)(outputStream, " def\n", 5);
    (*outputFunc)(outputStream, "/FontType 0 def\n", 16);
    (*outputFunc)(outputStream, "/FontMatrix [1 0 0 1 0 0] def\n", 30);
    (*outputFunc)(outputStream, "/FMapType 2 def\n", 16);
    (*outputFunc)(outputStream, "/Encoding [\n", 12);
    for (i = 0; i < n; i += 256) {
        const std::string buf = GooString::format("{0:d}\n", i >> 8);
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
    }
    (*outputFunc)(outputStream, "] def\n", 6);
    (*outputFunc)(outputStream, "/FDepVector [\n", 14);
    for (i = 0; i < n; i += 256) {
        (*outputFunc)(outputStream, "/", 1);
        (*outputFunc)(outputStream, psName.c_str(), psName.length());
        const std::string buf = GooString::format("_{0:02x} findfont\n", i >> 8);
        (*outputFunc)(outputStream, buf.c_str(), buf.size());
    }
    (*outputFunc)(outputStream, "] def\n", 6);
    (*outputFunc)(outputStream, "FontName currentdict end definefont pop\n", 40);
}

void FoFiTrueType::convertToType0(const std::string &psName, const std::vector<int> &cidMap, FoFiOutputFunc outputFunc, void *outputStream) const
{
    auto cffBlock = getCFFBlock();
    if (!cffBlock) {
        return;
    }
    auto ff = FoFiType1C::make(cffBlock.value());
    if (!ff) {
        return;
    }
    ff->convertToType0(psName, cidMap, outputFunc, outputStream);
}

void FoFiTrueType::cvtEncoding(char **encoding, FoFiOutputFunc outputFunc, void *outputStream)
{
    const char *name;
    int i;

    (*outputFunc)(outputStream, "/Encoding 256 array\n", 20);
    if (encoding) {
        for (i = 0; i < 256; ++i) {
            if (!(name = encoding[i])) {
                name = ".notdef";
            }
            const std::string buf = GooString::format("dup {0:d} /", i);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
            (*outputFunc)(outputStream, name, strlen(name));
            (*outputFunc)(outputStream, " put\n", 5);
        }
    } else {
        for (i = 0; i < 256; ++i) {
            const std::string buf = GooString::format("dup {0:d} /c{1:02x} put\n", i, i);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
        }
    }
    (*outputFunc)(outputStream, "readonly def\n", 13);
}

void FoFiTrueType::cvtCharStrings(char **encoding, const std::vector<int> &codeToGID, FoFiOutputFunc outputFunc, void *outputStream) const
{
    const char *name;
    char buf2[16];
    int i, k;

    // always define '.notdef'
    (*outputFunc)(outputStream, "/CharStrings 256 dict dup begin\n", 32);
    (*outputFunc)(outputStream, "/.notdef 0 def\n", 15);

    // if there's no 'cmap' table, punt
    if (cmaps.empty()) {
        goto err;
    }

    // map char name to glyph index:
    // 1. use encoding to map name to char code
    // 2. use codeToGID to map char code to glyph index
    // N.B. We do this in reverse order because font subsets can have
    //      weird encodings that use the same character name twice, and
    //      the first definition is probably the one we want.
    k = 0; // make gcc happy
    for (i = 255; i >= 0; --i) {
        if (encoding) {
            name = encoding[i];
        } else {
            sprintf(buf2, "c%02x", i);
            name = buf2;
        }
        if (name && (strcmp(name, ".notdef") != 0)) {
            k = codeToGID[i];
            // note: Distiller (maybe Adobe's PS interpreter in general)
            // doesn't like TrueType fonts that have CharStrings entries
            // which point to nonexistent glyphs, hence the (k < nGlyphs)
            // test
            if (k > 0 && k < nGlyphs) {
                (*outputFunc)(outputStream, "/", 1);
                (*outputFunc)(outputStream, name, strlen(name));
                const std::string buf = GooString::format(" {0:d} def\n", k);
                (*outputFunc)(outputStream, buf.c_str(), buf.size());
            }
        }
    }

err:
    (*outputFunc)(outputStream, "end readonly def\n", 17);
}

void FoFiTrueType::cvtSfnts(FoFiOutputFunc outputFunc, void *outputStream, const std::optional<std::string> &name, bool needVerticalMetrics, int *maxUsedGlyph) const
{
    std::array<unsigned char, 54> headData;
    std::vector<unsigned char> locaData;
    TrueTypeTable newTables[nT42Tables];
    unsigned char tableDir[12 + nT42Tables * 16];
    bool ok;
    unsigned int checksum;
    int nNewTables;
    int glyfTableLen, length, glyfPos, j, k;
    std::array<unsigned char, 36> vheaTab = {
        0, 1, 0, 0, // table version number
        0, 0, // ascent
        0, 0, // descent
        0, 0, // reserved
        0, 0, // max advance height
        0, 0, // min top side bearing
        0, 0, // min bottom side bearing
        0, 0, // y max extent
        0, 0, // caret slope rise
        0, 1, // caret slope run
        0, 0, // caret offset
        0, 0, // reserved
        0, 0, // reserved
        0, 0, // reserved
        0, 0, // reserved
        0, 0, // metric data format
        0, 1 // number of advance heights in vmtx table
    };
    std::vector<unsigned char> vmtxTab;
    bool needVhea, needVmtx;
    int advance;

    *maxUsedGlyph = -1;

    // construct the 'head' table, zero out the font checksum
    int i = seekTable("head");
    if (i < 0 || static_cast<size_t>(i) >= tables.size()) {
        return;
    }
    int pos = tables[i].offset;
    if (!checkRegion(pos, 54)) {
        return;
    }
    memcpy(headData.data(), file.data() + pos, 54);
    headData[8] = headData[9] = headData[10] = headData[11] = (unsigned char)0;

    // check for a bogus loca format field in the 'head' table
    // (I've encountered fonts with loca format set to 0x0100 instead of 0x0001)
    if (locaFmt != 0 && locaFmt != 1) {
        headData[50] = 0;
        headData[51] = 1;
    }

    // read the original 'loca' table, pad entries out to 4 bytes, and
    // sort it into proper order -- some (non-compliant) fonts have
    // out-of-order loca tables; in order to correctly handle the case
    // where (compliant) fonts have empty entries in the middle of the
    // table, cmpTrueTypeLocaOffset uses offset as its primary sort key,
    // and idx as its secondary key (ensuring that adjacent entries with
    // the same pos value remain in the same order)
    std::vector<TrueTypeLoca> locaTable { size_t(nGlyphs + 1) };
    i = seekTable("loca");
    pos = tables[i].offset;
    i = seekTable("glyf");
    glyfTableLen = tables[i].len;
    ok = true;
    for (i = 0; i <= nGlyphs; ++i) {
        locaTable[i].idx = i;
        if (locaFmt) {
            locaTable[i].origOffset = (int)getU32BE(pos + i * 4, &ok);
        } else {
            locaTable[i].origOffset = 2 * getU16BE(pos + i * 2, &ok);
        }
        if (locaTable[i].origOffset > glyfTableLen) {
            locaTable[i].origOffset = glyfTableLen;
        }
    }
    std::ranges::sort(locaTable, cmpTrueTypeLocaOffsetFunctor());
    for (i = 0; i < nGlyphs; ++i) {
        locaTable[i].len = locaTable[i + 1].origOffset - locaTable[i].origOffset;
    }
    locaTable[nGlyphs].len = 0;
    std::ranges::sort(locaTable, cmpTrueTypeLocaIdxFunctor());
    pos = 0;
    for (i = 0; i <= nGlyphs; ++i) {
        locaTable[i].newOffset = pos;

        int newPos;
        if (unlikely(checkedAdd(pos, locaTable[i].len, &newPos))) {
            ok = false;
        } else {
            pos = newPos;
            if (pos & 3) {
                pos += 4 - (pos & 3);
            }
        }
        if (locaTable[i].len > 0) {
            *maxUsedGlyph = i;
        }
    }

    // construct the new 'loca' table
    locaData.resize((nGlyphs + 1) * (locaFmt ? 4 : 2));
    for (i = 0; i <= nGlyphs; ++i) {
        pos = locaTable[i].newOffset;
        if (locaFmt) {
            locaData[4 * i] = (unsigned char)(pos >> 24);
            locaData[4 * i + 1] = (unsigned char)(pos >> 16);
            locaData[4 * i + 2] = (unsigned char)(pos >> 8);
            locaData[4 * i + 3] = (unsigned char)pos;
        } else {
            locaData[2 * i] = (unsigned char)(pos >> 9);
            locaData[2 * i + 1] = (unsigned char)(pos >> 1);
        }
    }

    // count the number of tables
    nNewTables = 0;
    for (i = 0; i < nT42Tables; ++i) {
        if (t42Tables[i].required || seekTable(t42Tables[i].tag) >= 0) {
            ++nNewTables;
        }
    }
    advance = 0; // make gcc happy
    if (needVerticalMetrics) {
        needVhea = seekTable("vhea") < 0;
        needVmtx = seekTable("vmtx") < 0;
        if (needVhea || needVmtx) {
            i = seekTable("head");
            advance = getU16BE(tables[i].offset + 18, &ok); // units per em
            if (needVhea) {
                ++nNewTables;
            }
            if (needVmtx) {
                ++nNewTables;
            }
        }
    }

    // construct the new table headers, including table checksums
    // (pad each table out to a multiple of 4 bytes)
    pos = 12 + nNewTables * 16;
    k = 0;
    for (i = 0; i < nT42Tables; ++i) {
        length = -1;
        checksum = 0; // make gcc happy
        if (i == t42HeadTable) {
            length = 54;
            checksum = computeTableChecksum(headData);
        } else if (i == t42LocaTable) {
            length = (nGlyphs + 1) * (locaFmt ? 4 : 2);
            checksum = computeTableChecksum(locaData);
        } else if (i == t42GlyfTable) {
            length = 0;
            checksum = 0;
            glyfPos = tables[seekTable("glyf")].offset;
            for (j = 0; j < nGlyphs; ++j) {
                length += locaTable[j].len;
                if (length & 3) {
                    length += 4 - (length & 3);
                }
                if (checkRegion(glyfPos + locaTable[j].origOffset, locaTable[j].len)) {
                    checksum += computeTableChecksum(std::span(file.data() + glyfPos + locaTable[j].origOffset, locaTable[j].len));
                }
            }
        } else {
            if ((j = seekTable(t42Tables[i].tag)) >= 0) {
                length = tables[j].len;
                if (checkRegion(tables[j].offset, length)) {
                    checksum = computeTableChecksum(std::span(file.data() + tables[j].offset, length));
                }
            } else if (needVerticalMetrics && i == t42VheaTable) {
                vheaTab[10] = advance / 256; // max advance height
                vheaTab[11] = advance % 256;
                length = vheaTab.size();
                checksum = computeTableChecksum(vheaTab);
            } else if (needVerticalMetrics && i == t42VmtxTable) {
                length = 4 + (nGlyphs - 1) * 2;
                vmtxTab.resize(length, 0);
                vmtxTab[0] = advance / 256;
                vmtxTab[1] = advance % 256;
                checksum = computeTableChecksum(vmtxTab);
            } else if (t42Tables[i].required) {
                //~ error(-1, "Embedded TrueType font is missing a required table ('%s')",
                //~       t42Tables[i].tag);
                length = 0;
                checksum = 0;
            }
        }
        if (length >= 0) {
            newTables[k].tag = ((t42Tables[i].tag[0] & 0xff) << 24) | ((t42Tables[i].tag[1] & 0xff) << 16) | ((t42Tables[i].tag[2] & 0xff) << 8) | (t42Tables[i].tag[3] & 0xff);
            newTables[k].checksum = checksum;
            newTables[k].offset = pos;
            newTables[k].len = length;
            int newPos;
            if (unlikely(checkedAdd(pos, length, &newPos))) {
                ok = false;
            } else {
                pos = newPos;
                if (pos & 3) {
                    pos += 4 - (length & 3);
                }
            }
            ++k;
        }
    }
    if (unlikely(k < nNewTables)) {
        error(errSyntaxWarning, -1, "unexpected number of tables");
        nNewTables = k;
    }

    // construct the table directory
    tableDir[0] = 0x00; // sfnt version
    tableDir[1] = 0x01;
    tableDir[2] = 0x00;
    tableDir[3] = 0x00;
    tableDir[4] = 0; // numTables
    tableDir[5] = nNewTables;
    tableDir[6] = 0; // searchRange
    tableDir[7] = (unsigned char)128;
    tableDir[8] = 0; // entrySelector
    tableDir[9] = 3;
    tableDir[10] = 0; // rangeShift
    tableDir[11] = (unsigned char)(16 * nNewTables - 128);
    pos = 12;
    for (i = 0; i < nNewTables; ++i) {
        tableDir[pos] = (unsigned char)(newTables[i].tag >> 24);
        tableDir[pos + 1] = (unsigned char)(newTables[i].tag >> 16);
        tableDir[pos + 2] = (unsigned char)(newTables[i].tag >> 8);
        tableDir[pos + 3] = (unsigned char)newTables[i].tag;
        tableDir[pos + 4] = (unsigned char)(newTables[i].checksum >> 24);
        tableDir[pos + 5] = (unsigned char)(newTables[i].checksum >> 16);
        tableDir[pos + 6] = (unsigned char)(newTables[i].checksum >> 8);
        tableDir[pos + 7] = (unsigned char)newTables[i].checksum;
        tableDir[pos + 8] = (unsigned char)(newTables[i].offset >> 24);
        tableDir[pos + 9] = (unsigned char)(newTables[i].offset >> 16);
        tableDir[pos + 10] = (unsigned char)(newTables[i].offset >> 8);
        tableDir[pos + 11] = (unsigned char)newTables[i].offset;
        tableDir[pos + 12] = (unsigned char)(newTables[i].len >> 24);
        tableDir[pos + 13] = (unsigned char)(newTables[i].len >> 16);
        tableDir[pos + 14] = (unsigned char)(newTables[i].len >> 8);
        tableDir[pos + 15] = (unsigned char)newTables[i].len;
        pos += 16;
    }

    // compute the font checksum and store it in the head table
    checksum = computeTableChecksum(std::span(tableDir, 12 + nNewTables * 16));
    for (i = 0; i < nNewTables; ++i) {
        checksum += newTables[i].checksum;
    }
    checksum = 0xb1b0afba - checksum; // because the TrueType spec says so
    headData[8] = (unsigned char)(checksum >> 24);
    headData[9] = (unsigned char)(checksum >> 16);
    headData[10] = (unsigned char)(checksum >> 8);
    headData[11] = (unsigned char)checksum;

    // start the sfnts array
    if (name) {
        (*outputFunc)(outputStream, "/", 1);
        (*outputFunc)(outputStream, name->c_str(), name->size());
        (*outputFunc)(outputStream, " [\n", 3);
    } else {
        (*outputFunc)(outputStream, "/sfnts [\n", 9);
    }

    // write the table directory
    dumpString(std::span(tableDir, 12 + nNewTables * 16), outputFunc, outputStream);

    // write the tables
    for (i = 0; i < nNewTables; ++i) {
        if (i == t42HeadTable) {
            dumpString(headData, outputFunc, outputStream);
        } else if (i == t42LocaTable) {
            length = (nGlyphs + 1) * (locaFmt ? 4 : 2);
            dumpString(locaData, outputFunc, outputStream);
        } else if (i == t42GlyfTable) {
            glyfPos = tables[seekTable("glyf")].offset;
            for (j = 0; j < nGlyphs; ++j) {
                if (locaTable[j].len > 0 && checkRegion(glyfPos + locaTable[j].origOffset, locaTable[j].len)) {
                    dumpString(std::span(file.data() + glyfPos + locaTable[j].origOffset, locaTable[j].len), outputFunc, outputStream);
                }
            }
        } else {
            // length == 0 means the table is missing and the error was
            // already reported during the construction of the table
            // headers
            if ((length = newTables[i].len) > 0) {
                if ((j = seekTable(t42Tables[i].tag)) >= 0 && checkRegion(tables[j].offset, tables[j].len)) {
                    dumpString(std::span(file.data() + tables[j].offset, tables[j].len), outputFunc, outputStream);
                } else if (needVerticalMetrics && i == t42VheaTable) {
                    if (unlikely(static_cast<size_t>(length) > sizeof(vheaTab))) {
                        error(errSyntaxWarning, -1, "length bigger than vheaTab size");
                        length = sizeof(vheaTab);
                    }
                    dumpString(vheaTab, outputFunc, outputStream);
                } else if (needVerticalMetrics && i == t42VmtxTable) {
                    if (unlikely(static_cast<size_t>(length) > vmtxTab.size())) {
                        error(errSyntaxWarning, -1, "length bigger than vmtxTab size");
                    }
                    dumpString(vmtxTab, outputFunc, outputStream);
                }
            }
        }
    }

    // end the sfnts array
    (*outputFunc)(outputStream, "] def\n", 6);
}

void FoFiTrueType::dumpString(std::span<const unsigned char> s, FoFiOutputFunc outputFunc, void *outputStream)
{
    (*outputFunc)(outputStream, "<", 1);
    for (size_t i = 0; i < s.size(); i += 32) {
        for (size_t j = 0; j < 32 && i + j < s.size(); ++j) {
            const std::string buf = GooString::format("{0:02x}", s[i + j] & 0xff);
            (*outputFunc)(outputStream, buf.c_str(), buf.size());
        }
        if (i % (65536 - 32) == 65536 - 64) {
            (*outputFunc)(outputStream, ">\n<", 3);
        } else if (i + 32 < s.size()) {
            (*outputFunc)(outputStream, "\n", 1);
        }
    }
    if (s.size() & 3) {
        int pad = 4 - (s.size() & 3);
        for (int i = 0; i < pad; ++i) {
            (*outputFunc)(outputStream, "00", 2);
        }
    }
    // add an extra zero byte because the Adobe Type 42 spec says so
    (*outputFunc)(outputStream, "00>\n", 4);
}

unsigned int FoFiTrueType::computeTableChecksum(std::span<const unsigned char> data)
{
    unsigned checksum = 0;
    for (size_t i = 0; i + 3 < data.size(); i += 4) {
        unsigned int word = ((data[i] & 0xff) << 24) + ((data[i + 1] & 0xff) << 16) + ((data[i + 2] & 0xff) << 8) + (data[i + 3] & 0xff);
        checksum += word;
    }
    if (data.size() & 3) {
        unsigned int word = 0;
        int i = data.size() & ~3;
        switch (data.size() & 3) {
        case 3:
            word |= (data[i + 2] & 0xff) << 8;
            // fallthrough
        case 2:
            word |= (data[i + 1] & 0xff) << 16;
            // fallthrough
        case 1:
            word |= (data[i] & 0xff) << 24;
            break;
        }
        checksum += word;
    }
    return checksum;
}

void FoFiTrueType::parse()
{
    unsigned int topTag;
    int pos, ver, i, j;

    parsedOk = true;

    // look for a collection (TTC)
    topTag = getU32BE(0, &parsedOk);
    if (!parsedOk) {
        return;
    }
    if (topTag == ttcfTag) {
        /* TTC font */
        int dircount;

        dircount = getU32BE(8, &parsedOk);
        if (!parsedOk) {
            return;
        }
        if (!dircount) {
            parsedOk = false;
            return;
        }

        if (faceIndex >= dircount) {
            faceIndex = 0;
        }
        pos = getU32BE(12 + faceIndex * 4, &parsedOk);
        if (!parsedOk) {
            return;
        }
    } else {
        pos = 0;
    }

    // check the sfnt version
    ver = getU32BE(pos, &parsedOk);
    if (!parsedOk) {
        return;
    }
    openTypeCFF = ver == 0x4f54544f; // 'OTTO'

    // read the table directory
    int nTables = getU16BE(pos + 4, &parsedOk);
    if (!parsedOk) {
        return;
    }
    tables.resize(nTables);
    pos += 12;
    j = 0;
    for (i = 0; i < nTables; ++i) {
        tables[j].tag = getU32BE(pos, &parsedOk);
        tables[j].checksum = getU32BE(pos + 4, &parsedOk);
        tables[j].offset = (int)getU32BE(pos + 8, &parsedOk);
        tables[j].len = (int)getU32BE(pos + 12, &parsedOk);
        if (unlikely((tables[j].offset < 0) || (tables[j].len < 0) || (tables[j].offset < INT_MAX - tables[j].len) || (tables[j].len > INT_MAX - tables[j].offset)
                     || (tables[j].offset + tables[j].len >= tables[j].offset && tables[j].offset + tables[j].len <= int(file.size())))) {
            // ignore any bogus entries in the table directory
            ++j;
        }
        pos += 16;
    }
    if (nTables != j) {
        nTables = j;
        tables.resize(nTables);
    }
    if (!parsedOk || tables.empty()) {
        parsedOk = false;
        return;
    }

    // check for tables that are required by both the TrueType spec and
    // the Type 42 spec
    if (seekTable("head") < 0 || seekTable("hhea") < 0 || seekTable("maxp") < 0 || (!openTypeCFF && seekTable("loca") < 0) || (!openTypeCFF && seekTable("glyf") < 0) || (openTypeCFF && (seekTable("CFF ") < 0 && seekTable("CFF2") < 0))) {
        parsedOk = false;
        return;
    }

    // read the cmaps
    if ((i = seekTable("cmap")) >= 0) {
        pos = tables[i].offset + 2;
        int nCmaps = getU16BE(pos, &parsedOk);
        pos += 2;
        if (!parsedOk) {
            return;
        }
        cmaps.resize(nCmaps);
        for (j = 0; j < nCmaps; ++j) {
            cmaps[j].platform = getU16BE(pos, &parsedOk);
            cmaps[j].encoding = getU16BE(pos + 2, &parsedOk);
            cmaps[j].offset = tables[i].offset + getU32BE(pos + 4, &parsedOk);
            pos += 8;
            cmaps[j].fmt = getU16BE(cmaps[j].offset, &parsedOk);
            int lenOffset;
            if (checkedAdd(cmaps[j].offset, 2, &lenOffset)) {
                parsedOk = false;
            } else {
                cmaps[j].len = getU16BE(lenOffset, &parsedOk);
            }
        }
        if (!parsedOk) {
            cmaps.clear();
            return;
        }
    } else {
        cmaps.clear();
    }

    // get the number of glyphs from the maxp table
    i = seekTable("maxp");
    nGlyphs = getU16BE(tables[i].offset + 4, &parsedOk);
    if (!parsedOk) {
        return;
    }

    // get the bbox and loca table format from the head table
    i = seekTable("head");
    bbox[0] = getS16BE(tables[i].offset + 36, &parsedOk);
    bbox[1] = getS16BE(tables[i].offset + 38, &parsedOk);
    bbox[2] = getS16BE(tables[i].offset + 40, &parsedOk);
    bbox[3] = getS16BE(tables[i].offset + 42, &parsedOk);
    locaFmt = getS16BE(tables[i].offset + 50, &parsedOk);
    if (!parsedOk) {
        return;
    }

    // read the post table
    readPostTable();
}

void FoFiTrueType::readPostTable()
{
    std::string name;
    int tablePos, postFmt, stringIdx, stringPos;
    bool ok;
    int i, j, n, m;

    ok = true;
    if ((i = seekTable("post")) < 0) {
        return;
    }
    tablePos = tables[i].offset;
    postFmt = getU32BE(tablePos, &ok);
    if (!ok) {
        goto err;
    }
    if (postFmt == 0x00010000) {
        nameToGID.reserve(258);
        for (i = 0; i < 258; ++i) {
            nameToGID.emplace(macGlyphNames[i], i);
        }
    } else if (postFmt == 0x00020000) {
        nameToGID.reserve(258);
        n = getU16BE(tablePos + 32, &ok);
        if (!ok) {
            goto err;
        }
        if (n > nGlyphs) {
            n = nGlyphs;
        }
        stringIdx = 0;
        stringPos = tablePos + 34 + 2 * n;
        for (i = 0; i < n; ++i) {
            ok = true;
            j = getU16BE(tablePos + 34 + 2 * i, &ok);
            if (j < 258) {
                nameToGID[macGlyphNames[j]] = i;
            } else {
                j -= 258;
                if (j != stringIdx) {
                    for (stringIdx = 0, stringPos = tablePos + 34 + 2 * n; stringIdx < j; ++stringIdx, stringPos += 1 + getU8(stringPos, &ok)) {
                        ;
                    }
                    if (!ok) {
                        continue;
                    }
                }
                m = getU8(stringPos, &ok);
                if (!ok || !checkRegion(stringPos + 1, m)) {
                    continue;
                }
                if (((size_t(stringPos) + 1) == file.size()) || m == 0) {
                    name.clear();
                } else {
                    name.assign((char *)&file[stringPos + 1], m);
                }
                nameToGID[name] = i;
                ++stringIdx;
                stringPos += 1 + m;
            }
        }
    } else if (postFmt == 0x00028000) {
        nameToGID.reserve(258);
        for (i = 0; i < nGlyphs; ++i) {
            j = getU8(tablePos + 32 + i, &ok);
            if (!ok) {
                continue;
            }
            if (j < 258) {
                nameToGID[macGlyphNames[j]] = i;
            }
        }
    }

    return;

err:
    nameToGID.clear();
}

int FoFiTrueType::seekTable(const char *tag) const
{
    const unsigned int tagI = ((tag[0] & 0xff) << 24) | ((tag[1] & 0xff) << 16) | ((tag[2] & 0xff) << 8) | (tag[3] & 0xff);
    const int nTables = tables.size();
    for (int i = 0; i < nTables; ++i) {
        if (tables[i].tag == tagI) {
            return i;
        }
    }
    return -1;
}

unsigned int FoFiTrueType::charToTag(const std::string &tagName)
{
    size_t n = tagName.size();
    unsigned int tag = 0;
    size_t i;

    if (n > 4) {
        n = 4;
    }
    for (i = 0; i < n; i++) {
        tag <<= 8;
        tag |= tagName[i] & 0xff;
    }
    for (; i < 4; i++) {
        tag <<= 8;
        tag |= ' ';
    }
    return tag;
}

/*
  setup GSUB table data
  Only supporting vertical text substitution.
*/
int FoFiTrueType::setupGSUB(const std::string &scriptName, const std::string &languageName)
{
    unsigned int gsubTable;
    unsigned int i;
    unsigned int scriptList, featureList;
    unsigned int scriptCount;
    unsigned int tag;
    unsigned int scriptTable = 0;
    unsigned int langSys;
    unsigned int featureCount;
    unsigned int featureIndex;
    unsigned int ftable = 0;
    unsigned int llist;
    int x;
    unsigned int pos;

    const unsigned int scriptTag = charToTag(scriptName);
    /* read GSUB Header */
    if ((x = seekTable("GSUB")) < 0) {
        return 0; /* GSUB table not found */
    }
    gsubTable = tables[x].offset;
    pos = gsubTable + 4;
    scriptList = getU16BE(pos, &parsedOk);
    pos += 2;
    featureList = getU16BE(pos, &parsedOk);
    pos += 2;
    llist = getU16BE(pos, &parsedOk);

    gsubLookupList = llist + gsubTable; /* change to offset from top of file */
    /* read script list table */
    pos = gsubTable + scriptList;
    scriptCount = getU16BE(pos, &parsedOk);
    pos += 2;
    /* find  script */
    for (i = 0; i < scriptCount; i++) {
        tag = getU32BE(pos, &parsedOk);
        pos += 4;
        scriptTable = getU16BE(pos, &parsedOk);
        pos += 2;
        if (tag == scriptTag) {
            /* found */
            break;
        }
    }
    if (i >= scriptCount) {
        /* not found */
        return 0;
    }

    /* read script table */
    /* use default language system */
    pos = gsubTable + scriptList + scriptTable;
    langSys = 0;
    const unsigned int langTag = charToTag(languageName);
    const unsigned int langCount = getU16BE(pos + 2, &parsedOk);
    for (i = 0; i < langCount && langSys == 0; i++) {
        tag = getU32BE(pos + 4 + i * (4 + 2), &parsedOk);
        if (tag == langTag) {
            langSys = getU16BE(pos + 4 + i * (4 + 2) + 4, &parsedOk);
        }
    }
    if (langSys == 0) {
        /* default language system */
        langSys = getU16BE(pos, &parsedOk);
    }

    /* read LangSys table */
    if (langSys == 0) {
        /* no default LangSys */
        return 0;
    }

    pos = gsubTable + scriptList + scriptTable + langSys + 2;
    featureIndex = getU16BE(pos, &parsedOk); /* ReqFeatureIndex */
    pos += 2;

    if (featureIndex != 0xffff) {
        unsigned int tpos;
        /* read feature record */
        tpos = gsubTable + featureList;
        featureCount = getU16BE(tpos, &parsedOk);
        tpos = gsubTable + featureList + 2 + featureIndex * (4 + 2);
        tag = getU32BE(tpos, &parsedOk);
        tpos += 4;
        if (tag == vrt2Tag) {
            /* vrt2 is preferred, overwrite vert */
            ftable = getU16BE(tpos, &parsedOk);
            /* convert to offset from file top */
            gsubFeatureTable = ftable + gsubTable + featureList;
            return 0;
        }
        if (tag == vertTag) {
            ftable = getU16BE(tpos, &parsedOk);
        }
    }
    featureCount = getU16BE(pos, &parsedOk);
    pos += 2;
    /* find 'vrt2' or 'vert' feature */
    for (i = 0; i < featureCount; i++) {
        unsigned int oldPos;

        featureIndex = getU16BE(pos, &parsedOk);
        pos += 2;
        oldPos = pos; /* save position */
        /* read feature record */
        pos = gsubTable + featureList + 2 + featureIndex * (4 + 2);
        tag = getU32BE(pos, &parsedOk);
        pos += 4;
        if (tag == vrt2Tag) {
            /* vrt2 is preferred, overwrite vert */
            ftable = getU16BE(pos, &parsedOk);
            break;
        }
        if (ftable == 0 && tag == vertTag) {
            ftable = getU16BE(pos, &parsedOk);
        }
        pos = oldPos; /* restore old position */
    }
    if (ftable == 0) {
        /* vert nor vrt2 are not found */
        return 0;
    }
    /* convert to offset from file top */
    gsubFeatureTable = ftable + gsubTable + featureList;
    return 0;
}

unsigned int FoFiTrueType::doMapToVertGID(unsigned int orgGID)
{
    unsigned int lookupCount;
    unsigned int lookupListIndex;
    unsigned int i;
    unsigned int gid = 0;
    unsigned int pos;

    pos = gsubFeatureTable + 2;
    lookupCount = getU16BE(pos, &parsedOk);
    pos += 2;
    for (i = 0; i < lookupCount; i++) {
        lookupListIndex = getU16BE(pos, &parsedOk);
        pos += 2;
        if ((gid = scanLookupList(lookupListIndex, orgGID)) != 0) {
            break;
        }
    }
    return gid;
}

unsigned int FoFiTrueType::mapToVertGID(unsigned int orgGID)
{
    unsigned int mapped;

    if (gsubFeatureTable == 0) {
        return orgGID;
    }
    if ((mapped = doMapToVertGID(orgGID)) != 0) {
        return mapped;
    }
    return orgGID;
}

unsigned int FoFiTrueType::scanLookupList(unsigned int listIndex, unsigned int orgGID)
{
    unsigned int lookupTable;
    unsigned int subTableCount;
    unsigned int subTable;
    unsigned int i;
    unsigned int gid = 0;
    unsigned int pos;

    if (gsubLookupList == 0) {
        return 0; /* no lookup list */
    }
    pos = gsubLookupList + 2 + listIndex * 2;
    lookupTable = getU16BE(pos, &parsedOk);
    /* read lookup table */
    pos = gsubLookupList + lookupTable + 4;
    subTableCount = getU16BE(pos, &parsedOk);
    pos += 2;
    ;
    for (i = 0; i < subTableCount; i++) {
        subTable = getU16BE(pos, &parsedOk);
        pos += 2;
        if ((gid = scanLookupSubTable(gsubLookupList + lookupTable + subTable, orgGID)) != 0) {
            break;
        }
    }
    return gid;
}

unsigned int FoFiTrueType::scanLookupSubTable(unsigned int subTable, unsigned int orgGID)
{
    unsigned int format;
    unsigned int coverage;
    int delta;
    int glyphCount;
    unsigned int substitute;
    unsigned int gid = 0;
    int coverageIndex;
    int pos;

    pos = subTable;
    format = getU16BE(pos, &parsedOk);
    pos += 2;
    coverage = getU16BE(pos, &parsedOk);
    pos += 2;
    if ((coverageIndex = checkGIDInCoverage(subTable + coverage, orgGID)) >= 0) {
        switch (format) {
        case 1:
            /* format 1 */
            delta = getS16BE(pos, &parsedOk);
            pos += 2;
            gid = orgGID + delta;
            break;
        case 2:
            /* format 2 */
            glyphCount = getS16BE(pos, &parsedOk);
            pos += 2;
            if (glyphCount > coverageIndex) {
                pos += coverageIndex * 2;
                substitute = getU16BE(pos, &parsedOk);
                gid = substitute;
            }
            break;
        default:
            /* unknown format */
            break;
        }
    }
    return gid;
}

int FoFiTrueType::checkGIDInCoverage(unsigned int coverage, unsigned int orgGID)
{
    int index = -1;
    unsigned int format;
    unsigned int count;
    unsigned int i;
    unsigned int pos;

    pos = coverage;
    format = getU16BE(pos, &parsedOk);
    pos += 2;
    switch (format) {
    case 1:
        count = getU16BE(pos, &parsedOk);
        pos += 2;
        // In some poor CJK fonts, key GIDs are not sorted,
        // thus we cannot finish checking even when the range
        // including orgGID seems to have already passed.
        for (i = 0; i < count; i++) {
            unsigned int gid;

            gid = getU16BE(pos, &parsedOk);
            pos += 2;
            if (gid == orgGID) {
                /* found */
                index = i;
                break;
            }
        }
        break;
    case 2:
        count = getU16BE(pos, &parsedOk);
        pos += 2;
        for (i = 0; i < count; i++) {
            unsigned int startGID, endGID;
            unsigned int startIndex;

            startGID = getU16BE(pos, &parsedOk);
            pos += 2;
            endGID = getU16BE(pos, &parsedOk);
            pos += 2;
            startIndex = getU16BE(pos, &parsedOk);
            pos += 2;
            // In some poor CJK fonts, key GIDs are not sorted,
            // thus we cannot finish checking even when the range
            // including orgGID seems to have already passed.
            if (startGID <= orgGID && orgGID <= endGID) {
                /* found */
                index = startIndex + orgGID - startGID;
                break;
            }
        }
        break;
    default:
        break;
    }
    return index;
}
