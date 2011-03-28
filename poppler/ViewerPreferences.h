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
// Hints
//------------------------------------------------------------------------

class ViewerPreferences {
public:

  ViewerPreferences(Dict *prefDict);
  ~ViewerPreferences();

  GBool getHideToolbar() const { return hideToolbar; }
  GBool getHideMenubar() const { return hideMenubar; }
  GBool getHideWindowUI() const { return hideWindowUI; }
  GBool getFitWindow() const { return fitWindow; }
  GBool getCenterWindow() const { return centerWindow; }
  GBool getDisplayDocTitle() const { return displayDocTitle; }

private:

  void init();

  GBool hideToolbar;
  GBool hideMenubar;
  GBool hideWindowUI;
  GBool fitWindow;
  GBool centerWindow;
  GBool displayDocTitle;
};

#endif
