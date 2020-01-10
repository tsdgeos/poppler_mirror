//========================================================================
//
// BuiltinFontTables.h
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
// Copyright (C) 2020 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef BUILTINFONTTABLES_H
#define BUILTINFONTTABLES_H

#include "BuiltinFont.h"

#define nBuiltinFonts      14
#define nBuiltinFontSubsts 12

extern BuiltinFont builtinFonts[nBuiltinFonts];
extern const BuiltinFont *builtinFontSubst[nBuiltinFontSubsts];

extern void initBuiltinFontTables();
extern void freeBuiltinFontTables();

#endif
