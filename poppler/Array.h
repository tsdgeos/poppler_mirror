//========================================================================
//
// Array.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2017, 2018 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef ARRAY_H
#define ARRAY_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <atomic>
#include <mutex>
#include <vector>

#include "poppler-config.h"
#include "Object.h"

class XRef;

//------------------------------------------------------------------------
// Array
//------------------------------------------------------------------------

class Array {
public:

  // Constructor.
  Array(XRef *xrefA);

  // Destructor.
  ~Array();

  Array(const Array &) = delete;
  Array& operator=(const Array &) = delete;

  // Get number of elements.
  int getLength() const { return elems.size(); }

  // Copy array with new xref
  Object copy(XRef *xrefA) const;

  // Add an element
  // elem becomes a dead object after this call
  void add(Object &&elem);

  // Remove an element by position
  void remove(int i);

  // Accessors.
  Object get(int i, int resursion = 0) const;
  Object getNF(int i) const;
  GBool getString(int i, GooString *string) const;

private:
  friend class Object; // for incRef/decRef

  // Reference counting.
  int incRef() { return ++ref; }
  int decRef() { return --ref; }

  XRef *xref;			// the xref table for this PDF file
  std::vector<Object> elems;		// array of elements
  std::atomic_int ref;			// reference count
  mutable std::recursive_mutex mutex;
};

#endif
