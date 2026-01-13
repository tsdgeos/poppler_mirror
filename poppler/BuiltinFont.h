//========================================================================
//
// BuiltinFont.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2018, 2020, 2025 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef BUILTINFONT_H
#define BUILTINFONT_H

#include "BuiltinFontWidth.h"
#include "FontEncodingTables.h"

#include <cstddef>
#include <cstring>

//------------------------------------------------------------------------

using GetWidthFunction = const BuiltinFontWidth *(*)(const char *str, size_t len);

struct BuiltinFont
{
    const char *name;
    const char **defaultBaseEnc;
    short ascent;
    short descent;
    short bbox[4];
    GetWidthFunction f;

    bool getWidth(const char *n, unsigned short *w) const
    {
        const struct BuiltinFontWidth *bfw = f(n, strlen(n));
        if (!bfw) {
            return false;
        }

        *w = bfw->width;
        return true;
    }
};

//------------------------------------------------------------------------

extern "C" {
// define the gperf generated Lookup functions
const struct BuiltinFontWidth *CourierWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *CourierBoldWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *CourierBoldObliqueWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *CourierObliqueWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *HelveticaWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *HelveticaBoldWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *HelveticaBoldObliqueWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *HelveticaObliqueWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *SymbolWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *TimesBoldWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *TimesBoldItalicWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *TimesItalicWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *TimesRomanWidthsLookup(const char *str, size_t len);
const struct BuiltinFontWidth *ZapfDingbatsWidthsLookup(const char *str, size_t len);
}

static const BuiltinFont builtinFonts[] = { { .name = "Courier", .defaultBaseEnc = standardEncoding, .ascent = 629, .descent = -157, .bbox = { -23, -250, 715, 805 }, .f = &CourierWidthsLookup },
                                            { .name = "Courier-Bold", .defaultBaseEnc = standardEncoding, .ascent = 629, .descent = -157, .bbox = { -113, -250, 749, 801 }, .f = &CourierBoldWidthsLookup },
                                            { .name = "Courier-BoldOblique", .defaultBaseEnc = standardEncoding, .ascent = 629, .descent = -157, .bbox = { -57, -250, 869, 801 }, .f = &CourierBoldObliqueWidthsLookup },
                                            { .name = "Courier-Oblique", .defaultBaseEnc = standardEncoding, .ascent = 629, .descent = -157, .bbox = { -27, -250, 849, 805 }, .f = &CourierObliqueWidthsLookup },
                                            { .name = "Helvetica", .defaultBaseEnc = standardEncoding, .ascent = 718, .descent = -207, .bbox = { -166, -225, 1000, 931 }, .f = &HelveticaWidthsLookup },
                                            { .name = "Helvetica-Bold", .defaultBaseEnc = standardEncoding, .ascent = 718, .descent = -207, .bbox = { -170, -228, 1003, 962 }, .f = &HelveticaBoldWidthsLookup },
                                            { .name = "Helvetica-BoldOblique", .defaultBaseEnc = standardEncoding, .ascent = 718, .descent = -207, .bbox = { -174, -228, 1114, 962 }, .f = &HelveticaBoldObliqueWidthsLookup },
                                            { .name = "Helvetica-Oblique", .defaultBaseEnc = standardEncoding, .ascent = 718, .descent = -207, .bbox = { -170, -225, 1116, 931 }, .f = &HelveticaObliqueWidthsLookup },
                                            { .name = "Symbol", .defaultBaseEnc = symbolEncoding, .ascent = 1010, .descent = -293, .bbox = { -180, -293, 1090, 1010 }, .f = &SymbolWidthsLookup },
                                            { .name = "Times-Bold", .defaultBaseEnc = standardEncoding, .ascent = 683, .descent = -217, .bbox = { -168, -218, 1000, 935 }, .f = &TimesBoldWidthsLookup },
                                            { .name = "Times-BoldItalic", .defaultBaseEnc = standardEncoding, .ascent = 683, .descent = -217, .bbox = { -200, -218, 996, 921 }, .f = &TimesBoldItalicWidthsLookup },
                                            { .name = "Times-Italic", .defaultBaseEnc = standardEncoding, .ascent = 683, .descent = -217, .bbox = { -169, -217, 1010, 883 }, .f = &TimesItalicWidthsLookup },
                                            { .name = "Times-Roman", .defaultBaseEnc = standardEncoding, .ascent = 683, .descent = -217, .bbox = { -168, -218, 1000, 898 }, .f = &TimesRomanWidthsLookup },
                                            { .name = "ZapfDingbats", .defaultBaseEnc = zapfDingbatsEncoding, .ascent = 820, .descent = -143, .bbox = { -1, -143, 981, 820 }, .f = &ZapfDingbatsWidthsLookup } };

static const BuiltinFont *builtinFontSubst[] = { &builtinFonts[0], &builtinFonts[3], &builtinFonts[1],  &builtinFonts[2],  &builtinFonts[4], &builtinFonts[7],
                                                 &builtinFonts[5], &builtinFonts[6], &builtinFonts[12], &builtinFonts[11], &builtinFonts[9], &builtinFonts[10] };

//------------------------------------------------------------------------

#endif
