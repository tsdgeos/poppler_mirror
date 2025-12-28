//========================================================================
//
// SplashPattern.cc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2010, 2011 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2020, 2021, 2025 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "SplashPattern.h"

//------------------------------------------------------------------------
// SplashPattern
//------------------------------------------------------------------------

SplashPattern::SplashPattern() = default;

SplashPattern::~SplashPattern() = default;

//------------------------------------------------------------------------
// SplashSolidColor
//------------------------------------------------------------------------

SplashSolidColor::SplashSolidColor(SplashColorConstPtr colorA)
{
    splashColorCopy(color, colorA);
}

SplashSolidColor::~SplashSolidColor() = default;

bool SplashSolidColor::getColor(int /*x*/, int /*y*/, SplashColorPtr c) const
{
    splashColorCopy(c, color);
    return true;
}

//------------------------------------------------------------------------
// SplashGouraudColor
//------------------------------------------------------------------------

SplashGouraudColor::~SplashGouraudColor() = default;
