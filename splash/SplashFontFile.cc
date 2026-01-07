//========================================================================
//
// SplashFontFile.cc
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
// Copyright (C) 2008, 2022, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2019 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "SplashFontFile.h"
#include "SplashFontFileID.h"

//------------------------------------------------------------------------
// SplashFontFile
//------------------------------------------------------------------------

SplashFontFile::SplashFontFile(std::unique_ptr<SplashFontFileID> idA, std::unique_ptr<SplashFontSrc> srcA) : src(std::move(srcA))
{
    id = std::move(idA);
    refCnt = 0;
    doAdjustMatrix = false;
}

SplashFontFile::~SplashFontFile() = default;

void SplashFontFile::incRefCnt()
{
    ++refCnt;
}

void SplashFontFile::decRefCnt()
{
    if (!--refCnt) {
        delete this;
    }
}

//

SplashFontSrc::SplashFontSrc() = default;

SplashFontSrc::~SplashFontSrc() = default;

void SplashFontSrc::setFile(const std::string &file)
{
    isFile = true;
    fileName = file;
}

void SplashFontSrc::setBuf(std::vector<unsigned char> &&bufA)
{
    isFile = false;
    buf = std::move(bufA);
}
