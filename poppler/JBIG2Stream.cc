//========================================================================
//
// JBIG2Stream.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Raj Kumar <rkumar@archive.org>
// Copyright (C) 2006 Paul Walmsley <paul@booyaka.com>
// Copyright (C) 2006-2010, 2012, 2014-2022, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 David Benjamin <davidben@mit.edu>
// Copyright (C) 2011 Edward Jiang <ejiang@google.com>
// Copyright (C) 2012 William Bader <williambader@hotmail.com>
// Copyright (C) 2012 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013, 2014 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2015 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 LE GARREC Vincent <legarrec.vincent@gmail.com>
// Copyright (C) 2019-2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2019-2021 Even Rouault <even.rouault@spatialys.com>
// Copyright (C) 2024, 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <memory>

#include <cstdlib>
#include <climits>
#include "Error.h"
#include "JArithmeticDecoder.h"
#include "JBIG2Stream.h"

//~ share these tables
#include "Stream-CCITT.h"

#include "goo/gmem.h"

//------------------------------------------------------------------------

static const int contextSize[4] = { 16, 13, 10, 10 };
static const int refContextSize[2] = { 13, 10 };
static constexpr auto intNBits = sizeof(int) * CHAR_BIT;

//------------------------------------------------------------------------
// JBIG2HuffmanTable
//------------------------------------------------------------------------

#define jbig2HuffmanLOW 0xfffffffd
#define jbig2HuffmanOOB 0xfffffffe
#define jbig2HuffmanEOT 0xffffffff

struct JBIG2HuffmanTable
{
    int val;
    unsigned int prefixLen;
    unsigned int rangeLen; // can also be LOW, OOB, or EOT
    unsigned int prefix;
};

static const JBIG2HuffmanTable huffTableA[] = { { .val = 0, .prefixLen = 1, .rangeLen = 4, .prefix = 0x000 },
                                                { .val = 16, .prefixLen = 2, .rangeLen = 8, .prefix = 0x002 },
                                                { .val = 272, .prefixLen = 3, .rangeLen = 16, .prefix = 0x006 },
                                                { .val = 65808, .prefixLen = 3, .rangeLen = 32, .prefix = 0x007 },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableB[] = { { .val = 0, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },
                                                { .val = 1, .prefixLen = 2, .rangeLen = 0, .prefix = 0x002 },
                                                { .val = 2, .prefixLen = 3, .rangeLen = 0, .prefix = 0x006 },
                                                { .val = 3, .prefixLen = 4, .rangeLen = 3, .prefix = 0x00e },
                                                { .val = 11, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01e },
                                                { .val = 75, .prefixLen = 6, .rangeLen = 32, .prefix = 0x03e },
                                                { .val = 0, .prefixLen = 6, .rangeLen = jbig2HuffmanOOB, .prefix = 0x03f },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableC[] = {
    { .val = 0, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },          { .val = 1, .prefixLen = 2, .rangeLen = 0, .prefix = 0x002 },    { .val = 2, .prefixLen = 3, .rangeLen = 0, .prefix = 0x006 },
    { .val = 3, .prefixLen = 4, .rangeLen = 3, .prefix = 0x00e },          { .val = 11, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01e },   { .val = 0, .prefixLen = 6, .rangeLen = jbig2HuffmanOOB, .prefix = 0x03e },
    { .val = 75, .prefixLen = 7, .rangeLen = 32, .prefix = 0x0fe },        { .val = -256, .prefixLen = 8, .rangeLen = 8, .prefix = 0x0fe }, { .val = -257, .prefixLen = 8, .rangeLen = jbig2HuffmanLOW, .prefix = 0x0ff },
    { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 }
};

static const JBIG2HuffmanTable huffTableD[] = { { .val = 1, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },          { .val = 2, .prefixLen = 2, .rangeLen = 0, .prefix = 0x002 },
                                                { .val = 3, .prefixLen = 3, .rangeLen = 0, .prefix = 0x006 },          { .val = 4, .prefixLen = 4, .rangeLen = 3, .prefix = 0x00e },
                                                { .val = 12, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01e },         { .val = 76, .prefixLen = 5, .rangeLen = 32, .prefix = 0x01f },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableE[] = { { .val = 1, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },          { .val = 2, .prefixLen = 2, .rangeLen = 0, .prefix = 0x002 },
                                                { .val = 3, .prefixLen = 3, .rangeLen = 0, .prefix = 0x006 },          { .val = 4, .prefixLen = 4, .rangeLen = 3, .prefix = 0x00e },
                                                { .val = 12, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01e },         { .val = 76, .prefixLen = 6, .rangeLen = 32, .prefix = 0x03e },
                                                { .val = -255, .prefixLen = 7, .rangeLen = 8, .prefix = 0x07e },       { .val = -256, .prefixLen = 7, .rangeLen = jbig2HuffmanLOW, .prefix = 0x07f },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableF[] = { { .val = 0, .prefixLen = 2, .rangeLen = 7, .prefix = 0x000 },
                                                { .val = 128, .prefixLen = 3, .rangeLen = 7, .prefix = 0x002 },
                                                { .val = 256, .prefixLen = 3, .rangeLen = 8, .prefix = 0x003 },
                                                { .val = -1024, .prefixLen = 4, .rangeLen = 9, .prefix = 0x008 },
                                                { .val = -512, .prefixLen = 4, .rangeLen = 8, .prefix = 0x009 },
                                                { .val = -256, .prefixLen = 4, .rangeLen = 7, .prefix = 0x00a },
                                                { .val = -32, .prefixLen = 4, .rangeLen = 5, .prefix = 0x00b },
                                                { .val = 512, .prefixLen = 4, .rangeLen = 9, .prefix = 0x00c },
                                                { .val = 1024, .prefixLen = 4, .rangeLen = 10, .prefix = 0x00d },
                                                { .val = -2048, .prefixLen = 5, .rangeLen = 10, .prefix = 0x01c },
                                                { .val = -128, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01d },
                                                { .val = -64, .prefixLen = 5, .rangeLen = 5, .prefix = 0x01e },
                                                { .val = -2049, .prefixLen = 6, .rangeLen = jbig2HuffmanLOW, .prefix = 0x03e },
                                                { .val = 2048, .prefixLen = 6, .rangeLen = 32, .prefix = 0x03f },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableG[] = { { .val = -512, .prefixLen = 3, .rangeLen = 8, .prefix = 0x000 },  { .val = 256, .prefixLen = 3, .rangeLen = 8, .prefix = 0x001 },
                                                { .val = 512, .prefixLen = 3, .rangeLen = 9, .prefix = 0x002 },   { .val = 1024, .prefixLen = 3, .rangeLen = 10, .prefix = 0x003 },
                                                { .val = -1024, .prefixLen = 4, .rangeLen = 9, .prefix = 0x008 }, { .val = -256, .prefixLen = 4, .rangeLen = 7, .prefix = 0x009 },
                                                { .val = -32, .prefixLen = 4, .rangeLen = 5, .prefix = 0x00a },   { .val = 0, .prefixLen = 4, .rangeLen = 5, .prefix = 0x00b },
                                                { .val = 128, .prefixLen = 4, .rangeLen = 7, .prefix = 0x00c },   { .val = -128, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01a },
                                                { .val = -64, .prefixLen = 5, .rangeLen = 5, .prefix = 0x01b },   { .val = 32, .prefixLen = 5, .rangeLen = 5, .prefix = 0x01c },
                                                { .val = 64, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01d },    { .val = -1025, .prefixLen = 5, .rangeLen = jbig2HuffmanLOW, .prefix = 0x01e },
                                                { .val = 2048, .prefixLen = 5, .rangeLen = 32, .prefix = 0x01f }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableH[] = { { .val = 0, .prefixLen = 2, .rangeLen = 1, .prefix = 0x000 },     { .val = 0, .prefixLen = 2, .rangeLen = jbig2HuffmanOOB, .prefix = 0x001 },
                                                { .val = 4, .prefixLen = 3, .rangeLen = 4, .prefix = 0x004 },     { .val = -1, .prefixLen = 4, .rangeLen = 0, .prefix = 0x00a },
                                                { .val = 22, .prefixLen = 4, .rangeLen = 4, .prefix = 0x00b },    { .val = 38, .prefixLen = 4, .rangeLen = 5, .prefix = 0x00c },
                                                { .val = 2, .prefixLen = 5, .rangeLen = 0, .prefix = 0x01a },     { .val = 70, .prefixLen = 5, .rangeLen = 6, .prefix = 0x01b },
                                                { .val = 134, .prefixLen = 5, .rangeLen = 7, .prefix = 0x01c },   { .val = 3, .prefixLen = 6, .rangeLen = 0, .prefix = 0x03a },
                                                { .val = 20, .prefixLen = 6, .rangeLen = 1, .prefix = 0x03b },    { .val = 262, .prefixLen = 6, .rangeLen = 7, .prefix = 0x03c },
                                                { .val = 646, .prefixLen = 6, .rangeLen = 10, .prefix = 0x03d },  { .val = -2, .prefixLen = 7, .rangeLen = 0, .prefix = 0x07c },
                                                { .val = 390, .prefixLen = 7, .rangeLen = 8, .prefix = 0x07d },   { .val = -15, .prefixLen = 8, .rangeLen = 3, .prefix = 0x0fc },
                                                { .val = -5, .prefixLen = 8, .rangeLen = 1, .prefix = 0x0fd },    { .val = -7, .prefixLen = 9, .rangeLen = 1, .prefix = 0x1fc },
                                                { .val = -3, .prefixLen = 9, .rangeLen = 0, .prefix = 0x1fd },    { .val = -16, .prefixLen = 9, .rangeLen = jbig2HuffmanLOW, .prefix = 0x1fe },
                                                { .val = 1670, .prefixLen = 9, .rangeLen = 32, .prefix = 0x1ff }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableI[] = { { .val = 0, .prefixLen = 2, .rangeLen = jbig2HuffmanOOB, .prefix = 0x000 },
                                                { .val = -1, .prefixLen = 3, .rangeLen = 1, .prefix = 0x002 },
                                                { .val = 1, .prefixLen = 3, .rangeLen = 1, .prefix = 0x003 },
                                                { .val = 7, .prefixLen = 3, .rangeLen = 5, .prefix = 0x004 },
                                                { .val = -3, .prefixLen = 4, .rangeLen = 1, .prefix = 0x00a },
                                                { .val = 43, .prefixLen = 4, .rangeLen = 5, .prefix = 0x00b },
                                                { .val = 75, .prefixLen = 4, .rangeLen = 6, .prefix = 0x00c },
                                                { .val = 3, .prefixLen = 5, .rangeLen = 1, .prefix = 0x01a },
                                                { .val = 139, .prefixLen = 5, .rangeLen = 7, .prefix = 0x01b },
                                                { .val = 267, .prefixLen = 5, .rangeLen = 8, .prefix = 0x01c },
                                                { .val = 5, .prefixLen = 6, .rangeLen = 1, .prefix = 0x03a },
                                                { .val = 39, .prefixLen = 6, .rangeLen = 2, .prefix = 0x03b },
                                                { .val = 523, .prefixLen = 6, .rangeLen = 8, .prefix = 0x03c },
                                                { .val = 1291, .prefixLen = 6, .rangeLen = 11, .prefix = 0x03d },
                                                { .val = -5, .prefixLen = 7, .rangeLen = 1, .prefix = 0x07c },
                                                { .val = 779, .prefixLen = 7, .rangeLen = 9, .prefix = 0x07d },
                                                { .val = -31, .prefixLen = 8, .rangeLen = 4, .prefix = 0x0fc },
                                                { .val = -11, .prefixLen = 8, .rangeLen = 2, .prefix = 0x0fd },
                                                { .val = -15, .prefixLen = 9, .rangeLen = 2, .prefix = 0x1fc },
                                                { .val = -7, .prefixLen = 9, .rangeLen = 1, .prefix = 0x1fd },
                                                { .val = -32, .prefixLen = 9, .rangeLen = jbig2HuffmanLOW, .prefix = 0x1fe },
                                                { .val = 3339, .prefixLen = 9, .rangeLen = 32, .prefix = 0x1ff },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableJ[] = { { .val = -2, .prefixLen = 2, .rangeLen = 2, .prefix = 0x000 },
                                                { .val = 6, .prefixLen = 2, .rangeLen = 6, .prefix = 0x001 },
                                                { .val = 0, .prefixLen = 2, .rangeLen = jbig2HuffmanOOB, .prefix = 0x002 },
                                                { .val = -3, .prefixLen = 5, .rangeLen = 0, .prefix = 0x018 },
                                                { .val = 2, .prefixLen = 5, .rangeLen = 0, .prefix = 0x019 },
                                                { .val = 70, .prefixLen = 5, .rangeLen = 5, .prefix = 0x01a },
                                                { .val = 3, .prefixLen = 6, .rangeLen = 0, .prefix = 0x036 },
                                                { .val = 102, .prefixLen = 6, .rangeLen = 5, .prefix = 0x037 },
                                                { .val = 134, .prefixLen = 6, .rangeLen = 6, .prefix = 0x038 },
                                                { .val = 198, .prefixLen = 6, .rangeLen = 7, .prefix = 0x039 },
                                                { .val = 326, .prefixLen = 6, .rangeLen = 8, .prefix = 0x03a },
                                                { .val = 582, .prefixLen = 6, .rangeLen = 9, .prefix = 0x03b },
                                                { .val = 1094, .prefixLen = 6, .rangeLen = 10, .prefix = 0x03c },
                                                { .val = -21, .prefixLen = 7, .rangeLen = 4, .prefix = 0x07a },
                                                { .val = -4, .prefixLen = 7, .rangeLen = 0, .prefix = 0x07b },
                                                { .val = 4, .prefixLen = 7, .rangeLen = 0, .prefix = 0x07c },
                                                { .val = 2118, .prefixLen = 7, .rangeLen = 11, .prefix = 0x07d },
                                                { .val = -5, .prefixLen = 8, .rangeLen = 0, .prefix = 0x0fc },
                                                { .val = 5, .prefixLen = 8, .rangeLen = 0, .prefix = 0x0fd },
                                                { .val = -22, .prefixLen = 8, .rangeLen = jbig2HuffmanLOW, .prefix = 0x0fe },
                                                { .val = 4166, .prefixLen = 8, .rangeLen = 32, .prefix = 0x0ff },
                                                { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableK[] = { { .val = 1, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },    { .val = 2, .prefixLen = 2, .rangeLen = 1, .prefix = 0x002 },
                                                { .val = 4, .prefixLen = 4, .rangeLen = 0, .prefix = 0x00c },    { .val = 5, .prefixLen = 4, .rangeLen = 1, .prefix = 0x00d },
                                                { .val = 7, .prefixLen = 5, .rangeLen = 1, .prefix = 0x01c },    { .val = 9, .prefixLen = 5, .rangeLen = 2, .prefix = 0x01d },
                                                { .val = 13, .prefixLen = 6, .rangeLen = 2, .prefix = 0x03c },   { .val = 17, .prefixLen = 7, .rangeLen = 2, .prefix = 0x07a },
                                                { .val = 21, .prefixLen = 7, .rangeLen = 3, .prefix = 0x07b },   { .val = 29, .prefixLen = 7, .rangeLen = 4, .prefix = 0x07c },
                                                { .val = 45, .prefixLen = 7, .rangeLen = 5, .prefix = 0x07d },   { .val = 77, .prefixLen = 7, .rangeLen = 6, .prefix = 0x07e },
                                                { .val = 141, .prefixLen = 7, .rangeLen = 32, .prefix = 0x07f }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableL[] = { { .val = 1, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },   { .val = 2, .prefixLen = 2, .rangeLen = 0, .prefix = 0x002 },
                                                { .val = 3, .prefixLen = 3, .rangeLen = 1, .prefix = 0x006 },   { .val = 5, .prefixLen = 5, .rangeLen = 0, .prefix = 0x01c },
                                                { .val = 6, .prefixLen = 5, .rangeLen = 1, .prefix = 0x01d },   { .val = 8, .prefixLen = 6, .rangeLen = 1, .prefix = 0x03c },
                                                { .val = 10, .prefixLen = 7, .rangeLen = 0, .prefix = 0x07a },  { .val = 11, .prefixLen = 7, .rangeLen = 1, .prefix = 0x07b },
                                                { .val = 13, .prefixLen = 7, .rangeLen = 2, .prefix = 0x07c },  { .val = 17, .prefixLen = 7, .rangeLen = 3, .prefix = 0x07d },
                                                { .val = 25, .prefixLen = 7, .rangeLen = 4, .prefix = 0x07e },  { .val = 41, .prefixLen = 8, .rangeLen = 5, .prefix = 0x0fe },
                                                { .val = 73, .prefixLen = 8, .rangeLen = 32, .prefix = 0x0ff }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableM[] = { { .val = 1, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },    { .val = 2, .prefixLen = 3, .rangeLen = 0, .prefix = 0x004 },
                                                { .val = 7, .prefixLen = 3, .rangeLen = 3, .prefix = 0x005 },    { .val = 3, .prefixLen = 4, .rangeLen = 0, .prefix = 0x00c },
                                                { .val = 5, .prefixLen = 4, .rangeLen = 1, .prefix = 0x00d },    { .val = 4, .prefixLen = 5, .rangeLen = 0, .prefix = 0x01c },
                                                { .val = 15, .prefixLen = 6, .rangeLen = 1, .prefix = 0x03a },   { .val = 17, .prefixLen = 6, .rangeLen = 2, .prefix = 0x03b },
                                                { .val = 21, .prefixLen = 6, .rangeLen = 3, .prefix = 0x03c },   { .val = 29, .prefixLen = 6, .rangeLen = 4, .prefix = 0x03d },
                                                { .val = 45, .prefixLen = 6, .rangeLen = 5, .prefix = 0x03e },   { .val = 77, .prefixLen = 7, .rangeLen = 6, .prefix = 0x07e },
                                                { .val = 141, .prefixLen = 7, .rangeLen = 32, .prefix = 0x07f }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableN[] = { { .val = 0, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },  { .val = -2, .prefixLen = 3, .rangeLen = 0, .prefix = 0x004 },
                                                { .val = -1, .prefixLen = 3, .rangeLen = 0, .prefix = 0x005 }, { .val = 1, .prefixLen = 3, .rangeLen = 0, .prefix = 0x006 },
                                                { .val = 2, .prefixLen = 3, .rangeLen = 0, .prefix = 0x007 },  { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

static const JBIG2HuffmanTable huffTableO[] = { { .val = 0, .prefixLen = 1, .rangeLen = 0, .prefix = 0x000 },   { .val = -1, .prefixLen = 3, .rangeLen = 0, .prefix = 0x004 },
                                                { .val = 1, .prefixLen = 3, .rangeLen = 0, .prefix = 0x005 },   { .val = -2, .prefixLen = 4, .rangeLen = 0, .prefix = 0x00c },
                                                { .val = 2, .prefixLen = 4, .rangeLen = 0, .prefix = 0x00d },   { .val = -4, .prefixLen = 5, .rangeLen = 1, .prefix = 0x01c },
                                                { .val = 3, .prefixLen = 5, .rangeLen = 1, .prefix = 0x01d },   { .val = -8, .prefixLen = 6, .rangeLen = 2, .prefix = 0x03c },
                                                { .val = 5, .prefixLen = 6, .rangeLen = 2, .prefix = 0x03d },   { .val = -24, .prefixLen = 7, .rangeLen = 4, .prefix = 0x07c },
                                                { .val = 9, .prefixLen = 7, .rangeLen = 4, .prefix = 0x07d },   { .val = -25, .prefixLen = 7, .rangeLen = jbig2HuffmanLOW, .prefix = 0x07e },
                                                { .val = 25, .prefixLen = 7, .rangeLen = 32, .prefix = 0x07f }, { .val = 0, .prefixLen = 0, .rangeLen = jbig2HuffmanEOT, .prefix = 0 } };

//------------------------------------------------------------------------
// JBIG2HuffmanDecoder
//------------------------------------------------------------------------

class JBIG2HuffmanDecoder
{
public:
    JBIG2HuffmanDecoder();
    ~JBIG2HuffmanDecoder() = default;
    void setStream(Stream *strA) { str = strA; }

    void reset();

    // Returns false for OOB, otherwise sets *<x> and returns true.
    bool decodeInt(int *x, const JBIG2HuffmanTable *table);

    unsigned int readBits(unsigned int n, bool *eof = nullptr);
    unsigned int readBit();

    // Sort the table by prefix length and assign prefix values.
    static bool buildTable(JBIG2HuffmanTable *table, unsigned int len);

    void resetByteCounter() { byteCounter = 0; }
    unsigned int getByteCounter() const { return byteCounter; }

private:
    Stream *str;
    unsigned int buf;
    unsigned int bufLen;
    unsigned int byteCounter;
};

JBIG2HuffmanDecoder::JBIG2HuffmanDecoder()
{
    str = nullptr;
    byteCounter = 0;
    reset();
}

void JBIG2HuffmanDecoder::reset()
{
    buf = 0;
    bufLen = 0;
}

//~ optimize this
bool JBIG2HuffmanDecoder::decodeInt(int *x, const JBIG2HuffmanTable *table)
{
    unsigned int i, len, prefix;

    i = 0;
    len = 0;
    prefix = 0;
    while (table[i].rangeLen != jbig2HuffmanEOT) {
        while (len < table[i].prefixLen) {
            prefix = (prefix << 1) | readBit();
            ++len;
        }
        if (prefix == table[i].prefix) {
            if (table[i].rangeLen == jbig2HuffmanOOB) {
                return false;
            }
            if (table[i].rangeLen == jbig2HuffmanLOW) {
                *x = table[i].val - readBits(32);
            } else if (table[i].rangeLen > 0) {
                *x = table[i].val + readBits(table[i].rangeLen);
            } else {
                *x = table[i].val;
            }
            return true;
        }
        ++i;
    }
    return false;
}

unsigned int JBIG2HuffmanDecoder::readBits(unsigned int n, bool *eof)
{
    unsigned int x, mask, nLeft;

    mask = (n == 32) ? 0xffffffff : ((1 << n) - 1);
    if (bufLen >= n) {
        x = (buf >> (bufLen - n)) & mask;
        bufLen -= n;
    } else {
        x = buf & ((1 << bufLen) - 1);
        nLeft = n - bufLen;
        bufLen = 0;
        while (nLeft >= 8) {
            const int c = str->getChar();
            if (eof) {
                *eof = *eof || c == EOF;
            }
            x = (x << 8) | (c & 0xff);
            ++byteCounter;
            nLeft -= 8;
        }
        if (nLeft > 0) {
            const int c = str->getChar();
            if (eof) {
                *eof = *eof || c == EOF;
            }
            buf = c;
            ++byteCounter;
            bufLen = 8 - nLeft;
            x = (x << nLeft) | ((buf >> bufLen) & ((1 << nLeft) - 1));
        }
    }
    return x;
}

unsigned int JBIG2HuffmanDecoder::readBit()
{
    if (bufLen == 0) {
        buf = str->getChar();
        ++byteCounter;
        bufLen = 8;
    }
    --bufLen;
    return (buf >> bufLen) & 1;
}

bool JBIG2HuffmanDecoder::buildTable(JBIG2HuffmanTable *table, unsigned int len)
{
    unsigned int i, j, k, prefix;
    JBIG2HuffmanTable tab;

    // stable selection sort:
    // - entries with prefixLen > 0, in ascending prefixLen order
    // - entry with prefixLen = 0, rangeLen = EOT
    // - all other entries with prefixLen = 0
    // (on entry, table[len] has prefixLen = 0, rangeLen = EOT)
    for (i = 0; i < len; ++i) {
        for (j = i; j < len && table[j].prefixLen == 0; ++j) {
            ;
        }
        if (j == len) {
            break;
        }
        for (k = j + 1; k < len; ++k) {
            if (table[k].prefixLen > 0 && table[k].prefixLen < table[j].prefixLen) {
                j = k;
            }
        }
        if (j != i) {
            tab = table[j];
            for (k = j; k > i; --k) {
                table[k] = table[k - 1];
            }
            table[i] = tab;
        }
    }
    table[i] = table[len];

    // assign prefixes
    if (table[0].rangeLen != jbig2HuffmanEOT) {
        i = 0;
        prefix = 0;
        table[i++].prefix = prefix++;
        for (; table[i].rangeLen != jbig2HuffmanEOT; ++i) {
            const size_t bitsToShift = table[i].prefixLen - table[i - 1].prefixLen;
            if (bitsToShift >= intNBits) {
                error(errSyntaxError, -1, "Failed to build table for JBIG2 stream");
                return false;
            }
            prefix <<= bitsToShift;

            table[i].prefix = prefix++;
        }
    }

    return true;
}

//------------------------------------------------------------------------
// JBIG2MMRDecoder
//------------------------------------------------------------------------

class JBIG2MMRDecoder
{
public:
    JBIG2MMRDecoder();
    ~JBIG2MMRDecoder() = default;
    void setStream(Stream *strA) { str = strA; }
    void reset();
    int get2DCode();
    int getBlackCode();
    int getWhiteCode();
    unsigned int get24Bits();
    void resetByteCounter() { byteCounter = 0; }
    unsigned int getByteCounter() const { return byteCounter; }
    void skipTo(unsigned int length);

private:
    Stream *str;
    unsigned int buf;
    unsigned int bufLen;
    unsigned int nBytesRead;
    unsigned int byteCounter;
};

JBIG2MMRDecoder::JBIG2MMRDecoder()
{
    str = nullptr;
    byteCounter = 0;
    reset();
}

void JBIG2MMRDecoder::reset()
{
    buf = 0;
    bufLen = 0;
    nBytesRead = 0;
}

int JBIG2MMRDecoder::get2DCode()
{
    const CCITTCode *p = nullptr;

    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
        p = &twoDimTab1[(buf >> 1) & 0x7f];
    } else if (bufLen == 8) {
        p = &twoDimTab1[(buf >> 1) & 0x7f];
    } else if (bufLen < 8) {
        p = &twoDimTab1[(buf << (7 - bufLen)) & 0x7f];
        if (p->bits < 0 || p->bits > (int)bufLen) {
            buf = (buf << 8) | (str->getChar() & 0xff);
            bufLen += 8;
            ++nBytesRead;
            ++byteCounter;
            p = &twoDimTab1[(buf >> (bufLen - 7)) & 0x7f];
        }
    }
    if (p == nullptr || p->bits < 0) {
        error(errSyntaxError, str->getPos(), "Bad two dim code in JBIG2 MMR stream");
        return EOF;
    }
    bufLen -= p->bits;
    return p->n;
}

int JBIG2MMRDecoder::getWhiteCode()
{
    const CCITTCode *p;
    unsigned int code;

    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
    }
    while (true) {
        if (bufLen >= 11 && ((buf >> (bufLen - 7)) & 0x7f) == 0) {
            if (bufLen <= 12) {
                code = buf << (12 - bufLen);
            } else {
                code = buf >> (bufLen - 12);
            }
            p = &whiteTab1[code & 0x1f];
        } else {
            if (bufLen <= 9) {
                code = buf << (9 - bufLen);
            } else {
                code = buf >> (bufLen - 9);
            }
            p = &whiteTab2[code & 0x1ff];
        }
        if (p->bits > 0 && p->bits <= (int)bufLen) {
            bufLen -= p->bits;
            return p->n;
        }
        if (bufLen >= 12) {
            break;
        }
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    error(errSyntaxError, str->getPos(), "Bad white code in JBIG2 MMR stream");
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    --bufLen;
    return 1;
}

int JBIG2MMRDecoder::getBlackCode()
{
    const CCITTCode *p;
    unsigned int code;

    if (bufLen == 0) {
        buf = str->getChar() & 0xff;
        bufLen = 8;
        ++nBytesRead;
        ++byteCounter;
    }
    while (true) {
        if (bufLen >= 10 && ((buf >> (bufLen - 6)) & 0x3f) == 0) {
            if (bufLen <= 13) {
                code = buf << (13 - bufLen);
            } else {
                code = buf >> (bufLen - 13);
            }
            p = &blackTab1[code & 0x7f];
        } else if (bufLen >= 7 && ((buf >> (bufLen - 4)) & 0x0f) == 0 && ((buf >> (bufLen - 6)) & 0x03) != 0) {
            if (bufLen <= 12) {
                code = buf << (12 - bufLen);
            } else {
                code = buf >> (bufLen - 12);
            }
            if (unlikely((code & 0xff) < 64)) {
                break;
            }
            p = &blackTab2[(code & 0xff) - 64];
        } else {
            if (bufLen <= 6) {
                code = buf << (6 - bufLen);
            } else {
                code = buf >> (bufLen - 6);
            }
            p = &blackTab3[code & 0x3f];
        }
        if (p->bits > 0 && p->bits <= (int)bufLen) {
            bufLen -= p->bits;
            return p->n;
        }
        if (bufLen >= 13) {
            break;
        }
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    error(errSyntaxError, str->getPos(), "Bad black code in JBIG2 MMR stream");
    // eat a bit and return a positive number so that the caller doesn't
    // go into an infinite loop
    --bufLen;
    return 1;
}

unsigned int JBIG2MMRDecoder::get24Bits()
{
    while (bufLen < 24) {
        buf = (buf << 8) | (str->getChar() & 0xff);
        bufLen += 8;
        ++nBytesRead;
        ++byteCounter;
    }
    return (buf >> (bufLen - 24)) & 0xffffff;
}

void JBIG2MMRDecoder::skipTo(unsigned int length)
{
    int n = str->discardChars(length - nBytesRead);
    nBytesRead += n;
    byteCounter += n;
}

//------------------------------------------------------------------------
// JBIG2Segment
//------------------------------------------------------------------------

enum JBIG2SegmentType
{
    jbig2SegBitmap,
    jbig2SegSymbolDict,
    jbig2SegPatternDict,
    jbig2SegCodeTable
};

class JBIG2Segment
{
public:
    explicit JBIG2Segment(unsigned int segNumA) { segNum = segNumA; }
    virtual ~JBIG2Segment();
    JBIG2Segment(const JBIG2Segment &) = delete;
    JBIG2Segment &operator=(const JBIG2Segment &) = delete;
    void setSegNum(unsigned int segNumA) { segNum = segNumA; }
    unsigned int getSegNum() const { return segNum; }
    virtual JBIG2SegmentType getType() = 0;

private:
    unsigned int segNum;
};

JBIG2Segment::~JBIG2Segment() = default;

//------------------------------------------------------------------------
// JBIG2Bitmap
//------------------------------------------------------------------------

struct JBIG2BitmapPtr
{
    unsigned char *p;
    int shift;
    int x;
};

class JBIG2Bitmap : public JBIG2Segment
{
public:
    JBIG2Bitmap(unsigned int segNumA, int wA, int hA);
    explicit JBIG2Bitmap(JBIG2Bitmap *bitmap);
    ~JBIG2Bitmap() override;
    JBIG2SegmentType getType() override { return jbig2SegBitmap; }
    JBIG2Bitmap *getSlice(unsigned int x, unsigned int y, unsigned int wA, unsigned int hA);
    void expand(int newH, unsigned int pixel);
    void clearToZero();
    void clearToOne();
    int getWidth() const { return w; }
    int getHeight() const { return h; }
    int getLineSize() const { return line; }
    int getPixel(int x, int y) const { return (x < 0 || x >= w || y < 0 || y >= h) ? 0 : (data[y * line + (x >> 3)] >> (7 - (x & 7))) & 1; }
    void setPixel(int x, int y) { data[y * line + (x >> 3)] |= 1 << (7 - (x & 7)); }
    void clearPixel(int x, int y) { data[y * line + (x >> 3)] &= 0x7f7f >> (x & 7); }
    void getPixelPtr(int x, int y, JBIG2BitmapPtr *ptr);
    int nextPixel(JBIG2BitmapPtr *ptr) const;
    void duplicateRow(int yDest, int ySrc);
    void combine(JBIG2Bitmap *bitmap, int x, int y, unsigned int combOp);
    unsigned char *getDataPtr() { return data; }
    int getDataSize() const { return h * line; }
    bool isOk() const { return data != nullptr; }

private:
    int w, h, line;
    unsigned char *data;
};

JBIG2Bitmap::JBIG2Bitmap(unsigned int segNumA, int wA, int hA) : JBIG2Segment(segNumA)
{
    w = wA;
    h = hA;
    int auxW;
    if (unlikely(checkedAdd(wA, 7, &auxW))) {
        error(errSyntaxError, -1, "invalid width");
        data = nullptr;
        return;
    }
    line = auxW >> 3;

    if (w <= 0 || h <= 0 || line <= 0 || h >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)gmalloc_checkoverflow(h * line + 1);
    if (data != nullptr) {
        data[h * line] = 0;
    }
}

JBIG2Bitmap::JBIG2Bitmap(JBIG2Bitmap *bitmap) : JBIG2Segment(0)
{
    if (unlikely(bitmap == nullptr)) {
        error(errSyntaxError, -1, "NULL bitmap in JBIG2Bitmap");
        w = h = line = 0;
        data = nullptr;
        return;
    }

    w = bitmap->w;
    h = bitmap->h;
    line = bitmap->line;

    if (w <= 0 || h <= 0 || line <= 0 || h >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)gmalloc(h * line + 1);
    memcpy(data, bitmap->data, h * line);
    data[h * line] = 0;
}

JBIG2Bitmap::~JBIG2Bitmap()
{
    gfree(data);
}

//~ optimize this
JBIG2Bitmap *JBIG2Bitmap::getSlice(unsigned int x, unsigned int y, unsigned int wA, unsigned int hA)
{
    JBIG2Bitmap *slice;
    unsigned int xx, yy;

    if (!data) {
        return nullptr;
    }

    slice = new JBIG2Bitmap(0, wA, hA);
    if (slice->isOk()) {
        slice->clearToZero();
        for (yy = 0; yy < hA; ++yy) {
            for (xx = 0; xx < wA; ++xx) {
                if (getPixel(x + xx, y + yy)) {
                    slice->setPixel(xx, yy);
                }
            }
        }
    } else {
        delete slice;
        slice = nullptr;
    }
    return slice;
}

void JBIG2Bitmap::expand(int newH, unsigned int pixel)
{
    if (unlikely(!data)) {
        return;
    }
    if (newH <= h || line <= 0 || newH >= (INT_MAX - 1) / line) {
        error(errSyntaxError, -1, "invalid width/height");
        gfree(data);
        data = nullptr;
        return;
    }
    // need to allocate one extra guard byte for use in combine()
    data = (unsigned char *)grealloc(data, newH * line + 1);
    if (pixel) {
        memset(data + h * line, 0xff, (newH - h) * line);
    } else {
        memset(data + h * line, 0x00, (newH - h) * line);
    }
    h = newH;
    data[h * line] = 0;
}

void JBIG2Bitmap::clearToZero()
{
    memset(data, 0, h * line);
}

void JBIG2Bitmap::clearToOne()
{
    memset(data, 0xff, h * line);
}

inline void JBIG2Bitmap::getPixelPtr(int x, int y, JBIG2BitmapPtr *ptr)
{
    if (y < 0 || y >= h || x >= w) {
        ptr->p = nullptr;
        ptr->shift = 0; // make gcc happy
        ptr->x = 0; // make gcc happy
    } else if (x < 0) {
        ptr->p = &data[y * line];
        ptr->shift = 7;
        ptr->x = x;
    } else {
        ptr->p = &data[y * line + (x >> 3)];
        ptr->shift = 7 - (x & 7);
        ptr->x = x;
    }
}

inline int JBIG2Bitmap::nextPixel(JBIG2BitmapPtr *ptr) const
{
    int pix;

    if (!ptr->p) {
        pix = 0;
    } else if (ptr->x < 0) {
        ++ptr->x;
        pix = 0;
    } else {
        pix = (*ptr->p >> ptr->shift) & 1;
        if (++ptr->x == w) {
            ptr->p = nullptr;
        } else if (ptr->shift == 0) {
            ++ptr->p;
            ptr->shift = 7;
        } else {
            --ptr->shift;
        }
    }
    return pix;
}

void JBIG2Bitmap::duplicateRow(int yDest, int ySrc)
{
    memcpy(data + yDest * line, data + ySrc * line, line);
}

void JBIG2Bitmap::combine(JBIG2Bitmap *bitmap, int x, int y, unsigned int combOp)
{
    int x0, x1, y0, y1, xx, yy, yyy;
    unsigned char *srcPtr, *destPtr;
    unsigned int src0, src1, src, dest, s1, s2, m1, m2, m3;
    bool oneByte;

    if (unlikely(!isOk())) {
        return;
    }
    // check for the pathological case where y = -2^31
    if (y < -0x7fffffff) {
        return;
    }
    if (y < 0) {
        y0 = -y;
    } else {
        y0 = 0;
    }
    if (y + bitmap->h > h) {
        y1 = h - y;
    } else {
        y1 = bitmap->h;
    }
    if (y0 >= y1) {
        return;
    }

    if (x >= 0) {
        x0 = x & ~7;
    } else {
        x0 = 0;
    }
    if (unlikely(checkedAdd(x, bitmap->w, &x1))) {
        return;
    }
    if (x1 > w) {
        x1 = w;
    }
    if (x0 >= x1) {
        return;
    }

    s1 = x & 7;
    s2 = 8 - s1;
    m1 = 0xff >> (x1 & 7);
    m2 = 0xff << (((x1 & 7) == 0) ? 0 : 8 - (x1 & 7));
    m3 = (0xff >> s1) & m2;

    oneByte = x0 == ((x1 - 1) & ~7);

    for (yy = y0; yy < y1; ++yy) {
        if (unlikely(checkedAdd(y, yy, &yyy))) {
            continue;
        }
        if (unlikely((yyy >= h) || (yyy < 0))) {
            continue;
        }

        // one byte per line -- need to mask both left and right side
        if (oneByte) {
            if (x >= 0) {
                destPtr = data + yyy * line + (x >> 3);
                srcPtr = bitmap->data + yy * bitmap->line;
                dest = *destPtr;
                src1 = *srcPtr;
                switch (combOp) {
                case 0: // or
                    dest |= (src1 >> s1) & m2;
                    break;
                case 1: // and
                    dest &= ((0xff00 | src1) >> s1) | m1;
                    break;
                case 2: // xor
                    dest ^= (src1 >> s1) & m2;
                    break;
                case 3: // xnor
                    dest ^= ((src1 ^ 0xff) >> s1) & m2;
                    break;
                case 4: // replace
                    dest = (dest & ~m3) | ((src1 >> s1) & m3);
                    break;
                }
                *destPtr = dest;
            } else {
                destPtr = data + yyy * line;
                srcPtr = bitmap->data + yy * bitmap->line + (-x >> 3);
                dest = *destPtr;
                src1 = *srcPtr;
                switch (combOp) {
                case 0: // or
                    dest |= src1 & m2;
                    break;
                case 1: // and
                    dest &= src1 | m1;
                    break;
                case 2: // xor
                    dest ^= src1 & m2;
                    break;
                case 3: // xnor
                    dest ^= (src1 ^ 0xff) & m2;
                    break;
                case 4: // replace
                    dest = (src1 & m2) | (dest & m1);
                    break;
                }
                *destPtr = dest;
            }

            // multiple bytes per line -- need to mask left side of left-most
            // byte and right side of right-most byte
        } else {

            // left-most byte
            if (x >= 0) {
                destPtr = data + yyy * line + (x >> 3);
                srcPtr = bitmap->data + yy * bitmap->line;
                src1 = *srcPtr++;
                dest = *destPtr;
                switch (combOp) {
                case 0: // or
                    dest |= src1 >> s1;
                    break;
                case 1: // and
                    dest &= (0xff00 | src1) >> s1;
                    break;
                case 2: // xor
                    dest ^= src1 >> s1;
                    break;
                case 3: // xnor
                    dest ^= (src1 ^ 0xff) >> s1;
                    break;
                case 4: // replace
                    dest = (dest & (0xff << s2)) | (src1 >> s1);
                    break;
                }
                *destPtr++ = dest;
                xx = x0 + 8;
            } else {
                destPtr = data + yyy * line;
                srcPtr = bitmap->data + yy * bitmap->line + (-x >> 3);
                src1 = *srcPtr++;
                xx = x0;
            }

            // middle bytes
            for (; xx < x1 - 8; xx += 8) {
                dest = *destPtr;
                src0 = src1;
                src1 = *srcPtr++;
                src = (((src0 << 8) | src1) >> s1) & 0xff;
                switch (combOp) {
                case 0: // or
                    dest |= src;
                    break;
                case 1: // and
                    dest &= src;
                    break;
                case 2: // xor
                    dest ^= src;
                    break;
                case 3: // xnor
                    dest ^= src ^ 0xff;
                    break;
                case 4: // replace
                    dest = src;
                    break;
                }
                *destPtr++ = dest;
            }

            // right-most byte
            // note: this last byte (src1) may not actually be used, depending
            // on the values of s1, m1, and m2 - and in fact, it may be off
            // the edge of the source bitmap, which means we need to allocate
            // one extra guard byte at the end of each bitmap
            dest = *destPtr;
            src0 = src1;
            src1 = *srcPtr++;
            src = (((src0 << 8) | src1) >> s1) & 0xff;
            switch (combOp) {
            case 0: // or
                dest |= src & m2;
                break;
            case 1: // and
                dest &= src | m1;
                break;
            case 2: // xor
                dest ^= src & m2;
                break;
            case 3: // xnor
                dest ^= (src ^ 0xff) & m2;
                break;
            case 4: // replace
                dest = (src & m2) | (dest & m1);
                break;
            }
            *destPtr = dest;
        }
    }
}

//------------------------------------------------------------------------
// JBIG2SymbolDict
//------------------------------------------------------------------------

class JBIG2SymbolDict : public JBIG2Segment
{
public:
    JBIG2SymbolDict(unsigned int segNumA, unsigned int sizeA);
    ~JBIG2SymbolDict() override;
    JBIG2SegmentType getType() override { return jbig2SegSymbolDict; }
    unsigned int getSize() const { return size; }
    void setBitmap(unsigned int idx, JBIG2Bitmap *bitmap) { bitmaps[idx] = bitmap; }
    JBIG2Bitmap *getBitmap(unsigned int idx) { return bitmaps[idx]; }
    bool isOk() const { return ok; }
    void setGenericRegionStats(JArithmeticDecoderStats *stats) { genericRegionStats = stats; }
    void setRefinementRegionStats(JArithmeticDecoderStats *stats) { refinementRegionStats = stats; }
    JArithmeticDecoderStats *getGenericRegionStats() { return genericRegionStats; }
    JArithmeticDecoderStats *getRefinementRegionStats() { return refinementRegionStats; }

private:
    bool ok;
    unsigned int size;
    JBIG2Bitmap **bitmaps;
    JArithmeticDecoderStats *genericRegionStats;
    JArithmeticDecoderStats *refinementRegionStats;
};

JBIG2SymbolDict::JBIG2SymbolDict(unsigned int segNumA, unsigned int sizeA) : JBIG2Segment(segNumA)
{
    ok = true;
    size = sizeA;
    if (size != 0) {
        bitmaps = (JBIG2Bitmap **)gmallocn_checkoverflow(size, sizeof(JBIG2Bitmap *));
        if (!bitmaps) {
            ok = false;
            size = 0;
        }
    } else {
        bitmaps = nullptr;
    }
    for (unsigned int i = 0; i < size; ++i) {
        bitmaps[i] = nullptr;
    }
    genericRegionStats = nullptr;
    refinementRegionStats = nullptr;
}

JBIG2SymbolDict::~JBIG2SymbolDict()
{
    unsigned int i;

    for (i = 0; i < size; ++i) {
        delete bitmaps[i];
    }
    gfree(static_cast<void *>(bitmaps));
    delete genericRegionStats;
    delete refinementRegionStats;
}

//------------------------------------------------------------------------
// JBIG2PatternDict
//------------------------------------------------------------------------

class JBIG2PatternDict : public JBIG2Segment
{
public:
    JBIG2PatternDict(unsigned int segNumA, unsigned int sizeA);
    ~JBIG2PatternDict() override;
    JBIG2SegmentType getType() override { return jbig2SegPatternDict; }
    unsigned int getSize() const { return size; }
    void setBitmap(unsigned int idx, JBIG2Bitmap *bitmap)
    {
        if (likely(idx < size)) {
            bitmaps[idx] = bitmap;
        }
    }
    JBIG2Bitmap *getBitmap(unsigned int idx) { return (idx < size) ? bitmaps[idx] : nullptr; }

private:
    unsigned int size;
    JBIG2Bitmap **bitmaps;
};

JBIG2PatternDict::JBIG2PatternDict(unsigned int segNumA, unsigned int sizeA) : JBIG2Segment(segNumA)
{
    bitmaps = (JBIG2Bitmap **)gmallocn_checkoverflow(sizeA, sizeof(JBIG2Bitmap *));
    if (bitmaps) {
        size = sizeA;
    } else {
        size = 0;
        error(errSyntaxError, -1, "JBIG2PatternDict: can't allocate bitmaps");
    }
}

JBIG2PatternDict::~JBIG2PatternDict()
{
    unsigned int i;

    for (i = 0; i < size; ++i) {
        delete bitmaps[i];
    }
    gfree(static_cast<void *>(bitmaps));
}

//------------------------------------------------------------------------
// JBIG2CodeTable
//------------------------------------------------------------------------

class JBIG2CodeTable : public JBIG2Segment
{
public:
    JBIG2CodeTable(unsigned int segNumA, JBIG2HuffmanTable *tableA);
    ~JBIG2CodeTable() override;
    JBIG2SegmentType getType() override { return jbig2SegCodeTable; }
    JBIG2HuffmanTable *getHuffTable() { return table; }

private:
    JBIG2HuffmanTable *table;
};

JBIG2CodeTable::JBIG2CodeTable(unsigned int segNumA, JBIG2HuffmanTable *tableA) : JBIG2Segment(segNumA)
{
    table = tableA;
}

JBIG2CodeTable::~JBIG2CodeTable()
{
    gfree(table);
}

//------------------------------------------------------------------------
// JBIG2Stream
//------------------------------------------------------------------------

JBIG2Stream::JBIG2Stream(std::unique_ptr<Stream> strA, Object &&globalsStreamA, Object *globalsStreamRefA) : OwnedFilterStream(std::move(strA))
{
    pageBitmap = nullptr;

    arithDecoder = new JArithmeticDecoder();
    genericRegionStats = new JArithmeticDecoderStats(1 << 1);
    refinementRegionStats = new JArithmeticDecoderStats(1 << 1);
    iadhStats = new JArithmeticDecoderStats(1 << 9);
    iadwStats = new JArithmeticDecoderStats(1 << 9);
    iaexStats = new JArithmeticDecoderStats(1 << 9);
    iaaiStats = new JArithmeticDecoderStats(1 << 9);
    iadtStats = new JArithmeticDecoderStats(1 << 9);
    iaitStats = new JArithmeticDecoderStats(1 << 9);
    iafsStats = new JArithmeticDecoderStats(1 << 9);
    iadsStats = new JArithmeticDecoderStats(1 << 9);
    iardxStats = new JArithmeticDecoderStats(1 << 9);
    iardyStats = new JArithmeticDecoderStats(1 << 9);
    iardwStats = new JArithmeticDecoderStats(1 << 9);
    iardhStats = new JArithmeticDecoderStats(1 << 9);
    iariStats = new JArithmeticDecoderStats(1 << 9);
    iaidStats = new JArithmeticDecoderStats(1 << 1);
    huffDecoder = new JBIG2HuffmanDecoder();
    mmrDecoder = new JBIG2MMRDecoder();

    if (globalsStreamA.isStream()) {
        globalsStream = std::move(globalsStreamA);
        if (globalsStreamRefA->isRef()) {
            globalsStreamRef = globalsStreamRefA->getRef();
        }
    }

    curStr = nullptr;
    dataPtr = dataEnd = nullptr;
}

JBIG2Stream::~JBIG2Stream()
{
    close();
    delete arithDecoder;
    delete genericRegionStats;
    delete refinementRegionStats;
    delete iadhStats;
    delete iadwStats;
    delete iaexStats;
    delete iaaiStats;
    delete iadtStats;
    delete iaitStats;
    delete iafsStats;
    delete iadsStats;
    delete iardxStats;
    delete iardyStats;
    delete iardwStats;
    delete iardhStats;
    delete iariStats;
    delete iaidStats;
    delete huffDecoder;
    delete mmrDecoder;
}

bool JBIG2Stream::rewind()
{
    segments.resize(0);
    globalSegments.resize(0);
    bool rewindSuccess = true;

    // read the globals stream
    if (globalsStream.isStream()) {
        curStr = globalsStream.getStream();
        rewindSuccess = curStr->rewind();
        arithDecoder->setStream(curStr);
        huffDecoder->setStream(curStr);
        mmrDecoder->setStream(curStr);
        readSegments();
        curStr->close();
        // swap the newly read segments list into globalSegments
        std::swap(segments, globalSegments);
    }

    // read the main stream
    curStr = str;
    rewindSuccess = curStr->rewind() && rewindSuccess;
    arithDecoder->setStream(curStr);
    huffDecoder->setStream(curStr);
    mmrDecoder->setStream(curStr);
    readSegments();

    if (pageBitmap) {
        dataPtr = pageBitmap->getDataPtr();
        dataEnd = dataPtr + pageBitmap->getDataSize();
    } else {
        dataPtr = dataEnd = nullptr;
    }

    return rewindSuccess;
}

void JBIG2Stream::close()
{
    if (pageBitmap) {
        delete pageBitmap;
        pageBitmap = nullptr;
    }
    segments.resize(0);
    globalSegments.resize(0);
    dataPtr = dataEnd = nullptr;
    FilterStream::close();
}

int JBIG2Stream::getChar()
{
    if (dataPtr && dataPtr < dataEnd) {
        return (*dataPtr++ ^ 0xff) & 0xff;
    }
    return EOF;
}

int JBIG2Stream::lookChar()
{
    if (dataPtr && dataPtr < dataEnd) {
        return (*dataPtr ^ 0xff) & 0xff;
    }
    return EOF;
}

Goffset JBIG2Stream::getPos()
{
    if (pageBitmap == nullptr) {
        return 0;
    }
    return dataPtr - pageBitmap->getDataPtr();
}

int JBIG2Stream::getChars(int nChars, unsigned char *buffer)
{
    int n, i;

    if (nChars <= 0 || !dataPtr) {
        return 0;
    }
    if (dataEnd - dataPtr < nChars) {
        n = (int)(dataEnd - dataPtr);
    } else {
        n = nChars;
    }
    for (i = 0; i < n; ++i) {
        buffer[i] = *dataPtr++ ^ 0xff;
    }
    return n;
}

std::optional<std::string> JBIG2Stream::getPSFilter(int /*psLevel*/, const char * /*indent*/)
{
    return {};
}

bool JBIG2Stream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

void JBIG2Stream::readSegments()
{
    unsigned int segNum, segFlags, segType, page, segLength;
    unsigned int refFlags, nRefSegs;
    unsigned int *refSegs;
    int c1, c2, c3;

    bool done = false;
    while (!done && readULong(&segNum)) {

        // segment header flags
        if (!readUByte(&segFlags)) {
            goto eofError1;
        }
        segType = segFlags & 0x3f;

        // referred-to segment count and retention flags
        if (!readUByte(&refFlags)) {
            goto eofError1;
        }
        nRefSegs = refFlags >> 5;
        if (nRefSegs == 7) {
            if ((c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
                goto eofError1;
            }
            refFlags = (refFlags << 24) | (c1 << 16) | (c2 << 8) | c3;
            nRefSegs = refFlags & 0x1fffffff;
            const unsigned int nCharsToRead = (nRefSegs + 9) >> 3;
            for (unsigned int i = 0; i < nCharsToRead; ++i) {
                if ((c1 = curStr->getChar()) == EOF) {
                    goto eofError1;
                }
            }
        }

        // referred-to segment numbers
        refSegs = (unsigned int *)gmallocn_checkoverflow(nRefSegs, sizeof(unsigned int));
        if (nRefSegs > 0 && !refSegs) {
            return;
        }
        if (segNum <= 256) {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readUByte(&refSegs[i])) {
                    goto eofError2;
                }
            }
        } else if (segNum <= 65536) {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readUWord(&refSegs[i])) {
                    goto eofError2;
                }
            }
        } else {
            for (unsigned int i = 0; i < nRefSegs; ++i) {
                if (!readULong(&refSegs[i])) {
                    goto eofError2;
                }
            }
        }

        // segment page association
        if (segFlags & 0x40) {
            if (!readULong(&page)) {
                goto eofError2;
            }
        } else {
            if (!readUByte(&page)) {
                goto eofError2;
            }
        }

        // segment data length
        if (!readULong(&segLength)) {
            goto eofError2;
        }

        // check for missing page information segment
        if (!pageBitmap && ((segType >= 4 && segType <= 7) || (segType >= 20 && segType <= 43))) {
            error(errSyntaxError, curStr->getPos(), "First JBIG2 segment associated with a page must be a page information segment");
            goto syntaxError;
        }

        // read the segment data
        arithDecoder->resetByteCounter();
        huffDecoder->resetByteCounter();
        mmrDecoder->resetByteCounter();
        byteCounter = 0;
        switch (segType) {
        case 0:
            if (!readSymbolDictSeg(segNum, refSegs, nRefSegs)) {
                error(errSyntaxError, curStr->getPos(), "readSymbolDictSeg reports syntax error!");
                goto syntaxError;
            }
            break;
        case 4:
            readTextRegionSeg(segNum, false, refSegs, nRefSegs);
            break;
        case 6:
            readTextRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 7:
            readTextRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 16:
            readPatternDictSeg(segNum, segLength);
            break;
        case 20:
            readHalftoneRegionSeg(segNum, false, refSegs, nRefSegs);
            break;
        case 22:
            readHalftoneRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 23:
            readHalftoneRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 36:
            readGenericRegionSeg(segNum, false, segLength);
            break;
        case 38:
            readGenericRegionSeg(segNum, true, segLength);
            break;
        case 39:
            readGenericRegionSeg(segNum, true, segLength);
            break;
        case 40:
            readGenericRefinementRegionSeg(segNum, false, refSegs, nRefSegs);
            break;
        case 42:
            readGenericRefinementRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 43:
            readGenericRefinementRegionSeg(segNum, true, refSegs, nRefSegs);
            break;
        case 48:
            readPageInfoSeg();
            break;
        case 50:
            readEndOfStripeSeg(segLength);
            break;
        case 51:
            // end of file segment
            done = true;
            break;
        case 52:
            readProfilesSeg(segLength);
            break;
        case 53:
            readCodeTableSeg(segNum);
            break;
        case 62:
            readExtensionSeg(segLength);
            break;
        default:
            error(errSyntaxError, curStr->getPos(), "Unknown segment type in JBIG2 stream");
            for (unsigned int i = 0; i < segLength; ++i) {
                if ((c1 = curStr->getChar()) == EOF) {
                    goto eofError2;
                }
            }
            break;
        }

        // Make sure the segment handler read all of the bytes in the
        // segment data, unless this segment is marked as having an
        // unknown length (section 7.2.7 of the JBIG2 Final Committee Draft)

        if (segType != 38 || segLength != 0xffffffff) {

            byteCounter += arithDecoder->getByteCounter();
            byteCounter += huffDecoder->getByteCounter();
            byteCounter += mmrDecoder->getByteCounter();

            if (segLength > byteCounter) {
                const unsigned int segExtraBytes = segLength - byteCounter;

                // If we didn't read all of the bytes in the segment data,
                // indicate an error, and throw away the rest of the data.

                // v.3.1.01.13 of the LuraTech PDF Compressor Server will
                // sometimes generate an extraneous NULL byte at the end of
                // arithmetic-coded symbol dictionary segments when numNewSyms
                // == 0.  Segments like this often occur for blank pages.

                error(errSyntaxError, curStr->getPos(), "{0:ud} extraneous byte{1:s} after segment", segExtraBytes, (segExtraBytes > 1) ? "s" : "");
                byteCounter += curStr->discardChars(segExtraBytes);
            } else if (segLength < byteCounter) {

                // If we read more bytes than we should have, according to the
                // segment length field, note an error.

                error(errSyntaxError, curStr->getPos(), "Previous segment handler read too many bytes");
                goto syntaxError;
            }
        }

        gfree(refSegs);
    }

    return;

syntaxError:
    gfree(refSegs);
    return;

eofError2:
    gfree(refSegs);
eofError1:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

bool JBIG2Stream::readSymbolDictSeg(unsigned int segNum, unsigned int *refSegs, unsigned int nRefSegs)
{
    std::unique_ptr<JBIG2SymbolDict> symbolDict;
    const JBIG2HuffmanTable *huffDHTable, *huffDWTable;
    const JBIG2HuffmanTable *huffBMSizeTable, *huffAggInstTable;
    JBIG2Segment *seg;
    std::vector<JBIG2Segment *> codeTables;
    JBIG2SymbolDict *inputSymbolDict;
    unsigned int flags, sdTemplate, sdrTemplate, huff, refAgg;
    unsigned int huffDH, huffDW, huffBMSize, huffAggInst;
    unsigned int contextUsed, contextRetained;
    int sdATX[4], sdATY[4], sdrATX[2], sdrATY[2];
    unsigned int numExSyms, numNewSyms, numInputSyms, symCodeLen;
    JBIG2Bitmap **bitmaps;
    JBIG2Bitmap *collBitmap, *refBitmap;
    unsigned int *symWidths;
    unsigned int symHeight, symWidth, totalWidth, x, symID;
    int dh = 0, dw, refAggNum, refDX = 0, refDY = 0, bmSize;
    bool ex;
    int run, cnt, c;
    unsigned int i, j, k;
    unsigned char *p;

    symWidths = nullptr;

    // symbol dictionary flags
    if (!readUWord(&flags)) {
        goto eofError;
    }
    sdTemplate = (flags >> 10) & 3;
    sdrTemplate = (flags >> 12) & 1;
    huff = flags & 1;
    refAgg = (flags >> 1) & 1;
    huffDH = (flags >> 2) & 3;
    huffDW = (flags >> 4) & 3;
    huffBMSize = (flags >> 6) & 1;
    huffAggInst = (flags >> 7) & 1;
    contextUsed = (flags >> 8) & 1;
    contextRetained = (flags >> 9) & 1;

    // symbol dictionary AT flags
    if (!huff) {
        if (sdTemplate == 0) {
            if (!readByte(&sdATX[0]) || !readByte(&sdATY[0]) || !readByte(&sdATX[1]) || !readByte(&sdATY[1]) || !readByte(&sdATX[2]) || !readByte(&sdATY[2]) || !readByte(&sdATX[3]) || !readByte(&sdATY[3])) {
                goto eofError;
            }
        } else {
            if (!readByte(&sdATX[0]) || !readByte(&sdATY[0])) {
                goto eofError;
            }
        }
    }

    // symbol dictionary refinement AT flags
    if (refAgg && !sdrTemplate) {
        if (!readByte(&sdrATX[0]) || !readByte(&sdrATY[0]) || !readByte(&sdrATX[1]) || !readByte(&sdrATY[1])) {
            goto eofError;
        }
    }

    // SDNUMEXSYMS and SDNUMNEWSYMS
    if (!readULong(&numExSyms) || !readULong(&numNewSyms)) {
        goto eofError;
    }

    // get referenced segments: input symbol dictionaries and code tables
    numInputSyms = 0;
    for (i = 0; i < nRefSegs; ++i) {
        // This is need by bug 12014, returning false makes it not crash
        // but we end up with a empty page while acroread is able to render
        // part of it
        if ((seg = findSegment(refSegs[i]))) {
            if (seg->getType() == jbig2SegSymbolDict) {
                j = ((JBIG2SymbolDict *)seg)->getSize();
                if (numInputSyms > UINT_MAX - j) {
                    error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
                    goto eofError;
                }
                numInputSyms += j;
            } else if (seg->getType() == jbig2SegCodeTable) {
                codeTables.push_back(seg);
            }
        } else {
            return false;
        }
    }
    if (numInputSyms > UINT_MAX - numNewSyms) {
        error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
        goto eofError;
    }

    // compute symbol code length, per 6.5.8.2.3
    //  symCodeLen = ceil( log2( numInputSyms + numNewSyms ) )
    i = numInputSyms + numNewSyms;
    if (i <= 1) {
        symCodeLen = huff ? 1 : 0;
    } else {
        --i;
        symCodeLen = 0;
        // i = floor((numSyms-1) / 2^symCodeLen)
        while (i > 0) {
            ++symCodeLen;
            i >>= 1;
        }
    }

    // get the input symbol bitmaps
    bitmaps = (JBIG2Bitmap **)gmallocn_checkoverflow(numInputSyms + numNewSyms, sizeof(JBIG2Bitmap *));
    if (!bitmaps && (numInputSyms + numNewSyms > 0)) {
        error(errSyntaxError, curStr->getPos(), "Too many input symbols in JBIG2 symbol dictionary");
        goto eofError;
    }
    for (i = 0; i < numInputSyms + numNewSyms; ++i) {
        bitmaps[i] = nullptr;
    }
    k = 0;
    inputSymbolDict = nullptr;
    for (i = 0; i < nRefSegs; ++i) {
        seg = findSegment(refSegs[i]);
        if (seg != nullptr && seg->getType() == jbig2SegSymbolDict) {
            inputSymbolDict = (JBIG2SymbolDict *)seg;
            for (j = 0; j < inputSymbolDict->getSize(); ++j) {
                bitmaps[k++] = inputSymbolDict->getBitmap(j);
            }
        }
    }

    // get the Huffman tables
    huffDHTable = huffDWTable = nullptr; // make gcc happy
    huffBMSizeTable = huffAggInstTable = nullptr; // make gcc happy
    i = 0;
    if (huff) {
        if (huffDH == 0) {
            huffDHTable = huffTableD;
        } else if (huffDH == 1) {
            huffDHTable = huffTableE;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffDHTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffDW == 0) {
            huffDWTable = huffTableB;
        } else if (huffDW == 1) {
            huffDWTable = huffTableC;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffDWTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffBMSize == 0) {
            huffBMSizeTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffBMSizeTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffAggInst == 0) {
            huffAggInstTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffAggInstTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
    }

    // set up the Huffman decoder
    if (huff) {
        huffDecoder->reset();

        // set up the arithmetic decoder
    } else {
        if (contextUsed && inputSymbolDict) {
            resetGenericStats(sdTemplate, inputSymbolDict->getGenericRegionStats());
        } else {
            resetGenericStats(sdTemplate, nullptr);
        }
        if (!resetIntStats(symCodeLen)) {
            goto syntaxError;
        }
        arithDecoder->start();
    }

    // set up the arithmetic decoder for refinement/aggregation
    if (refAgg) {
        if (contextUsed && inputSymbolDict) {
            resetRefinementStats(sdrTemplate, inputSymbolDict->getRefinementRegionStats());
        } else {
            resetRefinementStats(sdrTemplate, nullptr);
        }
    }

    // allocate symbol widths storage
    if (huff && !refAgg) {
        symWidths = (unsigned int *)gmallocn_checkoverflow(numNewSyms, sizeof(unsigned int));
        if (numNewSyms > 0 && !symWidths) {
            goto syntaxError;
        }
    }

    symHeight = 0;
    i = 0;
    while (i < numNewSyms) {

        // read the height class delta height
        if (huff) {
            huffDecoder->decodeInt(&dh, huffDHTable);
        } else {
            arithDecoder->decodeInt(&dh, iadhStats);
        }
        if (dh < 0 && (unsigned int)-dh >= symHeight) {
            error(errSyntaxError, curStr->getPos(), "Bad delta-height value in JBIG2 symbol dictionary");
            goto syntaxError;
        }
        symHeight += dh;
        if (unlikely(symHeight > 0x40000000)) {
            error(errSyntaxError, curStr->getPos(), "Bad height value in JBIG2 symbol dictionary");
            goto syntaxError;
        }
        symWidth = 0;
        totalWidth = 0;
        j = i;

        // read the symbols in this height class
        while (true) {

            // read the delta width
            if (huff) {
                if (!huffDecoder->decodeInt(&dw, huffDWTable)) {
                    break;
                }
            } else {
                if (!arithDecoder->decodeInt(&dw, iadwStats)) {
                    break;
                }
            }
            if (dw < 0 && (unsigned int)-dw >= symWidth) {
                error(errSyntaxError, curStr->getPos(), "Bad delta-height value in JBIG2 symbol dictionary");
                goto syntaxError;
            }
            symWidth += dw;
            if (i >= numNewSyms) {
                error(errSyntaxError, curStr->getPos(), "Too many symbols in JBIG2 symbol dictionary");
                goto syntaxError;
            }

            // using a collective bitmap, so don't read a bitmap here
            if (huff && !refAgg) {
                symWidths[i] = symWidth;
                totalWidth += symWidth;

                // refinement/aggregate coding
            } else if (refAgg) {
                if (huff) {
                    if (!huffDecoder->decodeInt(&refAggNum, huffAggInstTable)) {
                        break;
                    }
                } else {
                    if (!arithDecoder->decodeInt(&refAggNum, iaaiStats)) {
                        break;
                    }
                }
                //~ This special case was added about a year before the final draft
                //~ of the JBIG2 spec was released.  I have encountered some old
                //~ JBIG2 images that predate it.
                //~ if (0) {
                if (refAggNum == 1) {
                    if (huff) {
                        symID = huffDecoder->readBits(symCodeLen);
                        huffDecoder->decodeInt(&refDX, huffTableO);
                        huffDecoder->decodeInt(&refDY, huffTableO);
                        huffDecoder->decodeInt(&bmSize, huffTableA);
                        huffDecoder->reset();
                        arithDecoder->start();
                    } else {
                        if (iaidStats == nullptr) {
                            goto syntaxError;
                        }
                        symID = arithDecoder->decodeIAID(symCodeLen, iaidStats);
                        arithDecoder->decodeInt(&refDX, iardxStats);
                        arithDecoder->decodeInt(&refDY, iardyStats);
                    }
                    if (symID >= numInputSyms + i) {
                        error(errSyntaxError, curStr->getPos(), "Invalid symbol ID in JBIG2 symbol dictionary");
                        goto syntaxError;
                    }
                    refBitmap = bitmaps[symID];
                    if (unlikely(refBitmap == nullptr)) {
                        error(errSyntaxError, curStr->getPos(), "Invalid ref bitmap for symbol ID {0:ud} in JBIG2 symbol dictionary", symID);
                        goto syntaxError;
                    }
                    bitmaps[numInputSyms + i] = readGenericRefinementRegion(symWidth, symHeight, sdrTemplate, false, refBitmap, refDX, refDY, sdrATX, sdrATY).release();
                    //~ do we need to use the bmSize value here (in Huffman mode)?
                } else {
                    bitmaps[numInputSyms + i] = readTextRegion(huff, true, symWidth, symHeight, refAggNum, 0, numInputSyms + i, nullptr, symCodeLen, bitmaps, 0, 0, 0, 1, 0, huffTableF, huffTableH, huffTableK, huffTableO, huffTableO,
                                                               huffTableO, huffTableO, huffTableA, sdrTemplate, sdrATX, sdrATY)
                                                        .release();
                    if (unlikely(!bitmaps[numInputSyms + i])) {
                        error(errSyntaxError, curStr->getPos(), "NULL bitmap in readTextRegion");
                        goto syntaxError;
                    }
                }

                // non-ref/agg coding
            } else {
                bitmaps[numInputSyms + i] = readGenericBitmap(false, symWidth, symHeight, sdTemplate, false, false, nullptr, sdATX, sdATY, 0).release();
                if (unlikely(!bitmaps[numInputSyms + i])) {
                    error(errSyntaxError, curStr->getPos(), "NULL bitmap in readGenericBitmap");
                    goto syntaxError;
                }
            }

            ++i;
        }

        // read the collective bitmap
        if (huff && !refAgg) {
            huffDecoder->decodeInt(&bmSize, huffBMSizeTable);
            huffDecoder->reset();
            if (bmSize == 0) {
                collBitmap = new JBIG2Bitmap(0, totalWidth, symHeight);
                bmSize = symHeight * ((totalWidth + 7) >> 3);
                p = collBitmap->getDataPtr();
                if (unlikely(p == nullptr)) {
                    delete collBitmap;
                    goto syntaxError;
                }
                for (k = 0; k < (unsigned int)bmSize; ++k) {
                    if ((c = curStr->getChar()) == EOF) {
                        memset(p, 0, bmSize - k);
                        break;
                    }
                    *p++ = (unsigned char)c;
                }
                byteCounter += k;
            } else {
                collBitmap = readGenericBitmap(true, totalWidth, symHeight, 0, false, false, nullptr, nullptr, nullptr, bmSize).release();
            }
            if (likely(collBitmap != nullptr)) {
                x = 0;
                for (; j < i; ++j) {
                    bitmaps[numInputSyms + j] = collBitmap->getSlice(x, 0, symWidths[j], symHeight);
                    x += symWidths[j];
                }
                delete collBitmap;
            } else {
                error(errSyntaxError, curStr->getPos(), "collBitmap was null");
                goto syntaxError;
            }
        }
    }

    // create the symbol dict object
    symbolDict = std::make_unique<JBIG2SymbolDict>(segNum, numExSyms);
    if (!symbolDict->isOk()) {
        goto syntaxError;
    }

    // exported symbol list
    i = j = 0;
    ex = false;
    run = 0; // initialize it once in case the first decodeInt fails
             // we do not want to use uninitialized memory
    while (i < numInputSyms + numNewSyms) {
        if (huff) {
            huffDecoder->decodeInt(&run, huffTableA);
        } else {
            arithDecoder->decodeInt(&run, iaexStats);
        }
        if (i + run > numInputSyms + numNewSyms || (ex && j + run > numExSyms)) {
            error(errSyntaxError, curStr->getPos(), "Too many exported symbols in JBIG2 symbol dictionary");
            for (; j < numExSyms; ++j) {
                symbolDict->setBitmap(j, nullptr);
            }
            goto syntaxError;
        }
        if (ex) {
            for (cnt = 0; cnt < run; ++cnt) {
                symbolDict->setBitmap(j++, new JBIG2Bitmap(bitmaps[i++]));
            }
        } else {
            i += run;
        }
        ex = !ex;
    }
    if (j != numExSyms) {
        error(errSyntaxError, curStr->getPos(), "Too few symbols in JBIG2 symbol dictionary");
        for (; j < numExSyms; ++j) {
            symbolDict->setBitmap(j, nullptr);
        }
        goto syntaxError;
    }

    for (i = 0; i < numNewSyms; ++i) {
        delete bitmaps[numInputSyms + i];
    }
    gfree(static_cast<void *>(bitmaps));
    if (symWidths) {
        gfree(symWidths);
    }

    // save the arithmetic decoder stats
    if (!huff && contextRetained) {
        symbolDict->setGenericRegionStats(genericRegionStats->copy());
        if (refAgg) {
            symbolDict->setRefinementRegionStats(refinementRegionStats->copy());
        }
    }

    // store the new symbol dict
    segments.push_back(std::move(symbolDict));

    return true;

codeTableError:
    error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 symbol dictionary");

syntaxError:
    for (i = 0; i < numNewSyms; ++i) {
        if (bitmaps[numInputSyms + i]) {
            delete bitmaps[numInputSyms + i];
        }
    }
    gfree(static_cast<void *>(bitmaps));
    if (symWidths) {
        gfree(symWidths);
    }
    return false;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
    return false;
}

void JBIG2Stream::readTextRegionSeg(unsigned int segNum, bool imm, unsigned int *refSegs, unsigned int nRefSegs)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    JBIG2HuffmanTable runLengthTab[36];
    JBIG2HuffmanTable *symCodeTab = nullptr;
    const JBIG2HuffmanTable *huffFSTable, *huffDSTable, *huffDTTable;
    const JBIG2HuffmanTable *huffRDWTable, *huffRDHTable;
    const JBIG2HuffmanTable *huffRDXTable, *huffRDYTable, *huffRSizeTable;
    JBIG2Segment *seg;
    std::vector<JBIG2Segment *> codeTables;
    JBIG2SymbolDict *symbolDict;
    JBIG2Bitmap **syms;
    unsigned int w, h, x, y, segInfoFlags, extCombOp;
    unsigned int flags, huff, refine, logStrips, refCorner, transposed;
    unsigned int combOp, defPixel, templ;
    int sOffset;
    unsigned int huffFlags, huffFS, huffDS, huffDT;
    unsigned int huffRDW, huffRDH, huffRDX, huffRDY, huffRSize;
    unsigned int numInstances, numSyms, symCodeLen;
    int atx[2], aty[2];
    unsigned int i, k, kk;
    int j = 0;

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        goto eofError;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the text region header
    if (!readUWord(&flags)) {
        goto eofError;
    }
    huff = flags & 1;
    refine = (flags >> 1) & 1;
    logStrips = (flags >> 2) & 3;
    refCorner = (flags >> 4) & 3;
    transposed = (flags >> 6) & 1;
    combOp = (flags >> 7) & 3;
    defPixel = (flags >> 9) & 1;
    sOffset = (flags >> 10) & 0x1f;
    if (sOffset & 0x10) {
        sOffset |= -1 - 0x0f;
    }
    templ = (flags >> 15) & 1;
    huffFS = huffDS = huffDT = 0; // make gcc happy
    huffRDW = huffRDH = huffRDX = huffRDY = huffRSize = 0; // make gcc happy
    if (huff) {
        if (!readUWord(&huffFlags)) {
            goto eofError;
        }
        huffFS = huffFlags & 3;
        huffDS = (huffFlags >> 2) & 3;
        huffDT = (huffFlags >> 4) & 3;
        huffRDW = (huffFlags >> 6) & 3;
        huffRDH = (huffFlags >> 8) & 3;
        huffRDX = (huffFlags >> 10) & 3;
        huffRDY = (huffFlags >> 12) & 3;
        huffRSize = (huffFlags >> 14) & 1;
    }
    if (refine && templ == 0) {
        if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1])) {
            goto eofError;
        }
    }
    if (!readULong(&numInstances)) {
        goto eofError;
    }

    // get symbol dictionaries and tables
    numSyms = 0;
    for (i = 0; i < nRefSegs; ++i) {
        if ((seg = findSegment(refSegs[i]))) {
            if (seg->getType() == jbig2SegSymbolDict) {
                const unsigned int segSize = ((JBIG2SymbolDict *)seg)->getSize();
                if (unlikely(checkedAdd(numSyms, segSize, &numSyms))) {
                    error(errSyntaxError, getPos(), "Too many symbols in JBIG2 text region");
                    return;
                }
            } else if (seg->getType() == jbig2SegCodeTable) {
                codeTables.push_back(seg);
            }
        } else {
            error(errSyntaxError, curStr->getPos(), "Invalid segment reference in JBIG2 text region");
            return;
        }
    }
    i = numSyms;
    if (i <= 1) {
        symCodeLen = huff ? 1 : 0;
    } else {
        --i;
        symCodeLen = 0;
        // i = floor((numSyms-1) / 2^symCodeLen)
        while (i > 0) {
            ++symCodeLen;
            i >>= 1;
        }
    }

    // get the symbol bitmaps
    syms = (JBIG2Bitmap **)gmallocn_checkoverflow(numSyms, sizeof(JBIG2Bitmap *));
    if (numSyms > 0 && !syms) {
        return;
    }
    kk = 0;
    for (i = 0; i < nRefSegs; ++i) {
        if ((seg = findSegment(refSegs[i]))) {
            if (seg->getType() == jbig2SegSymbolDict) {
                symbolDict = (JBIG2SymbolDict *)seg;
                for (k = 0; k < symbolDict->getSize(); ++k) {
                    syms[kk++] = symbolDict->getBitmap(k);
                }
            }
        }
    }

    // get the Huffman tables
    huffFSTable = huffDSTable = huffDTTable = nullptr; // make gcc happy
    huffRDWTable = huffRDHTable = nullptr; // make gcc happy
    huffRDXTable = huffRDYTable = huffRSizeTable = nullptr; // make gcc happy
    i = 0;
    if (huff) {
        if (huffFS == 0) {
            huffFSTable = huffTableF;
        } else if (huffFS == 1) {
            huffFSTable = huffTableG;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffFSTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffDS == 0) {
            huffDSTable = huffTableH;
        } else if (huffDS == 1) {
            huffDSTable = huffTableI;
        } else if (huffDS == 2) {
            huffDSTable = huffTableJ;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffDSTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffDT == 0) {
            huffDTTable = huffTableK;
        } else if (huffDT == 1) {
            huffDTTable = huffTableL;
        } else if (huffDT == 2) {
            huffDTTable = huffTableM;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffDTTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffRDW == 0) {
            huffRDWTable = huffTableN;
        } else if (huffRDW == 1) {
            huffRDWTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffRDWTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffRDH == 0) {
            huffRDHTable = huffTableN;
        } else if (huffRDH == 1) {
            huffRDHTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffRDHTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffRDX == 0) {
            huffRDXTable = huffTableN;
        } else if (huffRDX == 1) {
            huffRDXTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffRDXTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffRDY == 0) {
            huffRDYTable = huffTableN;
        } else if (huffRDY == 1) {
            huffRDYTable = huffTableO;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffRDYTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
        if (huffRSize == 0) {
            huffRSizeTable = huffTableA;
        } else {
            if (i >= codeTables.size()) {
                goto codeTableError;
            }
            huffRSizeTable = ((JBIG2CodeTable *)codeTables[i++])->getHuffTable();
        }
    }

    // symbol ID Huffman decoding table
    if (huff) {
        huffDecoder->reset();
        for (i = 0; i < 32; ++i) {
            runLengthTab[i].val = i;
            runLengthTab[i].prefixLen = huffDecoder->readBits(4);
            runLengthTab[i].rangeLen = 0;
        }
        runLengthTab[32].val = 0x103;
        runLengthTab[32].prefixLen = huffDecoder->readBits(4);
        runLengthTab[32].rangeLen = 2;
        runLengthTab[33].val = 0x203;
        runLengthTab[33].prefixLen = huffDecoder->readBits(4);
        runLengthTab[33].rangeLen = 3;
        runLengthTab[34].val = 0x20b;
        runLengthTab[34].prefixLen = huffDecoder->readBits(4);
        runLengthTab[34].rangeLen = 7;
        runLengthTab[35].prefixLen = 0;
        runLengthTab[35].rangeLen = jbig2HuffmanEOT;
        if (!JBIG2HuffmanDecoder::buildTable(runLengthTab, 35)) {
            huff = false;
        }
    }

    if (huff) {
        symCodeTab = (JBIG2HuffmanTable *)gmallocn_checkoverflow(numSyms + 1, sizeof(JBIG2HuffmanTable));
        if (!symCodeTab) {
            gfree(static_cast<void *>(syms));
            return;
        }
        for (i = 0; i < numSyms; ++i) {
            symCodeTab[i].val = i;
            symCodeTab[i].rangeLen = 0;
        }
        i = 0;
        while (i < numSyms) {
            huffDecoder->decodeInt(&j, runLengthTab);
            if (j > 0x200) {
                for (j -= 0x200; j && i < numSyms; --j) {
                    symCodeTab[i++].prefixLen = 0;
                }
            } else if (j > 0x100) {
                if (unlikely(i == 0)) {
                    symCodeTab[i].prefixLen = 0;
                    ++i;
                }
                for (j -= 0x100; j && i < numSyms; --j) {
                    symCodeTab[i].prefixLen = symCodeTab[i - 1].prefixLen;
                    ++i;
                }
            } else {
                symCodeTab[i++].prefixLen = j;
            }
        }
        symCodeTab[numSyms].prefixLen = 0;
        symCodeTab[numSyms].rangeLen = jbig2HuffmanEOT;
        if (!JBIG2HuffmanDecoder::buildTable(symCodeTab, numSyms)) {
            huff = false;
            gfree(symCodeTab);
            symCodeTab = nullptr;
        }
        huffDecoder->reset();

        // set up the arithmetic decoder
    }

    if (!huff) {
        if (!resetIntStats(symCodeLen)) {
            gfree(static_cast<void *>(syms));
            return;
        }
        arithDecoder->start();
    }
    if (refine) {
        resetRefinementStats(templ, nullptr);
    }

    bitmap = readTextRegion(huff, refine, w, h, numInstances, logStrips, numSyms, symCodeTab, symCodeLen, syms, defPixel, combOp, transposed, refCorner, sOffset, huffFSTable, huffDSTable, huffDTTable, huffRDWTable, huffRDHTable,
                            huffRDXTable, huffRDYTable, huffRSizeTable, templ, atx, aty);

    gfree(static_cast<void *>(syms));

    if (bitmap) {
        // combine the region bitmap into the page bitmap
        if (imm) {
            if (pageH == 0xffffffff && y + h > curPageH) {
                pageBitmap->expand(y + h, pageDefPixel);
            }
            pageBitmap->combine(bitmap.get(), x, y, extCombOp);

            // store the region bitmap
        } else {
            bitmap->setSegNum(segNum);
            segments.push_back(std::move(bitmap));
        }
    }

    // clean up the Huffman decoder
    if (huff) {
        gfree(symCodeTab);
    }

    return;

codeTableError:
    error(errSyntaxError, curStr->getPos(), "Missing code table in JBIG2 text region");
    gfree(static_cast<void *>(syms));
    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readTextRegion(bool huff, bool refine, int w, int h, unsigned int numInstances, unsigned int logStrips, unsigned int numSyms, const JBIG2HuffmanTable *symCodeTab, unsigned int symCodeLen,
                                                         JBIG2Bitmap **syms, unsigned int defPixel, unsigned int combOp, unsigned int transposed, unsigned int refCorner, int sOffset, const JBIG2HuffmanTable *huffFSTable,
                                                         const JBIG2HuffmanTable *huffDSTable, const JBIG2HuffmanTable *huffDTTable, const JBIG2HuffmanTable *huffRDWTable, const JBIG2HuffmanTable *huffRDHTable,
                                                         const JBIG2HuffmanTable *huffRDXTable, const JBIG2HuffmanTable *huffRDYTable, const JBIG2HuffmanTable *huffRSizeTable, unsigned int templ, int *atx, int *aty)
{
    JBIG2Bitmap *symbolBitmap;
    unsigned int strips;
    int t = 0, dt = 0, tt, s, ds = 0, sFirst, j = 0;
    int rdw, rdh, rdx, rdy, ri = 0, refDX, refDY, bmSize;
    unsigned int symID, inst, bw, bh;

    strips = 1 << logStrips;

    // allocate the bitmap
    std::unique_ptr<JBIG2Bitmap> bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    if (defPixel) {
        bitmap->clearToOne();
    } else {
        bitmap->clearToZero();
    }

    // decode initial T value
    if (huff) {
        huffDecoder->decodeInt(&t, huffDTTable);
    } else {
        arithDecoder->decodeInt(&t, iadtStats);
    }

    if (checkedMultiply(t, -(int)strips, &t)) {
        return {};
    }

    inst = 0;
    sFirst = 0;
    while (inst < numInstances) {

        // decode delta-T
        if (huff) {
            huffDecoder->decodeInt(&dt, huffDTTable);
        } else {
            arithDecoder->decodeInt(&dt, iadtStats);
        }
        t += dt * strips;

        // first S value
        if (huff) {
            huffDecoder->decodeInt(&ds, huffFSTable);
        } else {
            arithDecoder->decodeInt(&ds, iafsStats);
        }
        if (unlikely(checkedAdd(sFirst, ds, &sFirst))) {
            return nullptr;
        }
        s = sFirst;

        // read the instances
        // (this loop test is here to avoid an infinite loop with damaged
        // JBIG2 streams where the normal loop exit doesn't get triggered)
        while (inst < numInstances) {

            // T value
            if (strips == 1) {
                dt = 0;
            } else if (huff) {
                dt = huffDecoder->readBits(logStrips);
            } else {
                arithDecoder->decodeInt(&dt, iaitStats);
            }
            if (unlikely(checkedAdd(t, dt, &tt))) {
                return nullptr;
            }

            // symbol ID
            if (huff) {
                if (symCodeTab) {
                    huffDecoder->decodeInt(&j, symCodeTab);
                    symID = (unsigned int)j;
                } else {
                    symID = huffDecoder->readBits(symCodeLen);
                }
            } else {
                if (iaidStats == nullptr) {
                    return nullptr;
                }
                symID = arithDecoder->decodeIAID(symCodeLen, iaidStats);
            }

            if (symID >= numSyms) {
                error(errSyntaxError, curStr->getPos(), "Invalid symbol number in JBIG2 text region");
                if (unlikely(numInstances - inst > 0x800)) {
                    // don't loop too often with damaged JBIg2 streams
                    return nullptr;
                }
            } else {

                // get the symbol bitmap
                symbolBitmap = nullptr;
                if (refine) {
                    if (huff) {
                        ri = (int)huffDecoder->readBit();
                    } else {
                        arithDecoder->decodeInt(&ri, iariStats);
                    }
                } else {
                    ri = 0;
                }
                if (ri) {
                    bool decodeSuccess;
                    if (huff) {
                        decodeSuccess = huffDecoder->decodeInt(&rdw, huffRDWTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdh, huffRDHTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdx, huffRDXTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&rdy, huffRDYTable);
                        decodeSuccess = decodeSuccess && huffDecoder->decodeInt(&bmSize, huffRSizeTable);
                        huffDecoder->reset();
                        arithDecoder->start();
                    } else {
                        decodeSuccess = arithDecoder->decodeInt(&rdw, iardwStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdh, iardhStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdx, iardxStats);
                        decodeSuccess = decodeSuccess && arithDecoder->decodeInt(&rdy, iardyStats);
                    }

                    if (decodeSuccess && syms[symID]) {
                        if (checkedAdd(((rdw >= 0) ? rdw : rdw - 1) / 2, rdx, &refDX)) {
                            return nullptr;
                        }
                        if (checkedAdd(((rdh >= 0) ? rdh : rdh - 1) / 2, rdy, &refDY)) {
                            return nullptr;
                        }
                        int refinementSectionW, refinementSectionH;
                        if (checkedAdd(rdw, syms[symID]->getWidth(), &refinementSectionW)) {
                            return nullptr;
                        }
                        if (checkedAdd(rdh, syms[symID]->getHeight(), &refinementSectionH)) {
                            return nullptr;
                        }

                        symbolBitmap = readGenericRefinementRegion(refinementSectionW, refinementSectionH, templ, false, syms[symID], refDX, refDY, atx, aty).release();
                    }
                    //~ do we need to use the bmSize value here (in Huffman mode)?
                } else {
                    symbolBitmap = syms[symID];
                }

                if (symbolBitmap) {
                    // combine the symbol bitmap into the region bitmap
                    //~ something is wrong here - refCorner shouldn't degenerate into
                    //~   two cases
                    bw = symbolBitmap->getWidth() - 1;
                    if (unlikely(symbolBitmap->getHeight() == 0)) {
                        error(errSyntaxError, curStr->getPos(), "Invalid symbol bitmap height");
                        if (ri) {
                            delete symbolBitmap;
                        }
                        return nullptr;
                    }
                    bh = symbolBitmap->getHeight() - 1;
                    if (transposed) {
                        if (unlikely(s > 2 * bitmap->getHeight())) {
                            error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                            if (ri) {
                                delete symbolBitmap;
                            }
                            return nullptr;
                        }
                        switch (refCorner) {
                        case 0: // bottom left
                            bitmap->combine(symbolBitmap, tt, s, combOp);
                            break;
                        case 1: // top left
                            bitmap->combine(symbolBitmap, tt, s, combOp);
                            break;
                        case 2: // bottom right
                            bitmap->combine(symbolBitmap, tt - bw, s, combOp);
                            break;
                        case 3: // top right
                            bitmap->combine(symbolBitmap, tt - bw, s, combOp);
                            break;
                        }
                        s += bh;
                    } else {
                        switch (refCorner) {
                        case 0: // bottom left
                            if (unlikely(tt - (int)bh > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(symbolBitmap, s, tt - bh, combOp);
                            break;
                        case 1: // top left
                            if (unlikely(tt > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(symbolBitmap, s, tt, combOp);
                            break;
                        case 2: // bottom right
                            if (unlikely(tt - (int)bh > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(symbolBitmap, s, tt - bh, combOp);
                            break;
                        case 3: // top right
                            if (unlikely(tt > 2 * bitmap->getHeight())) {
                                error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 combine");
                                if (ri) {
                                    delete symbolBitmap;
                                }
                                return nullptr;
                            }
                            bitmap->combine(symbolBitmap, s, tt, combOp);
                            break;
                        }
                        s += bw;
                    }
                    if (ri) {
                        delete symbolBitmap;
                    }
                } else {
                    // NULL symbolBitmap only happens on error
                    return nullptr;
                }
            }

            // next instance
            ++inst;

            // next S value
            if (huff) {
                if (!huffDecoder->decodeInt(&ds, huffDSTable)) {
                    break;
                }
            } else {
                if (!arithDecoder->decodeInt(&ds, iadsStats)) {
                    break;
                }
            }
            if (checkedAdd(s, sOffset + ds, &s)) {
                return nullptr;
            }
        }
    }

    return bitmap;
}

void JBIG2Stream::readPatternDictSeg(unsigned int segNum, unsigned int length)
{
    std::unique_ptr<JBIG2PatternDict> patternDict;
    std::unique_ptr<JBIG2Bitmap> bitmap;
    unsigned int flags, patternW, patternH, grayMax, templ, mmr;
    int atx[4], aty[4];
    unsigned int i, x;

    // halftone dictionary flags, pattern width and height, max gray value
    if (!readUByte(&flags) || !readUByte(&patternW) || !readUByte(&patternH) || !readULong(&grayMax)) {
        goto eofError;
    }
    templ = (flags >> 1) & 3;
    mmr = flags & 1;

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // read the bitmap
    atx[0] = -(int)patternW;
    aty[0] = 0;
    atx[1] = -3;
    aty[1] = -1;
    atx[2] = 2;
    aty[2] = -2;
    atx[3] = -2;
    aty[3] = -2;

    unsigned int grayMaxPlusOne;
    if (unlikely(checkedAdd(grayMax, 1U, &grayMaxPlusOne))) {
        return;
    }
    unsigned int bitmapW;
    if (unlikely(checkedMultiply(grayMaxPlusOne, patternW, &bitmapW))) {
        return;
    }
    if (bitmapW >= INT_MAX) {
        return;
    }
    bitmap = readGenericBitmap(mmr, static_cast<int>(bitmapW), patternH, templ, false, false, nullptr, atx, aty, length - 7);

    if (!bitmap) {
        return;
    }

    // create the pattern dict object
    patternDict = std::make_unique<JBIG2PatternDict>(segNum, grayMax + 1);

    // split up the bitmap
    x = 0;
    for (i = 0; i <= grayMax && i < patternDict->getSize(); ++i) {
        patternDict->setBitmap(i, bitmap->getSlice(x, 0, patternW, patternH));
        x += patternW;
    }

    // store the new pattern dict
    segments.push_back(std::move(patternDict));

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

void JBIG2Stream::readHalftoneRegionSeg(unsigned int segNum, bool imm, unsigned int *refSegs, unsigned int nRefSegs)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    JBIG2Segment *seg;
    JBIG2PatternDict *patternDict;
    std::unique_ptr<JBIG2Bitmap> skipBitmap;
    unsigned int *grayImg;
    JBIG2Bitmap *patternBitmap;
    unsigned int w, h, x, y, segInfoFlags, extCombOp;
    unsigned int flags, mmr, templ, enableSkip, combOp;
    unsigned int gridW, gridH, stepX, stepY, patW, patH;
    int atx[4], aty[4];
    int gridX, gridY, xx, yy, j;
    unsigned int bpp, m, n, i;

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        goto eofError;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the halftone region header
    if (!readUByte(&flags)) {
        goto eofError;
    }
    mmr = flags & 1;
    templ = (flags >> 1) & 3;
    enableSkip = (flags >> 3) & 1;
    combOp = (flags >> 4) & 7;
    if (!readULong(&gridW) || !readULong(&gridH) || !readLong(&gridX) || !readLong(&gridY) || !readUWord(&stepX) || !readUWord(&stepY)) {
        goto eofError;
    }
    if (w == 0 || h == 0 || w >= INT_MAX / h) {
        error(errSyntaxError, curStr->getPos(), "Bad bitmap size in JBIG2 halftone segment");
        return;
    }
    if (gridH == 0 || gridW >= INT_MAX / gridH) {
        error(errSyntaxError, curStr->getPos(), "Bad grid size in JBIG2 halftone segment");
        return;
    }

    // get pattern dictionary
    if (nRefSegs != 1) {
        error(errSyntaxError, curStr->getPos(), "Bad symbol dictionary reference in JBIG2 halftone segment");
        return;
    }
    seg = findSegment(refSegs[0]);
    if (seg == nullptr || seg->getType() != jbig2SegPatternDict) {
        error(errSyntaxError, curStr->getPos(), "Bad symbol dictionary reference in JBIG2 halftone segment");
        return;
    }

    patternDict = (JBIG2PatternDict *)seg;
    i = patternDict->getSize();
    if (i <= 1) {
        bpp = 0;
    } else {
        --i;
        bpp = 0;
        // i = floor((size-1) / 2^bpp)
        while (i > 0) {
            ++bpp;
            i >>= 1;
        }
    }
    patternBitmap = patternDict->getBitmap(0);
    if (unlikely(patternBitmap == nullptr)) {
        error(errSyntaxError, curStr->getPos(), "Bad pattern bitmap");
        return;
    }
    patW = patternBitmap->getWidth();
    patH = patternBitmap->getHeight();

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // allocate the bitmap
    bitmap = std::make_unique<JBIG2Bitmap>(segNum, w, h);
    if (flags & 0x80) { // HDEFPIXEL
        bitmap->clearToOne();
    } else {
        bitmap->clearToZero();
    }

    // compute the skip bitmap
    if (enableSkip) {
        skipBitmap = std::make_unique<JBIG2Bitmap>(0, gridW, gridH);
        skipBitmap->clearToZero();
        for (m = 0; m < gridH; ++m) {
            for (n = 0; n < gridW; ++n) {
                xx = gridX + m * stepY + n * stepX;
                yy = gridY + m * stepX - n * stepY;
                if (((xx + (int)patW) >> 8) <= 0 || (xx >> 8) >= (int)w || ((yy + (int)patH) >> 8) <= 0 || (yy >> 8) >= (int)h) {
                    skipBitmap->setPixel(n, m);
                }
            }
        }
    }

    // read the gray-scale image
    grayImg = (unsigned int *)gmallocn_checkoverflow(gridW * gridH, sizeof(unsigned int));
    if (!grayImg) {
        return;
    }
    memset(grayImg, 0, gridW * gridH * sizeof(unsigned int));
    atx[0] = templ <= 1 ? 3 : 2;
    aty[0] = -1;
    atx[1] = -3;
    aty[1] = -1;
    atx[2] = 2;
    aty[2] = -2;
    atx[3] = -2;
    aty[3] = -2;
    for (j = bpp - 1; j >= 0; --j) {
        const std::unique_ptr<JBIG2Bitmap> grayBitmap = readGenericBitmap(mmr, gridW, gridH, templ, false, enableSkip, skipBitmap.get(), atx, aty, -1);
        if (!grayBitmap) {
            gfree(grayImg);
            return;
        }
        i = 0;
        for (m = 0; m < gridH; ++m) {
            for (n = 0; n < gridW; ++n) {
                const int bit = grayBitmap->getPixel(n, m) ^ (grayImg[i] & 1);
                grayImg[i] = (grayImg[i] << 1) | bit;
                ++i;
            }
        }
    }

    // decode the image
    i = 0;
    for (m = 0; m < gridH; ++m) {
        xx = gridX + m * stepY;
        yy = gridY + m * stepX;
        for (n = 0; n < gridW; ++n) {
            if (!(enableSkip && skipBitmap->getPixel(n, m))) {
                patternBitmap = patternDict->getBitmap(grayImg[i]);
                if (unlikely(patternBitmap == nullptr)) {
                    gfree(grayImg);
                    error(errSyntaxError, curStr->getPos(), "Bad pattern bitmap");
                    return;
                }
                bitmap->combine(patternBitmap, xx >> 8, yy >> 8, combOp);
            }
            xx += stepX;
            yy -= stepY;
            ++i;
        }
    }

    gfree(grayImg);

    // combine the region bitmap into the page bitmap
    if (imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
        }
        pageBitmap->combine(bitmap.get(), x, y, extCombOp);

        // store the region bitmap
    } else {
        segments.push_back(std::move(bitmap));
    }

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

void JBIG2Stream::readGenericRegionSeg(unsigned int segNum, bool imm, unsigned int length)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    unsigned int w, h, x, y, segInfoFlags, extCombOp, rowCount;
    unsigned int flags, mmr, templ, tpgdOn;
    int atx[4], aty[4];

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        goto eofError;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the generic region segment header
    if (!readUByte(&flags)) {
        goto eofError;
    }
    mmr = flags & 1;
    templ = (flags >> 1) & 3;
    tpgdOn = (flags >> 3) & 1;

    // AT flags
    if (!mmr) {
        if (templ == 0) {
            if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1]) || !readByte(&atx[2]) || !readByte(&aty[2]) || !readByte(&atx[3]) || !readByte(&aty[3])) {
                goto eofError;
            }
        } else {
            if (!readByte(&atx[0]) || !readByte(&aty[0])) {
                goto eofError;
            }
        }
    }

    // set up the arithmetic decoder
    if (!mmr) {
        resetGenericStats(templ, nullptr);
        arithDecoder->start();
    }

    // read the bitmap
    bitmap = readGenericBitmap(mmr, w, h, templ, tpgdOn, false, nullptr, atx, aty, mmr ? length - 18 : 0);
    if (!bitmap) {
        return;
    }

    // combine the region bitmap into the page bitmap
    if (imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
            if (!pageBitmap->isOk()) {
                error(errSyntaxError, curStr->getPos(), "JBIG2Stream::readGenericRegionSeg: expand failed");
                return;
            }
        }
        pageBitmap->combine(bitmap.get(), x, y, extCombOp);

        // store the region bitmap
    } else {
        bitmap->setSegNum(segNum);
        segments.push_back(std::move(bitmap));
    }

    // immediate generic segments can have an unspecified length, in
    // which case, a row count is stored at the end of the segment
    if (imm && length == 0xffffffff) {
        readULong(&rowCount);
    }

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

inline void JBIG2Stream::mmrAddPixels(int a1, int blackPixels, int *codingLine, int *a0i, int w)
{
    if (a1 > codingLine[*a0i]) {
        if (a1 > w) {
            error(errSyntaxError, curStr->getPos(), "JBIG2 MMR row is wrong length ({0:d})", a1);
            a1 = w;
        }
        if ((*a0i & 1) ^ blackPixels) {
            ++*a0i;
        }
        codingLine[*a0i] = a1;
    }
}

inline void JBIG2Stream::mmrAddPixelsNeg(int a1, int blackPixels, int *codingLine, int *a0i, int w)
{
    if (a1 > codingLine[*a0i]) {
        if (a1 > w) {
            error(errSyntaxError, curStr->getPos(), "JBIG2 MMR row is wrong length ({0:d})", a1);
            a1 = w;
        }
        if ((*a0i & 1) ^ blackPixels) {
            ++*a0i;
        }
        codingLine[*a0i] = a1;
    } else if (a1 < codingLine[*a0i]) {
        if (a1 < 0) {
            error(errSyntaxError, curStr->getPos(), "Invalid JBIG2 MMR code");
            a1 = 0;
        }
        while (*a0i > 0 && a1 <= codingLine[*a0i - 1]) {
            --*a0i;
        }
        codingLine[*a0i] = a1;
    }
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readGenericBitmap(bool mmr, int w, int h, int templ, bool tpgdOn, bool useSkip, JBIG2Bitmap *skip, int *atx, int *aty, int mmrDataLength)
{
    auto bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    bitmap->clearToZero();

    //----- MMR decode

    if (mmr) {

        mmrDecoder->reset();
        // 0 <= codingLine[0] < codingLine[1] < ... < codingLine[n] = w
        // ---> max codingLine size = w + 1
        // refLine has one extra guard entry at the end
        // ---> max refLine size = w + 2
        int *codingLine = (int *)gmallocn_checkoverflow(w + 1, sizeof(int));
        int *refLine = (int *)gmallocn_checkoverflow(w + 2, sizeof(int));

        if (unlikely(!codingLine || !refLine)) {
            gfree(codingLine);
            error(errSyntaxError, curStr->getPos(), "Bad width in JBIG2 generic bitmap");
            return nullptr;
        }

        memset(refLine, 0, (w + 2) * sizeof(int));
        for (int i = 0; i < w + 1; ++i) {
            codingLine[i] = w;
        }

        for (int y = 0; y < h; ++y) {

            // copy coding line to ref line
            {
                int i;
                for (i = 0; codingLine[i] < w; ++i) {
                    refLine[i] = codingLine[i];
                }
                refLine[i++] = w;
                refLine[i] = w;
            }

            // decode a line
            codingLine[0] = 0;
            int a0i = 0;
            int b1i = 0;
            int blackPixels = 0;
            // invariant:
            // refLine[b1i-1] <= codingLine[a0i] < refLine[b1i] < refLine[b1i+1] <= w
            // exception at left edge:
            //   codingLine[a0i = 0] = refLine[b1i = 0] = 0 is possible
            // exception at right edge:
            //   refLine[b1i] = refLine[b1i+1] = w is possible
            while (codingLine[a0i] < w) {
                switch (mmrDecoder->get2DCode()) {
                case twoDimPass:
                    if (unlikely(b1i + 1 >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i + 1], blackPixels, codingLine, &a0i, w);
                    if (refLine[b1i + 1] < w) {
                        b1i += 2;
                    }
                    break;
                case twoDimHoriz: {
                    int code1 = 0;
                    int code2 = 0;
                    if (blackPixels) {
                        int code3;
                        do {
                            code1 += code3 = mmrDecoder->getBlackCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = mmrDecoder->getWhiteCode();
                        } while (code3 >= 64);
                    } else {
                        int code3;
                        do {
                            code1 += code3 = mmrDecoder->getWhiteCode();
                        } while (code3 >= 64);
                        do {
                            code2 += code3 = mmrDecoder->getBlackCode();
                        } while (code3 >= 64);
                    }
                    mmrAddPixels(codingLine[a0i] + code1, blackPixels, codingLine, &a0i, w);
                    if (codingLine[a0i] < w) {
                        mmrAddPixels(codingLine[a0i] + code2, blackPixels ^ 1, codingLine, &a0i, w);
                    }
                    while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                        b1i += 2;
                    }
                    break;
                }
                case twoDimVertR3:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 3, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertR2:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 2, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertR1:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i] + 1, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVert0:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixels(refLine[b1i], blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        ++b1i;
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL3:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 3, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL2:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 2, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case twoDimVertL1:
                    if (unlikely(b1i >= w + 2)) {
                        break;
                    }
                    mmrAddPixelsNeg(refLine[b1i] - 1, blackPixels, codingLine, &a0i, w);
                    blackPixels ^= 1;
                    if (codingLine[a0i] < w) {
                        if (b1i > 0) {
                            --b1i;
                        } else {
                            ++b1i;
                        }
                        while (likely(b1i < w + 2) && refLine[b1i] <= codingLine[a0i] && refLine[b1i] < w) {
                            b1i += 2;
                        }
                    }
                    break;
                case EOF:
                    mmrAddPixels(w, 0, codingLine, &a0i, w);
                    break;
                default:
                    error(errSyntaxError, curStr->getPos(), "Illegal code in JBIG2 MMR bitmap data");
                    mmrAddPixels(w, 0, codingLine, &a0i, w);
                    break;
                }
            }

            // convert the run lengths to a bitmap line
            {
                int i = 0;
                while (true) {
                    for (int x = codingLine[i]; x < codingLine[i + 1]; ++x) {
                        bitmap->setPixel(x, y);
                    }
                    if (codingLine[i + 1] >= w || codingLine[i + 2] >= w) {
                        break;
                    }
                    i += 2;
                }
            }
        }

        if (mmrDataLength >= 0) {
            mmrDecoder->skipTo(mmrDataLength);
        } else {
            if (mmrDecoder->get24Bits() != 0x001001) {
                error(errSyntaxError, curStr->getPos(), "Missing EOFB in JBIG2 MMR bitmap data");
            }
        }

        gfree(refLine);
        gfree(codingLine);

        //----- arithmetic decode

    } else {
        // set up the typical row context
        const unsigned int ltpCX = [tpgdOn, templ] {
            if (tpgdOn) {
                switch (templ) {
                case 0:
                    return 0x3953; // 001 11001 0101 0011
                    break;
                case 1:
                    return 0x079a; // 0011 11001 101 0
                    break;
                case 2:
                    return 0x0e3; // 001 1100 01 1
                    break;
                case 3:
                    return 0x18b; // 01100 0101 1
                    break;
                }
            }
            return 0;
        }();

        bool ltp = false;
        for (int y = 0; y < h; ++y) {

            // check for a "typical" (duplicate) row
            if (tpgdOn) {
                if (arithDecoder->decodeBit(ltpCX, genericRegionStats)) {
                    ltp = !ltp;
                }
                if (ltp) {
                    if (y > 0) {
                        bitmap->duplicateRow(y, y - 1);
                    }
                    continue;
                }
            }

            switch (templ) {
            case 0: {

                // set up the context
                unsigned char *p0, *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf0, buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                    if (y >= 2) {
                        p0 = bitmap->getDataPtr() + (y - 2) * bitmap->getLineSize();
                        buf0 = *p0++ << 8;
                    } else {
                        p0 = nullptr;
                        buf0 = 0;
                    }
                } else {
                    p1 = p0 = nullptr;
                    buf1 = buf0 = 0;
                }

                if (atx[0] >= -8 && atx[0] <= 8 && atx[1] >= -8 && atx[1] <= 8 && atx[2] >= -8 && atx[2] <= 8 && atx[3] >= -8 && atx[3] <= 8) {
                    // set up the adaptive context
                    unsigned int atBuf0, atBuf1, atBuf2, atBuf3;
                    unsigned char *atP0, *atP1, *atP2, *atP3;
                    if (y + aty[0] >= 0 && y + aty[0] < bitmap->getHeight()) {
                        atP0 = bitmap->getDataPtr() + (y + aty[0]) * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];
                    if (y + aty[1] >= 0 && y + aty[1] < bitmap->getHeight()) {
                        atP1 = bitmap->getDataPtr() + (y + aty[1]) * bitmap->getLineSize();
                        atBuf1 = *atP1++ << 8;
                    } else {
                        atP1 = nullptr;
                        atBuf1 = 0;
                    }
                    const int atShift1 = 15 - atx[1];
                    if (y + aty[2] >= 0 && y + aty[2] < bitmap->getHeight()) {
                        atP2 = bitmap->getDataPtr() + (y + aty[2]) * bitmap->getLineSize();
                        atBuf2 = *atP2++ << 8;
                    } else {
                        atP2 = nullptr;
                        atBuf2 = 0;
                    }
                    const int atShift2 = 15 - atx[2];
                    if (y + aty[3] >= 0 && y + aty[3] < bitmap->getHeight()) {
                        atP3 = bitmap->getDataPtr() + (y + aty[3]) * bitmap->getLineSize();
                        atBuf3 = *atP3++ << 8;
                    } else {
                        atP3 = nullptr;
                        atBuf3 = 0;
                    }
                    const int atShift3 = 15 - atx[3];

                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                            if (atP0) {
                                atBuf0 |= *atP0++;
                            }
                            if (atP1) {
                                atBuf1 |= *atP1++;
                            }
                            if (atP2) {
                                atBuf2 |= *atP2++;
                            }
                            if (atP3) {
                                atBuf3 |= *atP3++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx0 << 13) | (cx1 << 8) | (cx2 << 4) | (((atBuf0 >> atShift0) & 1) << 3) | (((atBuf1 >> atShift1) & 1) << 2) | (((atBuf2 >> atShift2) & 1) << 1) | ((atBuf3 >> atShift3) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                    if (aty[1] == 0) {
                                        atBuf1 |= 0x8000;
                                    }
                                    if (aty[2] == 0) {
                                        atBuf2 |= 0x8000;
                                    }
                                    if (aty[3] == 0) {
                                        atBuf3 |= 0x8000;
                                    }
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                            atBuf1 <<= 1;
                            atBuf2 <<= 1;
                            atBuf3 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx0 << 13) | (cx1 << 8) | (cx2 << 4) | (bitmap->getPixel(x + atx[0], y + aty[0]) << 3) | (bitmap->getPixel(x + atx[1], y + aty[1]) << 2) | (bitmap->getPixel(x + atx[2], y + aty[2]) << 1)
                                    | bitmap->getPixel(x + atx[3], y + aty[3]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
            case 1: {

                // set up the context
                unsigned char *p0, *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf0, buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                    if (y >= 2) {
                        p0 = bitmap->getDataPtr() + (y - 2) * bitmap->getLineSize();
                        buf0 = *p0++ << 8;
                    } else {
                        p0 = nullptr;
                        buf0 = 0;
                    }
                } else {
                    p1 = p0 = nullptr;
                    buf1 = buf0 = 0;
                }

                if (atx[0] >= -8 && atx[0] <= 8) {
                    // set up the adaptive context
                    const int atY = y + aty[0];
                    unsigned int atBuf0;
                    unsigned char *atP0;
                    if ((atY >= 0) && (atY < bitmap->getHeight())) {
                        atP0 = bitmap->getDataPtr() + atY * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];

                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                            if (atP0) {
                                atBuf0 |= *atP0++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 13) & 0x0f;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x07;
                            const unsigned int cx = (cx0 << 9) | (cx1 << 4) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                }
                                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                                    return nullptr;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 13) & 0x0f;
                            const unsigned int cx1 = (buf1 >> 13) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x07;
                            const unsigned int cx = (cx0 << 9) | (cx1 << 4) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
            case 2: {
                // set up the context
                unsigned char *p0, *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf0, buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                    if (y >= 2) {
                        p0 = bitmap->getDataPtr() + (y - 2) * bitmap->getLineSize();
                        buf0 = *p0++ << 8;
                    } else {
                        p0 = nullptr;
                        buf0 = 0;
                    }
                } else {
                    p1 = p0 = nullptr;
                    buf1 = buf0 = 0;
                }

                if (atx[0] >= -8 && atx[0] <= 8) {
                    // set up the adaptive context
                    const int atY = y + aty[0];
                    unsigned int atBuf0;
                    unsigned char *atP0;
                    if ((atY >= 0) && (atY < bitmap->getHeight())) {
                        atP0 = bitmap->getDataPtr() + atY * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];

                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                            if (atP0) {
                                atBuf0 |= *atP0++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 14) & 0x0f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x03;
                            const unsigned int cx = (cx0 << 7) | (cx1 << 3) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p0) {
                                buf0 |= *p0++;
                            }
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx0 = (buf0 >> 14) & 0x07;
                            const unsigned int cx1 = (buf1 >> 14) & 0x0f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x03;
                            const unsigned int cx = (cx0 << 7) | (cx1 << 3) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                            }

                            // update the context
                            buf0 <<= 1;
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
            case 3:

                // set up the context
                unsigned char *p1, *p2, *pp;
                p2 = pp = bitmap->getDataPtr() + y * bitmap->getLineSize();
                unsigned int buf1;
                unsigned int buf2 = *p2++ << 8;
                if (y >= 1) {
                    p1 = bitmap->getDataPtr() + (y - 1) * bitmap->getLineSize();
                    buf1 = *p1++ << 8;
                } else {
                    p1 = nullptr;
                    buf1 = 0;
                }

                if (atx[0] >= -8 && atx[0] <= 8) {
                    // set up the adaptive context
                    const int atY = y + aty[0];
                    unsigned int atBuf0;
                    unsigned char *atP0;
                    if ((atY >= 0) && (atY < bitmap->getHeight())) {
                        atP0 = bitmap->getDataPtr() + atY * bitmap->getLineSize();
                        atBuf0 = *atP0++ << 8;
                    } else {
                        atP0 = nullptr;
                        atBuf0 = 0;
                    }
                    const int atShift0 = 15 - atx[0];

                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                            if (atP0) {
                                atBuf0 |= *atP0++;
                            }
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx1 = (buf1 >> 14) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx1 << 5) | (cx2 << 1) | ((atBuf0 >> atShift0) & 1);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                    if (aty[0] == 0) {
                                        atBuf0 |= 0x8000;
                                    }
                                }
                            }

                            // update the context
                            buf1 <<= 1;
                            buf2 <<= 1;
                            atBuf0 <<= 1;
                        }
                    }

                } else {
                    // decode the row
                    for (int x0 = 0, x = 0; x0 < w; x0 += 8, ++pp) {
                        if (x0 + 8 < w) {
                            if (p1) {
                                buf1 |= *p1++;
                            }
                            buf2 |= *p2++;
                        }
                        unsigned char mask = 0x80;
                        for (int x1 = 0; x1 < 8 && x < w; ++x1, ++x, mask >>= 1) {

                            // build the context
                            const unsigned int cx1 = (buf1 >> 14) & 0x1f;
                            const unsigned int cx2 = (buf2 >> 16) & 0x0f;
                            const unsigned int cx = (cx1 << 5) | (cx2 << 1) | bitmap->getPixel(x + atx[0], y + aty[0]);

                            // check for a skipped pixel
                            if (!(useSkip && skip->getPixel(x, y))) {

                                // decode the pixel
                                if (arithDecoder->decodeBit(cx, genericRegionStats)) {
                                    *pp |= mask;
                                    buf2 |= 0x8000;
                                }
                            }

                            // update the context
                            buf1 <<= 1;
                            buf2 <<= 1;
                        }
                    }
                }
                break;
            }
        }
    }

    return bitmap;
}

void JBIG2Stream::readGenericRefinementRegionSeg(unsigned int segNum, bool imm, unsigned int *refSegs, unsigned int nRefSegs)
{
    std::unique_ptr<JBIG2Bitmap> bitmap;
    JBIG2Bitmap *refBitmap;
    unsigned int w, h, x, y, segInfoFlags, extCombOp;
    unsigned int flags, templ, tpgrOn;
    int atx[2], aty[2];
    JBIG2Segment *seg;

    // region segment info field
    if (!readULong(&w) || !readULong(&h) || !readULong(&x) || !readULong(&y) || !readUByte(&segInfoFlags)) {
        goto eofError;
    }
    extCombOp = segInfoFlags & 7;

    // rest of the generic refinement region segment header
    if (!readUByte(&flags)) {
        goto eofError;
    }
    templ = flags & 1;
    tpgrOn = (flags >> 1) & 1;

    // AT flags
    if (!templ) {
        if (!readByte(&atx[0]) || !readByte(&aty[0]) || !readByte(&atx[1]) || !readByte(&aty[1])) {
            goto eofError;
        }
    }

    // resize the page bitmap if needed
    if (nRefSegs == 0 || imm) {
        if (pageH == 0xffffffff && y + h > curPageH) {
            pageBitmap->expand(y + h, pageDefPixel);
        }
    }

    // get referenced bitmap
    if (nRefSegs > 1) {
        error(errSyntaxError, curStr->getPos(), "Bad reference in JBIG2 generic refinement segment");
        return;
    }
    if (nRefSegs == 1) {
        seg = findSegment(refSegs[0]);
        if (seg == nullptr || seg->getType() != jbig2SegBitmap) {
            error(errSyntaxError, curStr->getPos(), "Bad bitmap reference in JBIG2 generic refinement segment");
            return;
        }
        refBitmap = (JBIG2Bitmap *)seg;
    } else {
        refBitmap = pageBitmap->getSlice(x, y, w, h);
    }

    // set up the arithmetic decoder
    resetRefinementStats(templ, nullptr);
    arithDecoder->start();

    // read
    bitmap = readGenericRefinementRegion(w, h, templ, tpgrOn, refBitmap, 0, 0, atx, aty);

    // combine the region bitmap into the page bitmap
    if (imm && bitmap) {
        pageBitmap->combine(bitmap.get(), x, y, extCombOp);

        // store the region bitmap
    } else {
        if (bitmap) {
            bitmap->setSegNum(segNum);
            segments.push_back(std::move(bitmap));
        } else {
            error(errSyntaxError, curStr->getPos(), "readGenericRefinementRegionSeg with null bitmap");
        }
    }

    // delete the referenced bitmap
    if (nRefSegs == 1) {
        discardSegment(refSegs[0]);
    } else {
        delete refBitmap;
    }

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

std::unique_ptr<JBIG2Bitmap> JBIG2Stream::readGenericRefinementRegion(int w, int h, int templ, bool tpgrOn, JBIG2Bitmap *refBitmap, int refDX, int refDY, int *atx, int *aty)
{
    bool ltp;
    unsigned int ltpCX, cx, cx0, cx2, cx3, cx4, tpgrCX0, tpgrCX1, tpgrCX2;
    JBIG2BitmapPtr cxPtr0 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr1 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr2 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr3 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr4 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr5 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr cxPtr6 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr0 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr1 = { .p = nullptr, .shift = 0, .x = 0 };
    JBIG2BitmapPtr tpgrCXPtr2 = { .p = nullptr, .shift = 0, .x = 0 };
    int x, y, pix;

    if (!refBitmap) {
        return nullptr;
    }

    auto bitmap = std::make_unique<JBIG2Bitmap>(0, w, h);
    if (!bitmap->isOk()) {
        return nullptr;
    }
    bitmap->clearToZero();

    // set up the typical row context
    if (templ) {
        ltpCX = 0x008;
    } else {
        ltpCX = 0x0010;
    }

    ltp = false;
    for (y = 0; y < h; ++y) {

        if (templ) {

            // set up the context
            bitmap->getPixelPtr(0, y - 1, &cxPtr0);
            cx0 = bitmap->nextPixel(&cxPtr0);
            bitmap->getPixelPtr(-1, y, &cxPtr1);
            refBitmap->getPixelPtr(-refDX, y - 1 - refDY, &cxPtr2);
            refBitmap->getPixelPtr(-1 - refDX, y - refDY, &cxPtr3);
            cx3 = refBitmap->nextPixel(&cxPtr3);
            cx3 = (cx3 << 1) | refBitmap->nextPixel(&cxPtr3);
            refBitmap->getPixelPtr(-refDX, y + 1 - refDY, &cxPtr4);
            cx4 = refBitmap->nextPixel(&cxPtr4);

            // set up the typical prediction context
            tpgrCX0 = tpgrCX1 = tpgrCX2 = 0; // make gcc happy
            if (tpgrOn) {
                refBitmap->getPixelPtr(-1 - refDX, y - 1 - refDY, &tpgrCXPtr0);
                tpgrCX0 = refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                refBitmap->getPixelPtr(-1 - refDX, y - refDY, &tpgrCXPtr1);
                tpgrCX1 = refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &tpgrCXPtr2);
                tpgrCX2 = refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
            } else {
                tpgrCXPtr0.p = tpgrCXPtr1.p = tpgrCXPtr2.p = nullptr; // make gcc happy
                tpgrCXPtr0.shift = tpgrCXPtr1.shift = tpgrCXPtr2.shift = 0;
                tpgrCXPtr0.x = tpgrCXPtr1.x = tpgrCXPtr2.x = 0;
            }

            for (x = 0; x < w; ++x) {

                // update the context
                cx0 = ((cx0 << 1) | bitmap->nextPixel(&cxPtr0)) & 7;
                cx3 = ((cx3 << 1) | refBitmap->nextPixel(&cxPtr3)) & 7;
                cx4 = ((cx4 << 1) | refBitmap->nextPixel(&cxPtr4)) & 3;

                if (tpgrOn) {
                    // update the typical predictor context
                    tpgrCX0 = ((tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0)) & 7;
                    tpgrCX1 = ((tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1)) & 7;
                    tpgrCX2 = ((tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2)) & 7;

                    // check for a "typical" pixel
                    if (arithDecoder->decodeBit(ltpCX, refinementRegionStats)) {
                        ltp = !ltp;
                    }
                    if (tpgrCX0 == 0 && tpgrCX1 == 0 && tpgrCX2 == 0) {
                        bitmap->clearPixel(x, y);
                        continue;
                    }
                    if (tpgrCX0 == 7 && tpgrCX1 == 7 && tpgrCX2 == 7) {
                        bitmap->setPixel(x, y);
                        continue;
                    }
                }

                // build the context
                cx = (cx0 << 7) | (bitmap->nextPixel(&cxPtr1) << 6) | (refBitmap->nextPixel(&cxPtr2) << 5) | (cx3 << 2) | cx4;

                // decode the pixel
                if ((pix = arithDecoder->decodeBit(cx, refinementRegionStats))) {
                    bitmap->setPixel(x, y);
                }
            }

        } else {

            // set up the context
            bitmap->getPixelPtr(0, y - 1, &cxPtr0);
            cx0 = bitmap->nextPixel(&cxPtr0);
            bitmap->getPixelPtr(-1, y, &cxPtr1);
            refBitmap->getPixelPtr(-refDX, y - 1 - refDY, &cxPtr2);
            cx2 = refBitmap->nextPixel(&cxPtr2);
            refBitmap->getPixelPtr(-1 - refDX, y - refDY, &cxPtr3);
            cx3 = refBitmap->nextPixel(&cxPtr3);
            cx3 = (cx3 << 1) | refBitmap->nextPixel(&cxPtr3);
            refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &cxPtr4);
            cx4 = refBitmap->nextPixel(&cxPtr4);
            cx4 = (cx4 << 1) | refBitmap->nextPixel(&cxPtr4);
            bitmap->getPixelPtr(atx[0], y + aty[0], &cxPtr5);
            refBitmap->getPixelPtr(atx[1] - refDX, y + aty[1] - refDY, &cxPtr6);

            // set up the typical prediction context
            tpgrCX0 = tpgrCX1 = tpgrCX2 = 0; // make gcc happy
            if (tpgrOn) {
                refBitmap->getPixelPtr(-1 - refDX, y - 1 - refDY, &tpgrCXPtr0);
                tpgrCX0 = refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                tpgrCX0 = (tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0);
                refBitmap->getPixelPtr(-1 - refDX, y - refDY, &tpgrCXPtr1);
                tpgrCX1 = refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                tpgrCX1 = (tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1);
                refBitmap->getPixelPtr(-1 - refDX, y + 1 - refDY, &tpgrCXPtr2);
                tpgrCX2 = refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
                tpgrCX2 = (tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2);
            } else {
                tpgrCXPtr0.p = tpgrCXPtr1.p = tpgrCXPtr2.p = nullptr; // make gcc happy
                tpgrCXPtr0.shift = tpgrCXPtr1.shift = tpgrCXPtr2.shift = 0;
                tpgrCXPtr0.x = tpgrCXPtr1.x = tpgrCXPtr2.x = 0;
            }

            for (x = 0; x < w; ++x) {

                // update the context
                cx0 = ((cx0 << 1) | bitmap->nextPixel(&cxPtr0)) & 3;
                cx2 = ((cx2 << 1) | refBitmap->nextPixel(&cxPtr2)) & 3;
                cx3 = ((cx3 << 1) | refBitmap->nextPixel(&cxPtr3)) & 7;
                cx4 = ((cx4 << 1) | refBitmap->nextPixel(&cxPtr4)) & 7;

                if (tpgrOn) {
                    // update the typical predictor context
                    tpgrCX0 = ((tpgrCX0 << 1) | refBitmap->nextPixel(&tpgrCXPtr0)) & 7;
                    tpgrCX1 = ((tpgrCX1 << 1) | refBitmap->nextPixel(&tpgrCXPtr1)) & 7;
                    tpgrCX2 = ((tpgrCX2 << 1) | refBitmap->nextPixel(&tpgrCXPtr2)) & 7;

                    // check for a "typical" pixel
                    if (arithDecoder->decodeBit(ltpCX, refinementRegionStats)) {
                        ltp = !ltp;
                    }
                    if (tpgrCX0 == 0 && tpgrCX1 == 0 && tpgrCX2 == 0) {
                        bitmap->clearPixel(x, y);
                        continue;
                    }
                    if (tpgrCX0 == 7 && tpgrCX1 == 7 && tpgrCX2 == 7) {
                        bitmap->setPixel(x, y);
                        continue;
                    }
                }

                // build the context
                cx = (cx0 << 11) | (bitmap->nextPixel(&cxPtr1) << 10) | (cx2 << 8) | (cx3 << 5) | (cx4 << 2) | (bitmap->nextPixel(&cxPtr5) << 1) | refBitmap->nextPixel(&cxPtr6);

                // decode the pixel
                if ((pix = arithDecoder->decodeBit(cx, refinementRegionStats))) {
                    bitmap->setPixel(x, y);
                }

                if (unlikely(arithDecoder->getReadPastEndOfStream())) {
                    return nullptr;
                }
            }
        }
    }

    return bitmap;
}

void JBIG2Stream::readPageInfoSeg()
{
    unsigned int xRes, yRes, flags, striping;

    if (!readULong(&pageW) || !readULong(&pageH) || !readULong(&xRes) || !readULong(&yRes) || !readUByte(&flags) || !readUWord(&striping)) {
        goto eofError;
    }
    pageDefPixel = (flags >> 2) & 1;
    defCombOp = (flags >> 3) & 3;

    // allocate the page bitmap
    if (pageH == 0xffffffff) {
        curPageH = striping & 0x7fff;
    } else {
        curPageH = pageH;
    }
    delete pageBitmap;
    pageBitmap = new JBIG2Bitmap(0, pageW, curPageH);

    if (!pageBitmap->isOk()) {
        delete pageBitmap;
        pageBitmap = nullptr;
        return;
    }

    // default pixel value
    if (pageDefPixel) {
        pageBitmap->clearToOne();
    } else {
        pageBitmap->clearToZero();
    }

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
}

void JBIG2Stream::readEndOfStripeSeg(unsigned int length)
{
    // skip the segment
    byteCounter += curStr->discardChars(length);
}

void JBIG2Stream::readProfilesSeg(unsigned int length)
{
    // skip the segment
    byteCounter += curStr->discardChars(length);
}

void JBIG2Stream::readCodeTableSeg(unsigned int segNum)
{
    JBIG2HuffmanTable *huffTab;
    unsigned int flags, oob, prefixBits, rangeBits;
    int lowVal, highVal, val;
    unsigned int huffTabSize, i;
    bool eof = false;

    if (!readUByte(&flags) || !readLong(&lowVal) || !readLong(&highVal)) {
        goto eofError;
    }
    oob = flags & 1;
    prefixBits = ((flags >> 1) & 7) + 1;
    rangeBits = ((flags >> 4) & 7) + 1;

    huffDecoder->reset();
    huffTabSize = 8;
    huffTab = (JBIG2HuffmanTable *)gmallocn_checkoverflow(huffTabSize, sizeof(JBIG2HuffmanTable));
    if (unlikely(!huffTab)) {
        goto oomError;
    }

    i = 0;
    val = lowVal;
    while (val < highVal) {
        if (i == huffTabSize) {
            huffTabSize *= 2;
            huffTab = (JBIG2HuffmanTable *)greallocn_checkoverflow(huffTab, huffTabSize, sizeof(JBIG2HuffmanTable));
            if (unlikely(!huffTab)) {
                goto oomError;
            }
        }
        huffTab[i].val = val;
        huffTab[i].prefixLen = huffDecoder->readBits(prefixBits, &eof);
        huffTab[i].rangeLen = huffDecoder->readBits(rangeBits, &eof);

        const int shiftBits = huffTab[i].rangeLen % intNBits;

        if (eof || unlikely(checkedAdd(val, 1 << shiftBits, &val))) {
            free(huffTab);
            return;
        }
        ++i;
    }
    if (i + oob + 3 > huffTabSize) {
        huffTabSize = i + oob + 3;
        huffTab = (JBIG2HuffmanTable *)greallocn_checkoverflow(huffTab, huffTabSize, sizeof(JBIG2HuffmanTable));
        if (unlikely(!huffTab)) {
            goto oomError;
        }
    }
    huffTab[i].val = lowVal - 1;
    huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
    huffTab[i].rangeLen = jbig2HuffmanLOW;
    ++i;
    huffTab[i].val = highVal;
    huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
    huffTab[i].rangeLen = 32;
    ++i;
    if (oob) {
        huffTab[i].val = 0;
        huffTab[i].prefixLen = huffDecoder->readBits(prefixBits);
        huffTab[i].rangeLen = jbig2HuffmanOOB;
        ++i;
    }
    huffTab[i].val = 0;
    huffTab[i].prefixLen = 0;
    huffTab[i].rangeLen = jbig2HuffmanEOT;
    if (JBIG2HuffmanDecoder::buildTable(huffTab, i)) {
        // create and store the new table segment
        segments.push_back(std::make_unique<JBIG2CodeTable>(segNum, huffTab));
    } else {
        free(huffTab);
    }

    return;

eofError:
    error(errSyntaxError, curStr->getPos(), "Unexpected EOF in JBIG2 stream");
oomError:
    error(errInternal, curStr->getPos(), "Failed allocation when processing JBIG2 stream");
}

void JBIG2Stream::readExtensionSeg(unsigned int length)
{
    // skip the segment
    byteCounter += curStr->discardChars(length);
}

JBIG2Segment *JBIG2Stream::findSegment(unsigned int segNum)
{
    for (std::unique_ptr<JBIG2Segment> &seg : globalSegments) {
        if (seg->getSegNum() == segNum) {
            return seg.get();
        }
    }
    for (std::unique_ptr<JBIG2Segment> &seg : segments) {
        if (seg->getSegNum() == segNum) {
            return seg.get();
        }
    }
    return nullptr;
}

void JBIG2Stream::discardSegment(unsigned int segNum)
{
    for (auto it = globalSegments.begin(); it != globalSegments.end(); ++it) {
        if ((*it)->getSegNum() == segNum) {
            globalSegments.erase(it);
            return;
        }
    }
    for (auto it = segments.begin(); it != segments.end(); ++it) {
        if ((*it)->getSegNum() == segNum) {
            segments.erase(it);
            return;
        }
    }
}

void JBIG2Stream::resetGenericStats(unsigned int templ, JArithmeticDecoderStats *prevStats)
{
    int size;

    size = contextSize[templ];
    if (prevStats && prevStats->getContextSize() == size) {
        if (genericRegionStats->getContextSize() == size) {
            genericRegionStats->copyFrom(prevStats);
        } else {
            delete genericRegionStats;
            genericRegionStats = prevStats->copy();
        }
    } else {
        if (genericRegionStats->getContextSize() == size) {
            genericRegionStats->reset();
        } else {
            delete genericRegionStats;
            genericRegionStats = new JArithmeticDecoderStats(1 << size);
        }
    }
}

void JBIG2Stream::resetRefinementStats(unsigned int templ, JArithmeticDecoderStats *prevStats)
{
    int size;

    size = refContextSize[templ];
    if (prevStats && prevStats->getContextSize() == size) {
        if (refinementRegionStats->getContextSize() == size) {
            refinementRegionStats->copyFrom(prevStats);
        } else {
            delete refinementRegionStats;
            refinementRegionStats = prevStats->copy();
        }
    } else {
        if (refinementRegionStats->getContextSize() == size) {
            refinementRegionStats->reset();
        } else {
            delete refinementRegionStats;
            refinementRegionStats = new JArithmeticDecoderStats(1 << size);
        }
    }
}

bool JBIG2Stream::resetIntStats(int symCodeLen)
{
    iadhStats->reset();
    iadwStats->reset();
    iaexStats->reset();
    iaaiStats->reset();
    iadtStats->reset();
    iaitStats->reset();
    iafsStats->reset();
    iadsStats->reset();
    iardxStats->reset();
    iardyStats->reset();
    iardwStats->reset();
    iardhStats->reset();
    iariStats->reset();
    if (symCodeLen + 1 >= 31) {
        return false;
    }
    if (iaidStats != nullptr && iaidStats->getContextSize() == 1 << (symCodeLen + 1)) {
        iaidStats->reset();
    } else {
        delete iaidStats;
        iaidStats = new JArithmeticDecoderStats(1 << (symCodeLen + 1));
        if (!iaidStats->isValid()) {
            delete iaidStats;
            iaidStats = nullptr;
            return false;
        }
    }
    return true;
}

bool JBIG2Stream::readUByte(unsigned int *x)
{
    int c0;

    if ((c0 = curStr->getChar()) == EOF) {
        return false;
    }
    ++byteCounter;
    *x = (unsigned int)c0;
    return true;
}

bool JBIG2Stream::readByte(int *x)
{
    int c0;

    if ((c0 = curStr->getChar()) == EOF) {
        return false;
    }
    ++byteCounter;
    *x = c0;
    if (c0 & 0x80) {
        *x |= -1 - 0xff;
    }
    return true;
}

bool JBIG2Stream::readUWord(unsigned int *x)
{
    int c0, c1;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 2;
    *x = (unsigned int)((c0 << 8) | c1);
    return true;
}

bool JBIG2Stream::readULong(unsigned int *x)
{
    int c0, c1, c2, c3;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 4;
    *x = (unsigned int)((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
    return true;
}

bool JBIG2Stream::readLong(int *x)
{
    int c0, c1, c2, c3;

    if ((c0 = curStr->getChar()) == EOF || (c1 = curStr->getChar()) == EOF || (c2 = curStr->getChar()) == EOF || (c3 = curStr->getChar()) == EOF) {
        return false;
    }
    byteCounter += 4;
    *x = ((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
    if (c0 & 0x80) {
        *x |= -1 - (int)0xffffffff;
    }
    return true;
}
