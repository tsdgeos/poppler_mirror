/*
 * gtypes.h
 *
 * Some useful simple types.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2010 Patrick Spendrin <ps_ml@gmx.de>
// Copyright (C) 2010 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef GTYPES_H
#define GTYPES_H

#include "config.h"

/*
 * These have stupid names to avoid conflicts with some (but not all)
 * C++ compilers which define them.
 */
typedef bool GBool;
#define gTrue true
#define gFalse false

#ifdef _MSC_VER
#pragma warning(disable: 4800) /* 'type' : forcing value to bool 'true' or 'false' (performance warning) */
#endif

/*
 * These have stupid names to avoid conflicts with <sys/types.h>,
 * which on various systems defines some random subset of these.
 */
typedef unsigned char Guchar;
typedef unsigned short Gushort;
typedef unsigned int Guint;
typedef unsigned long Gulong;

/*
 * Define precise integer types.
 */
#if HAVE_STDINT_H
#include <stdint.h>
#elif _MSC_VER
typedef signed   __int8  int8_t;
typedef unsigned __int8  uint8_t;
typedef signed   __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed   __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed   __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

#endif
