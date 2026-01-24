//========================================================================
//
// SplashErrorCodes.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006, 2009, 2026 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHERRORCODES_H
#define SPLASHERRORCODES_H

//------------------------------------------------------------------------

enum class SplashError
{
    NoError, // no error
    NoCurPt, // no current point
    EmptyPath, // zero points in path
    BogusPath, // only one point in subpath
    NoSave, // state stack is empty
    OpenFile, // couldn't open file
    NoGlyph, // couldn't get the requested glyph
    ModeMismatch, // invalid combination of color modes
    SingularMatrix, // matrix is singular
    BadArg, // bad argument
    ZeroImage, // image of 0x0
    Generic
};

#endif
