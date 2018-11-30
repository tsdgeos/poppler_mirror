//========================================================================
//
// BuiltinFont.h
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
// Copyright (C) 2018 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef BUILTINFONT_H
#define BUILTINFONT_H

struct BuiltinFont;
class BuiltinFontWidths;

//------------------------------------------------------------------------

struct BuiltinFont {
  const char *name;
  const char **defaultBaseEnc;
  short ascent;
  short descent;
  short bbox[4];
  BuiltinFontWidths *widths;
};

//------------------------------------------------------------------------

struct BuiltinFontWidth {
  const char *name;
  unsigned short width;
  BuiltinFontWidth *next;
};

class BuiltinFontWidths {
public:

  BuiltinFontWidths(BuiltinFontWidth *widths, int sizeA);
  ~BuiltinFontWidths();

  BuiltinFontWidths(const BuiltinFontWidths &) = delete;
  BuiltinFontWidths& operator=(const BuiltinFontWidths &) = delete;

  bool getWidth(const char *name, unsigned short *width);

private:

  int hash(const char *name);

  BuiltinFontWidth **tab;
  int size;
};

#endif
