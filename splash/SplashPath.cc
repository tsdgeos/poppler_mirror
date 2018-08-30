//========================================================================
//
// SplashPath.cc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2018 Stefan Brüns <stefan.bruens@rwth-aachen.de>
// Copyright (C) 2018 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "goo/gmem.h"
#include "goo/GooLikely.h"
#include "SplashErrorCodes.h"
#include "SplashPath.h"

//------------------------------------------------------------------------
// SplashPath
//------------------------------------------------------------------------

// A path can be in three possible states:
//
// 1. no current point -- zero or more finished subpaths
//    [curSubpath == length]
//
// 2. one point in subpath
//    [curSubpath == length - 1]
//
// 3. open subpath with two or more points
//    [curSubpath < length - 1]

SplashPath::SplashPath() {
  pts = nullptr;
  flags = nullptr;
  length = size = 0;
  curSubpath = 0;
  hints = nullptr;
  hintsLength = hintsSize = 0;
}

SplashPath::SplashPath(SplashPath *path) {
  length = path->length;
  size = path->size;
  pts = (SplashPathPoint *)gmallocn(size, sizeof(SplashPathPoint));
  flags = (Guchar *)gmallocn(size, sizeof(Guchar));
  memcpy(pts, path->pts, length * sizeof(SplashPathPoint));
  memcpy(flags, path->flags, length * sizeof(Guchar));
  curSubpath = path->curSubpath;
  if (path->hints) {
    hintsLength = hintsSize = path->hintsLength;
    hints = (SplashPathHint *)gmallocn(hintsSize, sizeof(SplashPathHint));
    memcpy(hints, path->hints, hintsLength * sizeof(SplashPathHint));
  } else {
    hints = nullptr;
  }
}

SplashPath::~SplashPath() {
  gfree(pts);
  gfree(flags);
  gfree(hints);
}

void  SplashPath::reserve(int nPts) {
  grow(nPts - size);
}

// Add space for <nPts> more points.
void SplashPath::grow(int nPts) {
  if (length + nPts > size) {
    if (size == 0) {
      size = 32;
    }
    while (size < length + nPts) {
      size *= 2;
    }
    pts = (SplashPathPoint *)greallocn_checkoverflow(pts, size, sizeof(SplashPathPoint));
    flags = (Guchar *)greallocn_checkoverflow(flags, size, sizeof(Guchar));
    if (unlikely(!pts || !flags)) {
      length = size = 0;
    }
  }
}

void SplashPath::append(SplashPath *path) {
  int i;

  curSubpath = length + path->curSubpath;
  grow(path->length);
  for (i = 0; i < path->length; ++i) {
    pts[length] = path->pts[i];
    flags[length] = path->flags[i];
    ++length;
  }
}

SplashError SplashPath::moveTo(SplashCoord x, SplashCoord y) {
  if (onePointSubpath()) {
    return splashErrBogusPath;
  }
  grow(1);
  if (unlikely(length == 0))
    return splashErrBogusPath;
  pts[length].x = x;
  pts[length].y = y;
  flags[length] = splashPathFirst | splashPathLast;
  curSubpath = length++;
  return splashOk;
}

SplashError SplashPath::lineTo(SplashCoord x, SplashCoord y) {
  if (noCurrentPoint()) {
    return splashErrNoCurPt;
  }
  flags[length-1] &= ~splashPathLast;
  grow(1);
  pts[length].x = x;
  pts[length].y = y;
  flags[length] = splashPathLast;
  ++length;
  return splashOk;
}

SplashError SplashPath::curveTo(SplashCoord x1, SplashCoord y1,
				SplashCoord x2, SplashCoord y2,
				SplashCoord x3, SplashCoord y3) {
  if (noCurrentPoint()) {
    return splashErrNoCurPt;
  }
  flags[length-1] &= ~splashPathLast;
  grow(3);
  pts[length].x = x1;
  pts[length].y = y1;
  flags[length] = splashPathCurve;
  ++length;
  pts[length].x = x2;
  pts[length].y = y2;
  flags[length] = splashPathCurve;
  ++length;
  pts[length].x = x3;
  pts[length].y = y3;
  flags[length] = splashPathLast;
  ++length;
  return splashOk;
}

SplashError SplashPath::close(GBool force) {
  if (noCurrentPoint()) {
    return splashErrNoCurPt;
  }
  if (force ||
      curSubpath == length - 1 ||
      pts[length - 1].x != pts[curSubpath].x ||
      pts[length - 1].y != pts[curSubpath].y) {
    lineTo(pts[curSubpath].x, pts[curSubpath].y);
  }
  flags[curSubpath] |= splashPathClosed;
  flags[length - 1] |= splashPathClosed;
  curSubpath = length;
  return splashOk;
}

void SplashPath::addStrokeAdjustHint(int ctrl0, int ctrl1,
				     int firstPt, int lastPt) {
  if (hintsLength == hintsSize) {
    hintsSize = hintsLength ? 2 * hintsLength : 8;
    hints = (SplashPathHint *)greallocn(hints, hintsSize,
					sizeof(SplashPathHint));
  }
  hints[hintsLength].ctrl0 = ctrl0;
  hints[hintsLength].ctrl1 = ctrl1;
  hints[hintsLength].firstPt = firstPt;
  hints[hintsLength].lastPt = lastPt;
  ++hintsLength;
}

void SplashPath::offset(SplashCoord dx, SplashCoord dy) {
  int i;

  for (i = 0; i < length; ++i) {
    pts[i].x += dx;
    pts[i].y += dy;
  }
}

GBool SplashPath::getCurPt(SplashCoord *x, SplashCoord *y) {
  if (noCurrentPoint()) {
    return gFalse;
  }
  *x = pts[length - 1].x;
  *y = pts[length - 1].y;
  return gTrue;
}
