//========================================================================
//
// SplashFontEngine.cc
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
// Copyright (C) 2009 Kovid Goyal <kovid@kovidgoyal.net>
// Copyright (C) 2009, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2015 Dmytro Morgun <lztoad@gmail.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2019 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <algorithm>

#include "SplashMath.h"
#include "SplashFTFontEngine.h"
#include "SplashFontFile.h"
#include "SplashFontFileID.h"
#include "SplashFont.h"
#include "SplashFontEngine.h"

//------------------------------------------------------------------------
// SplashFontEngine
//------------------------------------------------------------------------

SplashFontEngine::SplashFontEngine(bool enableFreeType, bool enableFreeTypeHinting, bool enableSlightHinting, bool aa)
{
    std::ranges::fill(fontCache, nullptr);

    if (enableFreeType) {
        ftEngine = SplashFTFontEngine::init(aa, enableFreeTypeHinting, enableSlightHinting);
    } else {
        ftEngine = nullptr;
    }
}

SplashFontEngine::~SplashFontEngine()
{
    for (auto *font : fontCache) {
        delete font;
    }

    delete ftEngine;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::getFontFile(const SplashFontFileID &id)
{
    for (auto *font : fontCache) {
        if (font) {
            std::shared_ptr<SplashFontFile> fontFile = font->getFontFile();
            if (fontFile && fontFile->getID().matches(id)) {
                return fontFile;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadType1Font(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadType1Font(std::move(idA), std::move(src), enc, faceIndex);
    }

    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadType1CFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadType1CFont(std::move(idA), std::move(src), enc, faceIndex);
    }

    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadOpenTypeT1CFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **enc, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadOpenTypeT1CFont(std::move(idA), std::move(src), enc, faceIndex);
    }

    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadCIDFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadCIDFont(std::move(idA), std::move(src), faceIndex);
    }

    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadOpenTypeCFFFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGID, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadOpenTypeCFFFont(std::move(idA), std::move(src), std::move(codeToGID), faceIndex);
    }

    return nullptr;
}

std::shared_ptr<SplashFontFile> SplashFontEngine::loadTrueTypeFont(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGID, int faceIndex)
{
    if (ftEngine) {
        return ftEngine->loadTrueTypeFont(std::move(idA), std::move(src), std::move(codeToGID), faceIndex);
    }

    return nullptr;
}

bool SplashFontEngine::getAA()
{
    return (ftEngine == nullptr) ? false : ftEngine->getAA();
}

void SplashFontEngine::setAA(bool aa)
{
    if (ftEngine != nullptr) {
        ftEngine->setAA(aa);
    }
}

SplashFont *SplashFontEngine::getFont(std::shared_ptr<SplashFontFile> fontFile, const std::array<SplashCoord, 4> &textMat, const std::array<SplashCoord, 6> &ctm)
{
    std::array<SplashCoord, 4> mat;

    mat[0] = textMat[0] * ctm[0] + textMat[1] * ctm[2];
    mat[1] = -(textMat[0] * ctm[1] + textMat[1] * ctm[3]);
    mat[2] = textMat[2] * ctm[0] + textMat[3] * ctm[2];
    mat[3] = -(textMat[2] * ctm[1] + textMat[3] * ctm[3]);
    if (!splashCheckDet(mat[0], mat[1], mat[2], mat[3], 0.01)) {
        // avoid a singular (or close-to-singular) matrix
        mat[0] = 0.01;
        mat[1] = 0;
        mat[2] = 0;
        mat[3] = 0.01;
    }

    // Try to find the font in the cache
    auto fontIt = std::ranges::find_if(fontCache, [&](const SplashFont *font) { return font && font->matches(fontFile, mat, textMat); }); // NOLINT(readability-qualified-auto) https://github.com/llvm/llvm-project/issues/175731

    // The requested font has been found in the cache
    if (fontIt != fontCache.end()) {
        std::rotate(fontCache.begin(), fontIt, fontIt + 1);
        return fontCache[0];
    }

    // The requested font has not been found in the cache
    auto *newFont = fontFile->makeFont(mat, textMat);
    if (fontCache.back()) {
        delete fontCache.back();
    }
    std::ranges::rotate(fontCache, fontCache.end() - 1);

    fontCache[0] = newFont;
    return fontCache[0];
}
