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

private:

  void init();

};

#endif
