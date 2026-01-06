//========================================================================
//
// PDFRectange.h
//
// Code originally from Page.cc
//
// Copyright 1996-2007 Glyph & Cog, LLC
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
// Copyright (C) 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef PDFRECTANGLE_H
#define PDFRECTANGLE_H

//------------------------------------------------------------------------

class PDFRectangle
{
public:
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;

    constexpr PDFRectangle() = default;
    constexpr PDFRectangle(double x1A, double y1A, double x2A, double y2A)
    {
        x1 = x1A;
        y1 = y1A;
        x2 = x2A;
        y2 = y2A;
    }
    constexpr bool isValid() const { return x1 != 0 || y1 != 0 || x2 != 0 || y2 != 0; }
    constexpr bool isEmpty() const { return x1 == x2 && y1 == y2; }
    constexpr bool contains(double x, double y) const { return x1 <= x && x <= x2 && y1 <= y && y <= y2; }
    constexpr void clipTo(const PDFRectangle &rect);

    constexpr bool operator==(const PDFRectangle &rect) const { return x1 == rect.x1 && y1 == rect.y1 && x2 == rect.x2 && y2 == rect.y2; }
};

constexpr void PDFRectangle::clipTo(const PDFRectangle &rect)
{
    if (x1 < rect.x1) {
        x1 = rect.x1;
    } else if (x1 > rect.x2) {
        x1 = rect.x2;
    }
    if (x2 < rect.x1) {
        x2 = rect.x1;
    } else if (x2 > rect.x2) {
        x2 = rect.x2;
    }
    if (y1 < rect.y1) {
        y1 = rect.y1;
    } else if (y1 > rect.y2) {
        y1 = rect.y2;
    }
    if (y2 < rect.y1) {
        y2 = rect.y1;
    } else if (y2 > rect.y2) {
        y2 = rect.y2;
    }
}

#endif
