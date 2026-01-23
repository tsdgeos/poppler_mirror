//========================================================================
//
// SplashXPathScanner.cc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2008, 2010, 2014, 2018, 2019, 2021, 2022, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010 Paweł Wiejacha <pawel.wiejacha@gmail.com>
// Copyright (C) 2013, 2014, 2021 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2018, 2025 Stefan Brüns <stefan.bruens@rwth-aachen.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <limits>
#include "goo/GooLikely.h"
#include "SplashMath.h"
#include "SplashXPath.h"
#include "SplashBitmap.h"
#include "SplashXPathScanner.h"

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// SplashXPathScanner
//------------------------------------------------------------------------

SplashXPathScanner::SplashXPathScanner(const SplashXPath &xPath, bool eoA, int clipYMin, int clipYMax) //
    : eo(eoA)
{
    // compute the bbox
    if (xPath.length == 0) {
        return;
    }
    if (clipYMin > clipYMax) {
        return;
    }

    SplashCoord xMaxFP = std::numeric_limits<SplashCoord>::lowest();
    SplashCoord xMinFP = std::numeric_limits<SplashCoord>::max();
    SplashCoord yMaxFP = std::numeric_limits<SplashCoord>::lowest();
    SplashCoord yMinFP = std::numeric_limits<SplashCoord>::max();

    SplashCoord clipYMinFP = clipYMin;
    SplashCoord clipYMaxFP = clipYMax + 1.0;

    for (int i = 0; i < xPath.length; ++i) {
        const SplashXPathSeg *seg = &xPath.segs[i];
        if (unlikely(std::isnan(seg->x0) || std::isnan(seg->x1) || std::isnan(seg->y0) || std::isnan(seg->y1))) {
            return;
        }

        const SplashCoord segYMin = seg->y0;
        const SplashCoord segYMax = seg->y1;
        if (segYMin >= clipYMaxFP) {
            continue;
        }
        if (segYMax < clipYMinFP) {
            continue;
        }

        if (segYMin < yMinFP) {
            yMinFP = segYMin;
        }
        if (segYMax > yMaxFP) {
            yMaxFP = segYMax;
        }
        if (seg->x0 < xMinFP) {
            xMinFP = seg->x0;
        }
        if (seg->x0 > xMaxFP) {
            xMaxFP = seg->x0;
        }
        if (seg->x1 < xMinFP) {
            xMinFP = seg->x1;
        }
        if (seg->x1 > xMaxFP) {
            xMaxFP = seg->x1;
        }
    }
    if (yMinFP > yMaxFP) {
        return;
    }

    xMin = splashFloor(xMinFP);
    xMax = splashFloor(xMaxFP);
    yMin = splashFloor(yMinFP);
    yMax = splashFloor(yMaxFP);
    if (clipYMin > yMin) {
        yMin = clipYMin;
    }
    if (clipYMax < yMax) {
        yMax = clipYMax;
    }

    if (yMin > yMax) {
        // This means the splashFloors overflowed/underflowed
        return;
    }

    computeIntersections(xPath);
}

SplashXPathScanner::~SplashXPathScanner() = default;

void SplashXPathScanner::getBBoxAA(int *xMinA, int *yMinA, int *xMaxA, int *yMaxA) const
{
    *xMinA = xMin / splashAASize;
    *yMinA = yMin / splashAASize;
    *xMaxA = xMax / splashAASize;
    *yMaxA = yMax / splashAASize;
}

bool SplashXPathScanner::test(int x, int y) const
{
    if (y < yMin || y > yMax) {
        return false;
    }
    const auto &line = allIntersections[y - yMin];
    int count = 0;
    for (unsigned int i = 0; i < line.size() && line[i].x0 <= x; ++i) {
        if (x <= line[i].x1) {
            return true;
        }
        count += line[i].count;
    }
    return eo ? (count & 1) : (count != 0);
}

bool SplashXPathScanner::testSpan(int x0, int x1, int y) const
{
    unsigned int i;

    if (y < yMin || y > yMax) {
        return false;
    }
    const auto &line = allIntersections[y - yMin];
    int count = 0;
    for (i = 0; i < line.size() && line[i].x1 < x0; ++i) {
        count += line[i].count;
    }

    // invariant: the subspan [x0,xx1] is inside the path
    int xx1 = x0 - 1;
    int eoMask = eo ? 0x1 : ~0;
    while (xx1 < x1) {
        if (i >= line.size()) {
            return false;
        }
        if (line[i].x0 > xx1 + 1 && !(count & eoMask)) {
            return false;
        }
        if (line[i].x1 > xx1) {
            xx1 = line[i].x1;
        }
        count += line[i].count;
        ++i;
    }

    return true;
}

bool SplashXPathScanIterator::getNextSpan(int *x0, int *x1)
{
    int xx0, xx1;

    if (interIdx >= line.size()) {
        return false;
    }
    int eoMask = eo ? 0x1 : ~0;
    xx0 = line[interIdx].x0;
    xx1 = line[interIdx].x1;
    interCount += line[interIdx].count;
    ++interIdx;
    while (interIdx < line.size() && (line[interIdx].x0 <= xx1 || (interCount & eoMask))) {
        if (line[interIdx].x1 > xx1) {
            xx1 = line[interIdx].x1;
        }
        interCount += line[interIdx].count;
        ++interIdx;
    }
    *x0 = xx0;
    *x1 = xx1;
    return true;
}

SplashXPathScanIterator::SplashXPathScanIterator(const SplashXPathScanner &scanner, int y) : line((y < scanner.yMin || y > scanner.yMax) ? scanner.allIntersections[0] : scanner.allIntersections[y - scanner.yMin]), eo(scanner.eo)
{
    if (y < scanner.yMin || y > scanner.yMax) {
        // set index to line end
        interIdx = line.size();
    }
}

void SplashXPathScanner::computeIntersections(const SplashXPath &xPath)
{
    // build the list of all intersections
    allIntersections.resize(yMax - yMin + 1);

    const SplashCoord clipYMinFP = yMin;
    const SplashCoord clipYMaxFP = yMax + 1.0;

    for (int i = 0; i < xPath.length; ++i) {
        const SplashXPathSeg *seg = &xPath.segs[i];
        const SplashCoord segYMin = seg->y0;
        const SplashCoord segYMax = seg->y1;
        if (segYMin >= clipYMaxFP) {
            continue;
        }
        if (segYMax < clipYMinFP) {
            continue;
        }

        int y1 = splashFloor(segYMax);
        int y0 = (seg->flags & splashXPathHoriz) ? y1 : splashFloor(segYMin);

        if (y1 == y0) {
            addIntersection(segYMin, y1, splashFloor(seg->x0), splashFloor(seg->x1), 0);
        } else if (seg->flags & splashXPathVert) {
            if (y0 < yMin) {
                y0 = yMin;
            }
            if (y1 > yMax) {
                y1 = yMax;
            }
            int x = splashFloor(seg->x0);
            int count = (seg->flags & splashXPathFlipped) ? 1 : -1;
            for (int y = y0; y <= y1; ++y) {
                addIntersection(segYMin, y, x, x, count);
            }
        } else {
            SplashCoord segXMin, segXMax;

            if (seg->x0 < seg->x1) {
                segXMin = seg->x0;
                segXMax = seg->x1;
            } else {
                segXMin = seg->x1;
                segXMax = seg->x0;
            }
            // Calculate the projected intersection of the segment with the
            // X-Axis.
            SplashCoord xbase = seg->x0 - (seg->y0 * seg->dxdy);
            SplashCoord xx0 = seg->x0;
            if (y0 < yMin) {
                y0 = yMin;
                xx0 = xbase + ((SplashCoord)y0) * seg->dxdy;
            }
            if (y1 > yMax) {
                y1 = yMax;
            }
            int count = (seg->flags & splashXPathFlipped) ? 1 : -1;
            // the segment may not actually extend to the top and/or bottom edges
            if (xx0 < segXMin) {
                xx0 = segXMin;
            } else if (xx0 > segXMax) {
                xx0 = segXMax;
            }
            int x0 = splashFloor(xx0);

            for (int y = y0; y <= y1; ++y) {
                SplashCoord xx1 = xbase + ((SplashCoord)(y + 1) * seg->dxdy);

                if (xx1 < segXMin) {
                    xx1 = segXMin;
                } else if (xx1 > segXMax) {
                    xx1 = segXMax;
                }
                int x1 = splashFloor(xx1);
                addIntersection(segYMin, y, x0, x1, count);

                x0 = x1;
            }
        }
    }
    for (auto &line : allIntersections) {
        std::ranges::sort(line, [](const SplashIntersect i0, const SplashIntersect i1) { return i0.x0 < i1.x0; });
    }
}

inline void SplashXPathScanner::addIntersection(double segYMin, int y, int x0, int x1, int count)
{
    SplashIntersect intersect;
    if (x0 < x1) {
        intersect.x0 = x0;
        intersect.x1 = x1;
    } else {
        intersect.x0 = x1;
        intersect.x1 = x0;
    }
    if (segYMin < y) {
        intersect.count = count;
    } else {
        intersect.count = 0;
    }

    auto &line = allIntersections[y - yMin];
    if (line.empty()) {
#if !USE_BOOST_HEADERS
        line.reserve(4);
#endif
        line.push_back(intersect);
    } else {
        auto &last = line.back();
        // Check if last and new overlap/touch
        if ((last.x1 + 1) < intersect.x0) {
            line.push_back(intersect);
        } else if (last.x0 > (intersect.x1 + 1)) {
            line.push_back(intersect);
        } else {
            last.count += intersect.count;
            last.x0 = last.x0 < intersect.x0 ? last.x0 : intersect.x0;
            last.x1 = last.x1 > intersect.x1 ? last.x1 : intersect.x1;
        }
    }
}

void SplashXPathScanner::renderAALine(SplashBitmap *aaBuf, int *x0, int *x1, int y, bool adjustVertLine) const
{
    memset(aaBuf->getDataPtr(), 0, aaBuf->getRowSize() * aaBuf->getHeight());
    int xxMin = aaBuf->getWidth();
    int xxMax = -1;
    if (yMin <= yMax) {
        int yy = 0;
        int yyMax = splashAASize - 1;
        // clamp start and end position
        if (yMin > splashAASize * y) {
            yy = yMin - splashAASize * y;
        }
        if (yyMax + splashAASize * y > yMax) {
            yyMax = yMax - splashAASize * y;
        }

        int eoMask = eo ? 0x1 : ~0;
        for (; yy <= yyMax; ++yy) {
            const auto &line = allIntersections[splashAASize * y + yy - yMin];
            size_t interIdx = 0;
            int interCount = 0;
            while (interIdx < line.size()) {
                int xx0 = line[interIdx].x0;
                int xx1 = line[interIdx].x1;
                interCount += line[interIdx].count;
                ++interIdx;
                while (interIdx < line.size() && (line[interIdx].x0 <= xx1 || (interCount & eoMask))) {
                    if (line[interIdx].x1 > xx1) {
                        xx1 = line[interIdx].x1;
                    }
                    interCount += line[interIdx].count;
                    ++interIdx;
                }
                if (xx0 < 0) {
                    xx0 = 0;
                }
                ++xx1;
                if (xx1 > aaBuf->getWidth()) {
                    xx1 = aaBuf->getWidth();
                }
                // set [xx0, xx1) to 1
                if (xx0 < xx1) {
                    int xx = xx0;
                    SplashColorPtr p = aaBuf->getDataPtr() + yy * aaBuf->getRowSize() + (xx >> 3);
                    if (xx & 7) {
                        unsigned char mask = adjustVertLine ? 0xff : 0xff >> (xx & 7);
                        if (!adjustVertLine && (xx & ~7) == (xx1 & ~7)) {
                            mask &= (unsigned char)(0xff00 >> (xx1 & 7));
                        }
                        *p++ |= mask;
                        xx = (xx & ~7) + 8;
                    }
                    for (; xx + 7 < xx1; xx += 8) {
                        *p++ |= 0xff;
                    }
                    if (xx < xx1) {
                        *p |= adjustVertLine ? 0xff : (unsigned char)(0xff00 >> (xx1 & 7));
                    }
                }
                if (xx0 < xxMin) {
                    xxMin = xx0;
                }
                if (xx1 > xxMax) {
                    xxMax = xx1;
                }
            }
        }
    }
    if (xxMin > xxMax) {
        xxMin = xxMax;
    }
    *x0 = xxMin / splashAASize;
    *x1 = (xxMax - 1) / splashAASize;
}

void SplashXPathScanner::clipAALine(SplashBitmap *aaBuf, const int *x0, const int *x1, int y) const
{
    int yyMin = 0;
    int yyMax = splashAASize - 1;
    // clamp start and end position
    if (yMin > splashAASize * y) {
        yyMin = yMin - splashAASize * y;
    }
    if (yyMax + splashAASize * y > yMax) {
        yyMax = yMax - splashAASize * y;
    }
    int eoMask = eo ? 0x1 : ~0;
    for (int yy = 0; yy < splashAASize; ++yy) {
        int xx = *x0 * splashAASize;
        if (yy >= yyMin && yy <= yyMax) {
            const int intersectionIndex = splashAASize * y + yy - yMin;
            if (unlikely(intersectionIndex < 0 || (unsigned)intersectionIndex >= allIntersections.size())) {
                break;
            }
            const auto &line = allIntersections[intersectionIndex];
            size_t interIdx = 0;
            int interCount = 0;
            while (interIdx < line.size() && xx < (*x1 + 1) * splashAASize) {
                int xx0 = line[interIdx].x0;
                int xx1 = line[interIdx].x1;
                interCount += line[interIdx].count;
                ++interIdx;
                while (interIdx < line.size() && (line[interIdx].x0 <= xx1 || (interCount & eoMask))) {
                    if (line[interIdx].x1 > xx1) {
                        xx1 = line[interIdx].x1;
                    }
                    interCount += line[interIdx].count;
                    ++interIdx;
                }
                if (xx0 > aaBuf->getWidth()) {
                    xx0 = aaBuf->getWidth();
                }
                // set [xx, xx0) to 0
                if (xx < xx0) {
                    SplashColorPtr p = aaBuf->getDataPtr() + yy * aaBuf->getRowSize() + (xx >> 3);
                    if (xx & 7) {
                        auto mask = (unsigned char)(0xff00 >> (xx & 7));
                        if ((xx & ~7) == (xx0 & ~7)) {
                            mask |= 0xff >> (xx0 & 7);
                        }
                        *p++ &= mask;
                        xx = (xx & ~7) + 8;
                    }
                    for (; xx + 7 < xx0; xx += 8) {
                        *p++ = 0x00;
                    }
                    if (xx < xx0) {
                        *p &= 0xff >> (xx0 & 7);
                    }
                }
                if (xx1 >= xx) {
                    xx = xx1 + 1;
                }
            }
        }
        int xx0 = (*x1 + 1) * splashAASize;
        if (xx0 > aaBuf->getWidth()) {
            xx0 = aaBuf->getWidth();
        }
        // set [xx, xx0) to 0
        if (xx < xx0 && xx >= 0) {
            SplashColorPtr p = aaBuf->getDataPtr() + yy * aaBuf->getRowSize() + (xx >> 3);
            if (xx & 7) {
                auto mask = (unsigned char)(0xff00 >> (xx & 7));
                if ((xx & ~7) == (xx0 & ~7)) {
                    mask &= 0xff >> (xx0 & 7);
                }
                *p++ &= mask;
                xx = (xx & ~7) + 8;
            }
            for (; xx + 7 < xx0; xx += 8) {
                *p++ = 0x00;
            }
            if (xx < xx0) {
                *p &= 0xff >> (xx0 & 7);
            }
        }
    }
}
