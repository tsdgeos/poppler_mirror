//========================================================================
//
// SplashClip.cc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2010, 2021, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013, 2021 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2019, 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <algorithm>
#include "SplashErrorCodes.h"
#include "SplashMath.h"
#include "SplashPath.h"
#include "SplashXPath.h"
#include "SplashXPathScanner.h"
#include "SplashBitmap.h"
#include "SplashClip.h"

//------------------------------------------------------------------------
// SplashClip
//------------------------------------------------------------------------

SplashClip::SplashClip(SplashCoord x0, SplashCoord y0, SplashCoord x1, SplashCoord y1, bool antialiasA)
{
    antialias = antialiasA;
    if (x0 < x1) {
        xMin = x0;
        xMax = x1;
    } else {
        xMin = x1;
        xMax = x0;
    }
    if (y0 < y1) {
        yMin = y0;
        yMax = y1;
    } else {
        yMin = y1;
        yMax = y0;
    }
    xMinI = splashFloor(xMin);
    yMinI = splashFloor(yMin);
    xMaxI = splashCeil(xMax) - 1;
    yMaxI = splashCeil(yMax) - 1;
}

SplashClip::SplashClip(const SplashClip *clip, PrivateTag /*unused*/)
{
    antialias = clip->antialias;
    xMin = clip->xMin;
    yMin = clip->yMin;
    xMax = clip->xMax;
    yMax = clip->yMax;
    xMinI = clip->xMinI;
    yMinI = clip->yMinI;
    xMaxI = clip->xMaxI;
    yMaxI = clip->yMaxI;
    scanners = clip->scanners;
}

void SplashClip::resetToRect(SplashCoord x0, SplashCoord y0, SplashCoord x1, SplashCoord y1)
{
    scanners = {};

    if (x0 < x1) {
        xMin = x0;
        xMax = x1;
    } else {
        xMin = x1;
        xMax = x0;
    }
    if (y0 < y1) {
        yMin = y0;
        yMax = y1;
    } else {
        yMin = y1;
        yMax = y0;
    }
    xMinI = splashFloor(xMin);
    yMinI = splashFloor(yMin);
    xMaxI = splashCeil(xMax) - 1;
    yMaxI = splashCeil(yMax) - 1;
}

SplashError SplashClip::clipToRect(SplashCoord x0, SplashCoord y0, SplashCoord x1, SplashCoord y1)
{
    if (x0 < x1) {
        if (x0 > xMin) {
            xMin = x0;
            xMinI = splashFloor(xMin);
        }
        if (x1 < xMax) {
            xMax = x1;
            xMaxI = splashCeil(xMax) - 1;
        }
    } else {
        if (x1 > xMin) {
            xMin = x1;
            xMinI = splashFloor(xMin);
        }
        if (x0 < xMax) {
            xMax = x0;
            xMaxI = splashCeil(xMax) - 1;
        }
    }
    if (y0 < y1) {
        if (y0 > yMin) {
            yMin = y0;
            yMinI = splashFloor(yMin);
        }
        if (y1 < yMax) {
            yMax = y1;
            yMaxI = splashCeil(yMax) - 1;
        }
    } else {
        if (y1 > yMin) {
            yMin = y1;
            yMinI = splashFloor(yMin);
        }
        if (y0 < yMax) {
            yMax = y0;
            yMaxI = splashCeil(yMax) - 1;
        }
    }
    return SplashError::NoError;
}

namespace {
// returns true if the 4 consecutive segments form a axis aligned rectangle
// first and third segment must be the vertical segments
constexpr bool isRect(const SplashXPathSeg &a, const SplashXPathSeg &b, const SplashXPathSeg &c, const SplashXPathSeg &d)
{
    // Check if segment a and c are vertical, and b and d are horizontal
    if ((a.x0 != a.x1) || (b.y0 != b.y1) || (c.x0 != c.x1) || (d.y0 != d.y1)) {
        return false;
    }
    // Check if x coordinates match
    if ((a.x1 != b.x0) || (b.x1 != c.x0) || (c.x1 != d.x0) || (d.x1 != a.x0)) {
        return false;
    }
    if ((a.y0 != c.y0) || (a.y1 != c.y1)) {
        return false;
    }
    if ((a.y0 == b.y0) && (a.y1 == d.y0)) {
        return true;
    }
    if ((a.y0 == d.y0) && (a.y1 == b.y0)) {
        return true;
    }
    return false;
}
// 4 valid cases - two orientations, start on left or right
static_assert(isRect({ .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 1.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                     { .x0 = 2.0, .y0 = 0.0, .x1 = 0.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
static_assert(isRect({ .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                     { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }));
static_assert(isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 0.0, .x1 = 0.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                     { .x0 = 0.0, .y0 = 1.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }));
static_assert(isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                     { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
// 4 invalid cases, one segment point not closing
static_assert(!isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 3.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
static_assert(!isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 3.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
static_assert(!isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 3.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
static_assert(!isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 0.0, .y0 = 0.0, .x1 = 3.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
// invalid case, closed, but left segment not vertical
static_assert(!isRect({ .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 1.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 1.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 1.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
// invalid case, all horizontal/vertical, but horizontal segments coincident
static_assert(!isRect({ .x0 = 0.0, .y0 = 0.0, .x1 = 0.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 0.0, .y0 = 0.0, .x1 = 2.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }, { .x0 = 2.0, .y0 = 0.0, .x1 = 2.0, .y1 = 1.0, .dxdy = 0.0, .flags = 0 },
                      { .x0 = 2.0, .y0 = 0.0, .x1 = 0.0, .y1 = 0.0, .dxdy = 0.0, .flags = 0 }));
}

SplashError SplashClip::clipToPath(const SplashPath &path, const std::array<SplashCoord, 6> &matrix, SplashCoord flatness, bool eo)
{
    int yMinAA, yMaxAA;

    SplashXPath xPath(path, matrix, flatness, true);

    // check for an empty path
    if (xPath.length == 0) {
        xMax = xMin - 1;
        yMax = yMin - 1;
        xMaxI = splashCeil(xMax) - 1;
        yMaxI = splashCeil(yMax) - 1;

        // check for an axis aligned rectangle
    } else if (xPath.length == 4 && isRect(xPath.segs[0], xPath.segs[1], xPath.segs[2], xPath.segs[3])) {
        clipToRect(xPath.segs[0].x0, xPath.segs[0].y0, xPath.segs[2].x0, xPath.segs[2].y1);
    } else if (xPath.length == 4 && isRect(xPath.segs[1], xPath.segs[2], xPath.segs[3], xPath.segs[0])) {
        clipToRect(xPath.segs[1].x0, xPath.segs[1].y0, xPath.segs[3].x0, xPath.segs[3].y1);

    } else {
        if (antialias) {
            xPath.aaScale();
            yMinAA = yMinI * splashAASize;
            yMaxAA = (yMaxI + 1) * splashAASize - 1;
        } else {
            yMinAA = yMinI;
            yMaxAA = yMaxI;
        }
        scanners.emplace_back(std::make_shared<SplashXPathScanner>(xPath, eo, yMinAA, yMaxAA));
    }

    return SplashError::NoError;
}

SplashClipResult SplashClip::testRect(int rectXMin, int rectYMin, int rectXMax, int rectYMax)
{
    // This tests the rectangle:
    //     x = [rectXMin, rectXMax + 1)    (note: rect coords are ints)
    //     y = [rectYMin, rectYMax + 1)
    // against the clipping region:
    //     x = [xMin, xMax)                (note: clipping coords are fp)
    //     y = [yMin, yMax)
    if ((SplashCoord)(rectXMax + 1) <= xMin || (SplashCoord)rectXMin >= xMax || (SplashCoord)(rectYMax + 1) <= yMin || (SplashCoord)rectYMin >= yMax) {
        return splashClipAllOutside;
    }
    if ((SplashCoord)rectXMin >= xMin && (SplashCoord)(rectXMax + 1) <= xMax && (SplashCoord)rectYMin >= yMin && (SplashCoord)(rectYMax + 1) <= yMax && scanners.empty()) {
        return splashClipAllInside;
    }
    return splashClipPartial;
}

SplashClipResult SplashClip::testSpan(int spanXMin, int spanXMax, int spanY)
{
    // This tests the rectangle:
    //     x = [spanXMin, spanXMax + 1)    (note: span coords are ints)
    //     y = [spanY, spanY + 1)
    // against the clipping region:
    //     x = [xMin, xMax)                (note: clipping coords are fp)
    //     y = [yMin, yMax)
    if ((SplashCoord)(spanXMax + 1) <= xMin || (SplashCoord)spanXMin >= xMax || (SplashCoord)(spanY + 1) <= yMin || (SplashCoord)spanY >= yMax) {
        return splashClipAllOutside;
    }
    if ((SplashCoord)spanXMin < xMin || (SplashCoord)(spanXMax + 1) > xMax || (SplashCoord)spanY < yMin || (SplashCoord)(spanY + 1) > yMax) {
        return splashClipPartial;
    }
    if (antialias) {
        for (const auto &scanner : scanners) {
            if (!scanner->testSpan(spanXMin * splashAASize, spanXMax * splashAASize + (splashAASize - 1), spanY * splashAASize)) {
                return splashClipPartial;
            }
        }
    } else {
        for (const auto &scanner : scanners) {
            if (!scanner->testSpan(spanXMin, spanXMax, spanY)) {
                return splashClipPartial;
            }
        }
    }
    return splashClipAllInside;
}

void SplashClip::clipAALine(SplashBitmap *aaBuf, int *x0, int *x1, int y, bool adjustVertLine)
{
    int xx0, xx1, xx, yy;
    SplashColorPtr p;

    // zero out pixels with x < xMin
    xx0 = *x0 * splashAASize;
    xx1 = splashFloor(xMin * splashAASize);
    if (xx1 > aaBuf->getWidth()) {
        xx1 = aaBuf->getWidth();
    }
    if (xx0 < xx1) {
        xx0 &= ~7;
        for (yy = 0; yy < splashAASize; ++yy) {
            p = aaBuf->getDataPtr() + yy * aaBuf->getRowSize() + (xx0 >> 3);
            for (xx = xx0; xx + 7 < xx1; xx += 8) {
                *p++ = 0;
            }
            if (xx < xx1 && !adjustVertLine) {
                *p &= 0xff >> (xx1 & 7);
            }
        }
        *x0 = splashFloor(xMin);
    }

    // zero out pixels with x > xMax
    xx0 = splashFloor(xMax * splashAASize) + 1;
    if (xx0 < 0) {
        xx0 = 0;
    }
    xx1 = (*x1 + 1) * splashAASize;
    if (xx0 < xx1 && !adjustVertLine) {
        for (yy = 0; yy < splashAASize; ++yy) {
            p = aaBuf->getDataPtr() + yy * aaBuf->getRowSize() + (xx0 >> 3);
            xx = xx0;
            if (xx & 7) {
                *p &= 0xff00 >> (xx & 7);
                xx = (xx & ~7) + 8;
                ++p;
            }
            for (; xx < xx1; xx += 8) {
                *p++ = 0;
            }
        }
        *x1 = splashFloor(xMax);
    }

    // check the paths
    for (const auto &scanner : scanners) {
        scanner->clipAALine(aaBuf, x0, x1, y);
    }
    if (*x0 > *x1) {
        *x0 = *x1;
    }
    if (*x0 < 0) {
        *x0 = 0;
    }
    if ((*x0 >> 1) >= aaBuf->getRowSize()) {
        xx0 = *x0;
        *x0 = (aaBuf->getRowSize() - 1) << 1;
        if (xx0 & 1) {
            *x0 = *x0 + 1;
        }
    }
    if (*x1 < *x0) {
        *x1 = *x0;
    }
    if ((*x1 >> 1) >= aaBuf->getRowSize()) {
        xx0 = *x1;
        *x1 = (aaBuf->getRowSize() - 1) << 1;
        if (xx0 & 1) {
            *x1 = *x1 + 1;
        }
    }
}

bool SplashClip::testClipPaths(int x, int y) const
{
    if (antialias) {
        x *= splashAASize;
        y *= splashAASize;
    }

    auto testXY = [x, y](const auto &scanner) -> bool { //
        return scanner->test(x, y);
    };

    return std::ranges::all_of(scanners, testXY);
}
