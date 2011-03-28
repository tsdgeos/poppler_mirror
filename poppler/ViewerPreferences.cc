//========================================================================
//
// ViewerPreferences.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2011 Pino Toscano <pino@kde.org>
//
//========================================================================

#include <config.h>

#include "ViewerPreferences.h"

#include "Object.h"
#include "Dict.h"

ViewerPreferences::ViewerPreferences(Dict *prefDict)
{
  init();

  if (!prefDict) {
    return;
  }

  Object obj;

  if (prefDict->lookup("HideToolbar", &obj)->isBool()) {
    hideToolbar = obj.getBool();
  }
  obj.free();

  if (prefDict->lookup("HideMenubar", &obj)->isBool()) {
    hideMenubar = obj.getBool();
  }
  obj.free();

  if (prefDict->lookup("HideWindowUI", &obj)->isBool()) {
    hideWindowUI = obj.getBool();
  }
  obj.free();

  if (prefDict->lookup("FitWindow", &obj)->isBool()) {
    fitWindow = obj.getBool();
  }
  obj.free();

  if (prefDict->lookup("CenterWindow", &obj)->isBool()) {
    centerWindow = obj.getBool();
  }
  obj.free();

  if (prefDict->lookup("DisplayDocTitle", &obj)->isBool()) {
    centerWindow = obj.getBool();
  }
  obj.free();
}

ViewerPreferences::~ViewerPreferences()
{
}

void ViewerPreferences::init()
{
  hideToolbar = gFalse;
  hideMenubar = gFalse;
  hideWindowUI = gFalse;
  fitWindow = gFalse;
  centerWindow = gFalse;
  displayDocTitle = gFalse;
}
