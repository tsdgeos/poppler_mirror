//========================================================================
//
// SplashFTFontEngine.cc
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
// Copyright (C) 2009, 2011, 2012, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Petr Gajdos <pgajdos@novell.com>
// Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2019 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2024, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdio>
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/gfile.h"
#include "fofi/FoFiTrueType.h"
#include "fofi/FoFiType1C.h"
#include "SplashFTFontFile.h"
#include "SplashFontFileID.h"
#include "SplashFTFontEngine.h"

//------------------------------------------------------------------------
// SplashFTFontEngine
//------------------------------------------------------------------------

SplashFTFontEngine::SplashFTFontEngine(bool aaA, bool enableFreeTypeHintingA, bool enableSlightHintingA, FT_Library libA)
{
    aa = aaA;
    enableFreeTypeHinting = enableFreeTypeHintingA;
    enableSlightHinting = enableSlightHintingA;
    lib = libA;
}

SplashFTFontEngine *SplashFTFontEngine::init(bool aaA, bool enableFreeTypeHintingA, bool enableSlightHintingA)
{
    FT_Library libA;

    if (FT_Init_FreeType(&libA)) {
        return nullptr;
    }
    return new SplashFTFontEngine(aaA, enableFreeTypeHintingA, enableSlightHintingA, libA);
}

SplashFTFontEngine::~SplashFTFontEngine()
{
    FT_Done_FreeType(lib);
}

SplashFontFile *SplashFTFontEngine::loadType1Font(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, const char **enc, int faceIndex)
{
    return SplashFTFontFile::loadType1Font(this, std::move(idA), src, enc, faceIndex);
}

SplashFontFile *SplashFTFontEngine::loadType1CFont(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, const char **enc, int faceIndex)
{
    return SplashFTFontFile::loadType1Font(this, std::move(idA), src, enc, faceIndex);
}

SplashFontFile *SplashFTFontEngine::loadOpenTypeT1CFont(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, const char **enc, int faceIndex)
{
    return SplashFTFontFile::loadType1Font(this, std::move(idA), src, enc, faceIndex);
}

SplashFontFile *SplashFTFontEngine::loadCIDFont(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, int faceIndex)
{
    return SplashFTFontFile::loadCIDFont(this, std::move(idA), src, {}, faceIndex);
}

SplashFontFile *SplashFTFontEngine::loadOpenTypeCFFFont(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, std::vector<int> &&codeToGID, int faceIndex)
{
    return SplashFTFontFile::loadCIDFont(this, std::move(idA), src, std::move(codeToGID), faceIndex);
}

SplashFontFile *SplashFTFontEngine::loadTrueTypeFont(std::unique_ptr<SplashFontFileID> idA, SplashFontSrc *src, std::vector<int> &&codeToGID, int faceIndex)
{
    SplashFontFile *ret;
    ret = SplashFTFontFile::loadTrueTypeFont(this, std::move(idA), src, std::move(codeToGID), faceIndex);
    return ret;
}
