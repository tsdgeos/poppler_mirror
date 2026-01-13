//========================================================================
//
// SplashFTFontFile.cc
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
// Copyright (C) 2014, 2017, 2022 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2017, 2018, 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2018, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "goo/ft_utils.h"
#include "poppler/GfxFont.h"
#include "SplashFTFontEngine.h"
#include "SplashFTFont.h"
#include "SplashFTFontFile.h"
#include "SplashFontFileID.h"

//------------------------------------------------------------------------
// SplashFTFontFile
//------------------------------------------------------------------------

std::shared_ptr<SplashFontFile> SplashFTFontFile::loadType1Font(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, const char **encA, int faceIndexA)
{
    FT_Face faceA;
    const char *name;
    int i;

    if (src->isFile()) {
        if (ft_new_face_from_file(engineA->lib, src->fileName().c_str(), faceIndexA, &faceA)) {
            return nullptr;
        }
    } else {
        if (FT_New_Memory_Face(engineA->lib, (const FT_Byte *)src->buf().data(), src->buf().size(), faceIndexA, &faceA)) {
            return nullptr;
        }
    }
    std::vector<int> codeToGIDA;
    codeToGIDA.resize(256, 0);
    for (i = 0; i < 256; ++i) {
        if ((name = encA[i])) {
            codeToGIDA[i] = (int)FT_Get_Name_Index(faceA, (char *)name);
            if (codeToGIDA[i] == 0) {
                name = GfxFont::getAlternateName(name);
                if (name) {
                    codeToGIDA[i] = FT_Get_Name_Index(faceA, (char *)name);
                }
            }
        }
    }

    return std::make_shared<SplashFTFontFile>(engineA, std::move(idA), std::move(src), faceA, std::move(codeToGIDA), false, true);
}

std::shared_ptr<SplashFontFile> SplashFTFontFile::loadCIDFont(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGIDA, int faceIndexA)
{
    FT_Face faceA;

    if (src->isFile()) {
        if (ft_new_face_from_file(engineA->lib, src->fileName().c_str(), faceIndexA, &faceA)) {
            return nullptr;
        }
    } else {
        if (FT_New_Memory_Face(engineA->lib, (const FT_Byte *)src->buf().data(), src->buf().size(), faceIndexA, &faceA)) {
            return nullptr;
        }
    }

    return std::make_shared<SplashFTFontFile>(engineA, std::move(idA), std::move(src), faceA, std::move(codeToGIDA), false, false);
}

std::shared_ptr<SplashFontFile> SplashFTFontFile::loadTrueTypeFont(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> src, std::vector<int> &&codeToGIDA, int faceIndexA)
{
    FT_Face faceA;

    if (src->isFile()) {
        if (ft_new_face_from_file(engineA->lib, src->fileName().c_str(), faceIndexA, &faceA)) {
            return nullptr;
        }
    } else {
        if (FT_New_Memory_Face(engineA->lib, (const FT_Byte *)src->buf().data(), src->buf().size(), faceIndexA, &faceA)) {
            return nullptr;
        }
    }

    return std::make_shared<SplashFTFontFile>(engineA, std::move(idA), std::move(src), faceA, std::move(codeToGIDA), true, false);
}

SplashFTFontFile::SplashFTFontFile(SplashFTFontEngine *engineA, std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> srcA, FT_Face faceA, std::vector<int> &&codeToGIDA, bool trueTypeA, bool type1A, PrivateTag /*unused*/)
    : SplashFontFile(std::move(idA), std::move(srcA))
{
    engine = engineA;
    face = faceA;
    codeToGID = std::move(codeToGIDA);
    trueType = trueTypeA;
    type1 = type1A;
}

SplashFTFontFile::~SplashFTFontFile()
{
    if (face) {
        FT_Done_Face(face);
    }
}

SplashFont *SplashFTFontFile::makeFont(const std::array<SplashCoord, 4> &mat, const std::array<SplashCoord, 4> &textMat)
{
    SplashFont *font;

    font = new SplashFTFont(this->shared_from_this(), mat, textMat);
    font->initCache();
    return font;
}
