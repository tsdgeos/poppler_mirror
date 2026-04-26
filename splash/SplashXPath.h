//========================================================================
//
// SplashXPath.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2018, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHXPATH_H
#define SPLASHXPATH_H

#include "SplashTypes.h"
#include <memory>
#include <array>

class SplashPath;
struct SplashXPathAdjust;

//------------------------------------------------------------------------

static constexpr int splashMaxCurveSplits = 1 << 10;

//------------------------------------------------------------------------
// SplashXPathSeg
//------------------------------------------------------------------------

struct SplashXPathSeg
{
    double x0, y0; // first endpoint
    double x1, y1; // second endpoint
    double dxdy; // slope: delta-x / delta-y
    unsigned int flags;
};

#define splashXPathHoriz                                                                                                                                                                                                                       \
    0x01 // segment is vertical (y0 == y1)
         //   (dxdy is undef)
#define splashXPathVert 0x02 // segment is horizontal (x0 == x1)
#define splashXPathFlipped 0x04 // y0 > y1

//------------------------------------------------------------------------
// SplashXPath
//------------------------------------------------------------------------

class SplashXPath
{
public:
    // Expands (converts to segments) and flattens (converts curves to
    // lines) <path>.  Transforms all points from user space to device
    // space, via <matrix>.  If <closeSubpaths> is true, closes all open
    // subpaths.
    SplashXPath(const SplashPath &path, const std::array<double, 6> &matrix, double flatness, bool closeSubpaths, bool adjustLines = false, int linePosI = 0);

    ~SplashXPath();

    SplashXPath(const SplashXPath &) = delete;
    SplashXPath &operator=(const SplashXPath &) = delete;

    // Multiply all coordinates by splashAASize, in preparation for
    // anti-aliased rendering.
    void aaScale();

protected:
    static void transform(const std::array<double, 6> &matrix, double xi, double yi, double *xo, double *yo);
    static void strokeAdjust(SplashXPathAdjust *adjust, double *xp, double *yp);
    void grow(int nSegs);
    void addCurve(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double flatness);
    void addSegment(double x0, double y0, double x1, double y1);

    SplashXPathSeg *segs;
    int length, size; // length and size of segs array

    struct CurveData
    {
        std::array<double, (splashMaxCurveSplits + 1) * 3> cx;
        std::array<double, (splashMaxCurveSplits + 1) * 3> cy;
        std::array<int, splashMaxCurveSplits + 1> cNext;
    };
    std::unique_ptr<CurveData> curveData;

    friend class SplashXPathScanner;
    friend class SplashClip;
    friend class Splash;
};

#endif
