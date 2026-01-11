//========================================================================
//
// SplashFTFontFile.h
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
// Copyright (C) 2017, 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2019, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHFTFONTFILE_H
#define SPLASHFTFONTFILE_H

#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "SplashFontFile.h"

#include <memory>

class SplashFontFileID;
class SplashFTFontEngine;

//------------------------------------------------------------------------
// SplashFTFontFile
//------------------------------------------------------------------------

class SplashFTFontFile : public SplashFontFile, public std::enable_shared_from_this<SplashFTFontFile>
{
    class PrivateTag
    {
    };

public:
    static std::shared_ptr<SplashFontFile> loadType1Font(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **encA, int faceIndexA);
    static std::shared_ptr<SplashFontFile> loadCIDFont(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGIDA, int faceIndexA);
    static std::shared_ptr<SplashFontFile> loadTrueTypeFont(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGIDA, int faceIndexA);

    ~SplashFTFontFile() override;

    // Create a new SplashFTFont, i.e., a scaled instance of this font
    // file.
    SplashFont *makeFont(const std::array<SplashCoord, 4> &mat, const std::array<SplashCoord, 4> &textMat) override;

    SplashFTFontFile(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, FT_Face faceA, std::vector<int> &&codeToGIDA, bool trueTypeA, bool type1A, PrivateTag /*unused*/ = {});

private:
    SplashFTFontEngine *engine;
    FT_Face face;
    std::vector<int> codeToGID;
    bool trueType;
    bool type1;

    friend class SplashFTFont;
};

#endif
