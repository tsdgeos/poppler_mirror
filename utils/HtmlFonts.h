//========================================================================
//
// This file comes from pdftohtml project
// http://pdftohtml.sourceforge.net
//
// Copyright from:
// Gueorgui Ovtcharov
// Rainer Dorsch <http://www.ra.informatik.uni-stuttgart.de/~rainer/>
// Mikhail Kruk <meshko@cs.brandeis.edu>
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2010 OSSD CDAC Mumbai by Leena Chourey (leenac@cdacmumbai.in) and Onkar Potdar (onkar@cdacmumbai.in)
// Copyright (C) 2010, 2012, 2017, 2018, 2020, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2011 Steven Murdoch <Steven.Murdoch@cl.cam.ac.uk>
// Copyright (C) 2011 Joshua Richardson <jric@chegg.com>
// Copyright (C) 2012 Igor Slepchin <igor.slepchin@gmail.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2020 Eddie Kohler <ekohler@gmail.com>
// Copyright (C) 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef _HTML_FONTS_H
#define _HTML_FONTS_H
#include "goo/GooString.h"
#include "GfxState.h"
#include "CharTypes.h"
#include <vector>

class HtmlFontColor
{
private:
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int opacity;
    static bool Ok(unsigned int xcol) { return xcol <= 255; }
    static std::string convtoX(unsigned int xcol);

public:
    HtmlFontColor() : r(0), g(0), b(0), opacity(255) { }
    HtmlFontColor(GfxRGB rgb, double opacity);
    std::string toString() const;
    double getOpacity() const { return opacity / 255.0; }
    bool isEqual(HtmlFontColor col) const { return ((r == col.r) && (g == col.g) && (b == col.b) && (opacity == col.opacity)); }
};

class HtmlFont
{
private:
    int size;
    int lineSize;
    bool italic;
    bool bold;
    bool rotOrSkewed;
    std::string familyName;
    std::string FontName;
    HtmlFontColor color;
    std::array<double, 4> rotSkewMat; // only four values needed for rotation and skew
public:
    HtmlFont(const GfxFont &font, int _size, GfxRGB rgb, double opacity);
    HtmlFontColor getColor() const { return color; }
    ~HtmlFont();
    std::string getFullName() const;
    bool isItalic() const { return italic; }
    bool isBold() const { return bold; }
    bool isRotOrSkewed() const { return rotOrSkewed; }
    int getSize() const { return size; }
    int getLineSize() const { return lineSize; }
    void setLineSize(int _lineSize) { lineSize = _lineSize; }
    void setRotMat(const std::array<double, 4> &mat)
    {
        rotOrSkewed = true;
        rotSkewMat = mat;
    }
    const std::array<double, 4> &getRotMat() const { return rotSkewMat; }
    std::string getFontName() const;
    static std::unique_ptr<GooString> HtmlFilter(const Unicode *u, int uLen); // char* s);
    bool isEqual(const HtmlFont &x) const;
    bool isEqualIgnoreBold(const HtmlFont &x) const;
    void print() const { printf("font: %s (%s) %d %s%s\n", FontName.c_str(), familyName.c_str(), size, bold ? "bold " : "", italic ? "italic " : ""); };
};

class HtmlFontAccu
{
private:
    std::vector<HtmlFont> accu;

public:
    HtmlFontAccu();
    ~HtmlFontAccu();
    HtmlFontAccu(const HtmlFontAccu &) = delete;
    HtmlFontAccu &operator=(const HtmlFontAccu &) = delete;
    int AddFont(const HtmlFont &font);
    const HtmlFont *Get(int i) const { return &accu[i]; }
    std::unique_ptr<GooString> CSStyle(int i, int j = 0);
    int size() const { return accu.size(); }
};
#endif
