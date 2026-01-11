//========================================================================
//
// SplashXPathScanner.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2013, 2014, 2021 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2018, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2018, 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHXPATHSCANNER_H
#define SPLASHXPATHSCANNER_H

#include <poppler-config.h>

#if USE_BOOST_HEADERS
#    include <boost/container/small_vector.hpp>
#endif

#include <vector>

class SplashXPath;
class SplashBitmap;

struct SplashIntersect
{
    int x0, x1; // intersection of segment with [y, y+1)
    int count; // EO/NZWN counter increment
};

//------------------------------------------------------------------------
// SplashXPathScanner
//------------------------------------------------------------------------

class SplashXPathScanner
{
public:
    // Create a new SplashXPathScanner object.  <xPathA> must be sorted.
    SplashXPathScanner(const SplashXPath &xPath, bool eoA, int clipYMin, int clipYMax);

    ~SplashXPathScanner();

    SplashXPathScanner(const SplashXPathScanner &) = delete;
    SplashXPathScanner &operator=(const SplashXPathScanner &) = delete;

    // Return the path's bounding box.
    void getBBox(int *xMinA, int *yMinA, int *xMaxA, int *yMaxA) const
    {
        *xMinA = xMin;
        *yMinA = yMin;
        *xMaxA = xMax;
        *yMaxA = yMax;
    }

    // Return the path's bounding box.
    void getBBoxAA(int *xMinA, int *yMinA, int *xMaxA, int *yMaxA) const;

    // Returns true if (<x>,<y>) is inside the path.
    bool test(int x, int y) const;

    // Returns true if the entire span ([<x0>,<x1>], <y>) is inside the
    // path.
    bool testSpan(int x0, int x1, int y) const;

    // Renders one anti-aliased line into <aaBuf>.  Returns the min and
    // max x coordinates with non-zero pixels in <x0> and <x1>.
    void renderAALine(SplashBitmap *aaBuf, int *x0, int *x1, int y, bool adjustVertLine = false) const;

    // Clips an anti-aliased line by setting pixels to zero.  On entry,
    // all non-zero pixels are between <x0> and <x1>.  This function
    // will update <x0> and <x1>.
    void clipAALine(SplashBitmap *aaBuf, const int *x0, const int *x1, int y) const;

private:
    void computeIntersections(const SplashXPath &xPath);
    void addIntersection(double segYMin, int y, int x0, int x1, int count);

    const bool eo;
    int xMin = 1, yMin = 1, xMax = 0, yMax = 0;

#if USE_BOOST_HEADERS
    using IntersectionLine = boost::container::small_vector<SplashIntersect, 4>;
#else
    using IntersectionLine = std::vector<SplashIntersect>;
#endif
    std::vector<IntersectionLine> allIntersections;

    friend class SplashXPathScanIterator;
};

class SplashXPathScanIterator
{
public:
    SplashXPathScanIterator(const SplashXPathScanner &scanner, int y);

    // Returns the next span inside the path at the current y position
    // Returns false if there are no more spans.
    bool getNextSpan(int *x0, int *x1);

private:
#if USE_BOOST_HEADERS
    using IntersectionLine = boost::container::small_vector<SplashIntersect, 4>;
#else
    using IntersectionLine = std::vector<SplashIntersect>;
#endif
    const IntersectionLine &line;

    size_t interIdx = 0; // current index into <line>
    int interCount = 0; // current EO/NZWN counter
    const bool eo;
};

#endif
