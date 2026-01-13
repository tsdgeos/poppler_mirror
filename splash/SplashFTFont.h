//========================================================================
//
// SplashFTFont.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2007-2009, 2011, 2018, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Petr Gajdos <pgajdos@novell.com>
// Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHFTFONT_H
#define SPLASHFTFONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include "SplashFont.h"

class SplashFTFontFile;

//------------------------------------------------------------------------
// SplashFTFont
//------------------------------------------------------------------------

class SplashFTFont : public SplashFont
{
public:
    SplashFTFont(const std::shared_ptr<SplashFTFontFile> &fontFileA, const std::array<SplashCoord, 4> &matA, const std::array<SplashCoord, 4> &textMatA);

    ~SplashFTFont() override;

    // Munge xFrac and yFrac before calling SplashFont::getGlyph.
    bool getGlyph(int c, int xFrac, int yFrac, SplashGlyphBitmap *bitmap, int x0, int y0, SplashClip *clip, SplashClipResult *clipRes) override;

    // Rasterize a glyph.  The <xFrac> and <yFrac> values are the same
    // as described for getGlyph.
    bool makeGlyph(int c, int xFrac, int yFrac, SplashGlyphBitmap *bitmap, int x0, int y0, SplashClip *clip, SplashClipResult *clipRes) override;

    // Return the path for a glyph.
    SplashPath *getGlyphPath(int c) override;

    // Return the advance of a glyph. (in 0..1 range)
    double getGlyphAdvance(int c) override;

private:
    FT_Size sizeObj;
    FT_Matrix matrix;
    FT_Matrix textMatrix;
    SplashCoord textScale = 0;
    int size;
    bool enableFreeTypeHinting;
    bool enableSlightHinting;
    bool isOk = false;
};

#endif
