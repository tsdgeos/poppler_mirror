//========================================================================
//
// SplashFTFontEngine.h
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
// Copyright (C) 2009 Petr Gajdos <pgajdos@novell.com>
// Copyright (C) 2009, 2018, 2022, 2024, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHFTFONTENGINE_H
#define SPLASHFTFONTENGINE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory>
#include <vector>

class SplashFontFile;
class SplashFontFileID;
class SplashFontSrc;

//------------------------------------------------------------------------
// SplashFTFontEngine
//------------------------------------------------------------------------

class SplashFTFontEngine
{
public:
    static SplashFTFontEngine *init(bool aaA, bool enableFreeTypeHintingA, bool enableSlightHinting);

    ~SplashFTFontEngine();

    SplashFTFontEngine(const SplashFTFontEngine &) = delete;
    SplashFTFontEngine &operator=(const SplashFTFontEngine &) = delete;

    // Load fonts.
    std::shared_ptr<SplashFontFile> loadType1Font(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex);
    std::shared_ptr<SplashFontFile> loadType1CFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex);
    std::shared_ptr<SplashFontFile> loadOpenTypeT1CFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex);
    std::shared_ptr<SplashFontFile> loadCIDFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, int faceIndex);
    std::shared_ptr<SplashFontFile> loadOpenTypeCFFFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGID, int faceIndex);
    std::shared_ptr<SplashFontFile> loadTrueTypeFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGID, int faceIndex);
    bool getAA() const { return aa; }
    void setAA(bool aaA) { aa = aaA; }

private:
    SplashFTFontEngine(bool aaA, bool enableFreeTypeHintingA, bool enableSlightHintingA, FT_Library libA);

    bool aa;
    bool enableFreeTypeHinting;
    bool enableSlightHinting;
    FT_Library lib;

    friend class SplashFTFontFile;
    friend class SplashFTFont;
};

#endif
