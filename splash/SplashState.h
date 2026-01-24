//========================================================================
//
// SplashState.h
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2011, 2012, 2015 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018, 2021, 2022, 2024-2026 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef SPLASHSTATE_H
#define SPLASHSTATE_H

#include "SplashTypes.h"

#include <array>
#include <vector>

class SplashPattern;
class SplashScreen;
class SplashClip;
class SplashBitmap;

//------------------------------------------------------------------------
// SplashState
//------------------------------------------------------------------------

class SplashState
{
public:
    // Create a new state object, initialized with default settings.
    SplashState(int width, int height, bool vectorAntialias, SplashScreenParams *screenParams);
    SplashState(int width, int height, bool vectorAntialias, const SplashScreen &screenA);

    // Copy a state object.
    SplashState *copy() const { return new SplashState(this); }

    ~SplashState();

    SplashState(const SplashState &) = delete;
    SplashState &operator=(const SplashState &) = delete;

    // Set the stroke pattern.  This does not copy <strokePatternA>.
    void setStrokePattern(SplashPattern *strokePatternA);

    // Set the fill pattern.  This does not copy <fillPatternA>.
    void setFillPattern(SplashPattern *fillPatternA);

    // Set the line dash pattern.
    void setLineDash(std::vector<SplashCoord> &&lineDashA, SplashCoord lineDashPhaseA);

    // Set the soft mask bitmap.
    void setSoftMask(SplashBitmap *softMaskA);

    // Set the overprint parametes.
    void setFillOverprint(bool fillOverprintA) { fillOverprint = fillOverprintA; }
    void setStrokeOverprint(bool strokeOverprintA) { strokeOverprint = strokeOverprintA; }
    void setOverprintMode(int overprintModeA) { overprintMode = overprintModeA; }

    // Set the transfer function.
    void setTransfer(unsigned char *red, unsigned char *green, unsigned char *blue, unsigned char *gray);

private:
    explicit SplashState(const SplashState *state);

    std::array<SplashCoord, 6> matrix;
    SplashPattern *strokePattern;
    SplashPattern *fillPattern;
    SplashScreen *screen;
    SplashBlendFunc blendFunc;
    SplashCoord strokeAlpha;
    SplashCoord fillAlpha;
    bool multiplyPatternAlpha;
    SplashCoord patternStrokeAlpha;
    SplashCoord patternFillAlpha;
    SplashCoord lineWidth;
    SplashLineCap lineCap;
    SplashLineJoin lineJoin;
    SplashCoord miterLimit;
    SplashCoord flatness;
    std::vector<SplashCoord> lineDash;
    SplashCoord lineDashPhase;
    bool strokeAdjust;
    std::unique_ptr<SplashClip> clip;
    SplashBitmap *softMask;
    bool deleteSoftMask;
    bool inNonIsolatedGroup;
    bool fillOverprint;
    bool strokeOverprint;
    int overprintMode;
    unsigned char rgbTransferR[256], rgbTransferG[256], rgbTransferB[256];
    unsigned char grayTransfer[256];
    unsigned char cmykTransferC[256], cmykTransferM[256], cmykTransferY[256], cmykTransferK[256];
    unsigned char deviceNTransfer[SPOT_NCOMPS + 4][256];
    unsigned int overprintMask;
    bool overprintAdditive;

    SplashState *next; // used by Splash class

    friend class Splash;
};

#endif
