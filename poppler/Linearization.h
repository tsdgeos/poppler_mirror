//========================================================================
//
// Linearization.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
//
//========================================================================

#ifndef LINEARIZATION_H
#define LINEARIZATION_H

#include "Object.h"
class BaseStream;

//------------------------------------------------------------------------
// Linearization
//------------------------------------------------------------------------

class Linearization {
public:

  Linearization(BaseStream *str);
  ~Linearization();

  unsigned int getLength();
  unsigned int getHintsOffset();
  unsigned int getHintsLength();
  unsigned int getHintsOffset2();
  unsigned int getHintsLength2();
  int getObjectNumberFirst();
  unsigned int getEndFirst();
  int getNumPages();
  unsigned int getMainXRefEntriesOffset();
  int getPageFirst();

private:

  Object linDict;

};

#endif
