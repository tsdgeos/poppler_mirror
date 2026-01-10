//========================================================================
//
// CharTypes.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef CHARTYPES_H
#define CHARTYPES_H

// Unicode character.
using Unicode = unsigned int;

// Character ID for CID character collections.
using CID = unsigned int;

// This is large enough to hold any of the following:
// - 8-bit char code
// - 16-bit CID
// - Unicode
using CharCode = unsigned int;

#endif
