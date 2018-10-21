//========================================================================
//
// ViewerPreferences.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2011 Pino Toscano <pino@kde.org>
//
//========================================================================

#ifndef VIEWERPREFERENCES_H
#define VIEWERPREFERENCES_H

#include "goo/gtypes.h"

class Dict;

//------------------------------------------------------------------------
// ViewerPreferences
//------------------------------------------------------------------------

class ViewerPreferences {
public:

  enum NonFullScreenPageMode {
    nfpmUseNone,
    nfpmUseOutlines,
    nfpmUseThumbs,
    nfpmUseOC
  };
  enum Direction {
    directionL2R,
    directionR2L
  };
  enum PrintScaling {
    printScalingNone,
    printScalingAppDefault
  };
  enum Duplex {
    duplexNone,
    duplexSimplex,
    duplexDuplexFlipShortEdge,
    duplexDuplexFlipLongEdge
  };

  ViewerPreferences(Dict *prefDict);
  ~ViewerPreferences();

  bool getHideToolbar() const { return hideToolbar; }
  bool getHideMenubar() const { return hideMenubar; }
  bool getHideWindowUI() const { return hideWindowUI; }
  bool getFitWindow() const { return fitWindow; }
  bool getCenterWindow() const { return centerWindow; }
  bool getDisplayDocTitle() const { return displayDocTitle; }
  NonFullScreenPageMode getNonFullScreenPageMode() const { return nonFullScreenPageMode; }
  Direction getDirection() const { return direction; }
  PrintScaling getPrintScaling() const { return printScaling; }
  Duplex getDuplex() const { return duplex; }

private:

  void init();

  bool hideToolbar;
  bool hideMenubar;
  bool hideWindowUI;
  bool fitWindow;
  bool centerWindow;
  bool displayDocTitle;
  NonFullScreenPageMode nonFullScreenPageMode;
  Direction direction;
  PrintScaling printScaling;
  Duplex duplex;
};

#endif
