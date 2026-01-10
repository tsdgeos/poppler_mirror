//========================================================================
//
// Stream-CCITT.h
//
// Tables for CCITT Fax decoding.
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
// Copyright (C) 2008 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef STREAM_CCITT_H
#define STREAM_CCITT_H

struct CCITTCode
{
    short bits;
    short n;
};

#define ccittEOL (-2)

//------------------------------------------------------------------------
// 2D codes
//------------------------------------------------------------------------

#define twoDimPass 0
#define twoDimHoriz 1
#define twoDimVert0 2
#define twoDimVertR1 3
#define twoDimVertL1 4
#define twoDimVertR2 5
#define twoDimVertL2 6
#define twoDimVertR3 7
#define twoDimVertL3 8

// 1-7 bit codes
static const CCITTCode twoDimTab1[128] = {
    { .bits = -1, .n = -1 },          { .bits = -1, .n = -1 }, // 000000x
    { .bits = 7, .n = twoDimVertL3 }, // 0000010
    { .bits = 7, .n = twoDimVertR3 }, // 0000011
    { .bits = 6, .n = twoDimVertL2 }, { .bits = 6, .n = twoDimVertL2 }, // 000010x
    { .bits = 6, .n = twoDimVertR2 }, { .bits = 6, .n = twoDimVertR2 }, // 000011x
    { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass }, // 0001xxx
    { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass },   { .bits = 4, .n = twoDimPass },
    { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz }, // 001xxxx
    { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },
    { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },
    { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimHoriz },  { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, // 010xxxx
    { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 },
    { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 },
    { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertL1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, // 011xxxx
    { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 },
    { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 },
    { .bits = 3, .n = twoDimVertR1 }, { .bits = 3, .n = twoDimVertR1 }, { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 }, // 1xxxxxx
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 },
    { .bits = 1, .n = twoDimVert0 },  { .bits = 1, .n = twoDimVert0 }
};

//------------------------------------------------------------------------
// white run lengths
//------------------------------------------------------------------------

// 11-12 bit codes (upper 7 bits are 0)
static const CCITTCode whiteTab1[32] = {
    { .bits = -1, .n = -1 }, // 00000
    { .bits = 12, .n = ccittEOL }, // 00001
    { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 }, // 0001x
    { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, // 001xx
    { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, // 010xx
    { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, // 011xx
    { .bits = 11, .n = 1792 },     { .bits = 11, .n = 1792 }, // 1000x
    { .bits = 12, .n = 1984 }, // 10010
    { .bits = 12, .n = 2048 }, // 10011
    { .bits = 12, .n = 2112 }, // 10100
    { .bits = 12, .n = 2176 }, // 10101
    { .bits = 12, .n = 2240 }, // 10110
    { .bits = 12, .n = 2304 }, // 10111
    { .bits = 11, .n = 1856 },     { .bits = 11, .n = 1856 }, // 1100x
    { .bits = 11, .n = 1920 },     { .bits = 11, .n = 1920 }, // 1101x
    { .bits = 12, .n = 2368 }, // 11100
    { .bits = 12, .n = 2432 }, // 11101
    { .bits = 12, .n = 2496 }, // 11110
    { .bits = 12, .n = 2560 } // 11111
};

// 1-9 bit codes
static const CCITTCode whiteTab2[512] = {
    { .bits = -1, .n = -1 },  { .bits = -1, .n = -1 },  { .bits = -1, .n = -1 },  { .bits = -1, .n = -1 }, // 0000000xx
    { .bits = 8, .n = 29 },   { .bits = 8, .n = 29 }, // 00000010x
    { .bits = 8, .n = 30 },   { .bits = 8, .n = 30 }, // 00000011x
    { .bits = 8, .n = 45 },   { .bits = 8, .n = 45 }, // 00000100x
    { .bits = 8, .n = 46 },   { .bits = 8, .n = 46 }, // 00000101x
    { .bits = 7, .n = 22 },   { .bits = 7, .n = 22 },   { .bits = 7, .n = 22 },   { .bits = 7, .n = 22 }, // 0000011xx
    { .bits = 7, .n = 23 },   { .bits = 7, .n = 23 },   { .bits = 7, .n = 23 },   { .bits = 7, .n = 23 }, // 0000100xx
    { .bits = 8, .n = 47 },   { .bits = 8, .n = 47 }, // 00001010x
    { .bits = 8, .n = 48 },   { .bits = 8, .n = 48 }, // 00001011x
    { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 }, // 000011xxx
    { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 },   { .bits = 6, .n = 13 },   { .bits = 7, .n = 20 },   { .bits = 7, .n = 20 },   { .bits = 7, .n = 20 },   { .bits = 7, .n = 20 }, // 0001000xx
    { .bits = 8, .n = 33 },   { .bits = 8, .n = 33 }, // 00010010x
    { .bits = 8, .n = 34 },   { .bits = 8, .n = 34 }, // 00010011x
    { .bits = 8, .n = 35 },   { .bits = 8, .n = 35 }, // 00010100x
    { .bits = 8, .n = 36 },   { .bits = 8, .n = 36 }, // 00010101x
    { .bits = 8, .n = 37 },   { .bits = 8, .n = 37 }, // 00010110x
    { .bits = 8, .n = 38 },   { .bits = 8, .n = 38 }, // 00010111x
    { .bits = 7, .n = 19 },   { .bits = 7, .n = 19 },   { .bits = 7, .n = 19 },   { .bits = 7, .n = 19 }, // 0001100xx
    { .bits = 8, .n = 31 },   { .bits = 8, .n = 31 }, // 00011010x
    { .bits = 8, .n = 32 },   { .bits = 8, .n = 32 }, // 00011011x
    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 }, // 000111xxx
    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 },    { .bits = 6, .n = 1 },    { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 }, // 001000xxx
    { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 },   { .bits = 6, .n = 12 },   { .bits = 8, .n = 53 },   { .bits = 8, .n = 53 }, // 00100100x
    { .bits = 8, .n = 54 },   { .bits = 8, .n = 54 }, // 00100101x
    { .bits = 7, .n = 26 },   { .bits = 7, .n = 26 },   { .bits = 7, .n = 26 },   { .bits = 7, .n = 26 }, // 0010011xx
    { .bits = 8, .n = 39 },   { .bits = 8, .n = 39 }, // 00101000x
    { .bits = 8, .n = 40 },   { .bits = 8, .n = 40 }, // 00101001x
    { .bits = 8, .n = 41 },   { .bits = 8, .n = 41 }, // 00101010x
    { .bits = 8, .n = 42 },   { .bits = 8, .n = 42 }, // 00101011x
    { .bits = 8, .n = 43 },   { .bits = 8, .n = 43 }, // 00101100x
    { .bits = 8, .n = 44 },   { .bits = 8, .n = 44 }, // 00101101x
    { .bits = 7, .n = 21 },   { .bits = 7, .n = 21 },   { .bits = 7, .n = 21 },   { .bits = 7, .n = 21 }, // 0010111xx
    { .bits = 7, .n = 28 },   { .bits = 7, .n = 28 },   { .bits = 7, .n = 28 },   { .bits = 7, .n = 28 }, // 0011000xx
    { .bits = 8, .n = 61 },   { .bits = 8, .n = 61 }, // 00110010x
    { .bits = 8, .n = 62 },   { .bits = 8, .n = 62 }, // 00110011x
    { .bits = 8, .n = 63 },   { .bits = 8, .n = 63 }, // 00110100x
    { .bits = 8, .n = 0 },    { .bits = 8, .n = 0 }, // 00110101x
    { .bits = 8, .n = 320 },  { .bits = 8, .n = 320 }, // 00110110x
    { .bits = 8, .n = 384 },  { .bits = 8, .n = 384 }, // 00110111x
    { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 }, // 00111xxxx
    { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },
    { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 10 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 }, // 01000xxxx
    { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },
    { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 5, .n = 11 },   { .bits = 7, .n = 27 },   { .bits = 7, .n = 27 },   { .bits = 7, .n = 27 },   { .bits = 7, .n = 27 }, // 0100100xx
    { .bits = 8, .n = 59 },   { .bits = 8, .n = 59 }, // 01001010x
    { .bits = 8, .n = 60 },   { .bits = 8, .n = 60 }, // 01001011x
    { .bits = 9, .n = 1472 }, // 010011000
    { .bits = 9, .n = 1536 }, // 010011001
    { .bits = 9, .n = 1600 }, // 010011010
    { .bits = 9, .n = 1728 }, // 010011011
    { .bits = 7, .n = 18 },   { .bits = 7, .n = 18 },   { .bits = 7, .n = 18 },   { .bits = 7, .n = 18 }, // 0100111xx
    { .bits = 7, .n = 24 },   { .bits = 7, .n = 24 },   { .bits = 7, .n = 24 },   { .bits = 7, .n = 24 }, // 0101000xx
    { .bits = 8, .n = 49 },   { .bits = 8, .n = 49 }, // 01010010x
    { .bits = 8, .n = 50 },   { .bits = 8, .n = 50 }, // 01010011x
    { .bits = 8, .n = 51 },   { .bits = 8, .n = 51 }, // 01010100x
    { .bits = 8, .n = 52 },   { .bits = 8, .n = 52 }, // 01010101x
    { .bits = 7, .n = 25 },   { .bits = 7, .n = 25 },   { .bits = 7, .n = 25 },   { .bits = 7, .n = 25 }, // 0101011xx
    { .bits = 8, .n = 55 },   { .bits = 8, .n = 55 }, // 01011000x
    { .bits = 8, .n = 56 },   { .bits = 8, .n = 56 }, // 01011001x
    { .bits = 8, .n = 57 },   { .bits = 8, .n = 57 }, // 01011010x
    { .bits = 8, .n = 58 },   { .bits = 8, .n = 58 }, // 01011011x
    { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 }, // 010111xxx
    { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 },  { .bits = 6, .n = 192 },  { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, // 011000xxx
    { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, { .bits = 6, .n = 1664 }, { .bits = 8, .n = 448 },  { .bits = 8, .n = 448 }, // 01100100x
    { .bits = 8, .n = 512 },  { .bits = 8, .n = 512 }, // 01100101x
    { .bits = 9, .n = 704 }, // 011001100
    { .bits = 9, .n = 768 }, // 011001101
    { .bits = 8, .n = 640 },  { .bits = 8, .n = 640 }, // 01100111x
    { .bits = 8, .n = 576 },  { .bits = 8, .n = 576 }, // 01101000x
    { .bits = 9, .n = 832 }, // 011010010
    { .bits = 9, .n = 896 }, // 011010011
    { .bits = 9, .n = 960 }, // 011010100
    { .bits = 9, .n = 1024 }, // 011010101
    { .bits = 9, .n = 1088 }, // 011010110
    { .bits = 9, .n = 1152 }, // 011010111
    { .bits = 9, .n = 1216 }, // 011011000
    { .bits = 9, .n = 1280 }, // 011011001
    { .bits = 9, .n = 1344 }, // 011011010
    { .bits = 9, .n = 1408 }, // 011011011
    { .bits = 7, .n = 256 },  { .bits = 7, .n = 256 },  { .bits = 7, .n = 256 },  { .bits = 7, .n = 256 }, // 0110111xx
    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 }, // 0111xxxxx
    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },
    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },
    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },
    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 2 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 }, // 1000xxxxx
    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },
    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },
    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },
    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 4, .n = 3 },    { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 }, // 10010xxxx
    { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },
    { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 128 },  { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 }, // 10011xxxx
    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },
    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 8 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 }, // 10100xxxx
    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },
    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 5, .n = 9 },    { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 }, // 101010xxx
    { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 },   { .bits = 6, .n = 16 },   { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 }, // 101011xxx
    { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 },   { .bits = 6, .n = 17 },   { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 }, // 1011xxxxx
    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },
    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },
    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },
    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 4 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 }, // 1100xxxxx
    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },
    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },
    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },
    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 4, .n = 5 },    { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 }, // 110100xxx
    { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 },   { .bits = 6, .n = 14 },   { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 }, // 110101xxx
    { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 },   { .bits = 6, .n = 15 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 }, // 11011xxxx
    { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },
    { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 5, .n = 64 },   { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 }, // 1110xxxxx
    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },
    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },
    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },
    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 6 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 }, // 1111xxxxx
    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },
    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },
    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },
    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 },    { .bits = 4, .n = 7 }
};

//------------------------------------------------------------------------
// black run lengths
//------------------------------------------------------------------------

// 10-13 bit codes (upper 6 bits are 0)
static const CCITTCode blackTab1[128] = { { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 }, // 000000000000x
                                          { .bits = 12, .n = ccittEOL }, { .bits = 12, .n = ccittEOL }, // 000000000001x
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000001xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000010xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000011xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000100xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000101xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000110xx
                                          { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },       { .bits = -1, .n = -1 },   { .bits = -1, .n = -1 }, // 00000000111xx
                                          { .bits = 11, .n = 1792 },     { .bits = 11, .n = 1792 },     { .bits = 11, .n = 1792 }, { .bits = 11, .n = 1792 }, // 00000001000xx
                                          { .bits = 12, .n = 1984 },     { .bits = 12, .n = 1984 }, // 000000010010x
                                          { .bits = 12, .n = 2048 },     { .bits = 12, .n = 2048 }, // 000000010011x
                                          { .bits = 12, .n = 2112 },     { .bits = 12, .n = 2112 }, // 000000010100x
                                          { .bits = 12, .n = 2176 },     { .bits = 12, .n = 2176 }, // 000000010101x
                                          { .bits = 12, .n = 2240 },     { .bits = 12, .n = 2240 }, // 000000010110x
                                          { .bits = 12, .n = 2304 },     { .bits = 12, .n = 2304 }, // 000000010111x
                                          { .bits = 11, .n = 1856 },     { .bits = 11, .n = 1856 },     { .bits = 11, .n = 1856 }, { .bits = 11, .n = 1856 }, // 00000001100xx
                                          { .bits = 11, .n = 1920 },     { .bits = 11, .n = 1920 },     { .bits = 11, .n = 1920 }, { .bits = 11, .n = 1920 }, // 00000001101xx
                                          { .bits = 12, .n = 2368 },     { .bits = 12, .n = 2368 }, // 000000011100x
                                          { .bits = 12, .n = 2432 },     { .bits = 12, .n = 2432 }, // 000000011101x
                                          { .bits = 12, .n = 2496 },     { .bits = 12, .n = 2496 }, // 000000011110x
                                          { .bits = 12, .n = 2560 },     { .bits = 12, .n = 2560 }, // 000000011111x
                                          { .bits = 10, .n = 18 },       { .bits = 10, .n = 18 },       { .bits = 10, .n = 18 },   { .bits = 10, .n = 18 }, // 0000001000xxx
                                          { .bits = 10, .n = 18 },       { .bits = 10, .n = 18 },       { .bits = 10, .n = 18 },   { .bits = 10, .n = 18 },   { .bits = 12, .n = 52 }, { .bits = 12, .n = 52 }, // 000000100100x
                                          { .bits = 13, .n = 640 }, // 0000001001010
                                          { .bits = 13, .n = 704 }, // 0000001001011
                                          { .bits = 13, .n = 768 }, // 0000001001100
                                          { .bits = 13, .n = 832 }, // 0000001001101
                                          { .bits = 12, .n = 55 },       { .bits = 12, .n = 55 }, // 000000100111x
                                          { .bits = 12, .n = 56 },       { .bits = 12, .n = 56 }, // 000000101000x
                                          { .bits = 13, .n = 1280 }, // 0000001010010
                                          { .bits = 13, .n = 1344 }, // 0000001010011
                                          { .bits = 13, .n = 1408 }, // 0000001010100
                                          { .bits = 13, .n = 1472 }, // 0000001010101
                                          { .bits = 12, .n = 59 },       { .bits = 12, .n = 59 }, // 000000101011x
                                          { .bits = 12, .n = 60 },       { .bits = 12, .n = 60 }, // 000000101100x
                                          { .bits = 13, .n = 1536 }, // 0000001011010
                                          { .bits = 13, .n = 1600 }, // 0000001011011
                                          { .bits = 11, .n = 24 },       { .bits = 11, .n = 24 },       { .bits = 11, .n = 24 },   { .bits = 11, .n = 24 }, // 00000010111xx
                                          { .bits = 11, .n = 25 },       { .bits = 11, .n = 25 },       { .bits = 11, .n = 25 },   { .bits = 11, .n = 25 }, // 00000011000xx
                                          { .bits = 13, .n = 1664 }, // 0000001100100
                                          { .bits = 13, .n = 1728 }, // 0000001100101
                                          { .bits = 12, .n = 320 },      { .bits = 12, .n = 320 }, // 000000110011x
                                          { .bits = 12, .n = 384 },      { .bits = 12, .n = 384 }, // 000000110100x
                                          { .bits = 12, .n = 448 },      { .bits = 12, .n = 448 }, // 000000110101x
                                          { .bits = 13, .n = 512 }, // 0000001101100
                                          { .bits = 13, .n = 576 }, // 0000001101101
                                          { .bits = 12, .n = 53 },       { .bits = 12, .n = 53 }, // 000000110111x
                                          { .bits = 12, .n = 54 },       { .bits = 12, .n = 54 }, // 000000111000x
                                          { .bits = 13, .n = 896 }, // 0000001110010
                                          { .bits = 13, .n = 960 }, // 0000001110011
                                          { .bits = 13, .n = 1024 }, // 0000001110100
                                          { .bits = 13, .n = 1088 }, // 0000001110101
                                          { .bits = 13, .n = 1152 }, // 0000001110110
                                          { .bits = 13, .n = 1216 }, // 0000001110111
                                          { .bits = 10, .n = 64 },       { .bits = 10, .n = 64 },       { .bits = 10, .n = 64 },   { .bits = 10, .n = 64 }, // 0000001111xxx
                                          { .bits = 10, .n = 64 },       { .bits = 10, .n = 64 },       { .bits = 10, .n = 64 },   { .bits = 10, .n = 64 } };

// 7-12 bit codes (upper 4 bits are 0)
static const CCITTCode blackTab2[192] = {
    { .bits = 8, .n = 13 },   { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 }, // 00000100xxxx
    { .bits = 8, .n = 13 },   { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },   { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 }, { .bits = 8, .n = 13 },
    { .bits = 8, .n = 13 },   { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },  { .bits = 8, .n = 13 },  { .bits = 11, .n = 23 },  { .bits = 11, .n = 23 }, // 00000101000x
    { .bits = 12, .n = 50 }, // 000001010010
    { .bits = 12, .n = 51 }, // 000001010011
    { .bits = 12, .n = 44 }, // 000001010100
    { .bits = 12, .n = 45 }, // 000001010101
    { .bits = 12, .n = 46 }, // 000001010110
    { .bits = 12, .n = 47 }, // 000001010111
    { .bits = 12, .n = 57 }, // 000001011000
    { .bits = 12, .n = 58 }, // 000001011001
    { .bits = 12, .n = 61 }, // 000001011010
    { .bits = 12, .n = 256 }, // 000001011011
    { .bits = 10, .n = 16 },  { .bits = 10, .n = 16 }, { .bits = 10, .n = 16 }, { .bits = 10, .n = 16 }, // 0000010111xx
    { .bits = 10, .n = 17 },  { .bits = 10, .n = 17 }, { .bits = 10, .n = 17 }, { .bits = 10, .n = 17 }, // 0000011000xx
    { .bits = 12, .n = 48 }, // 000001100100
    { .bits = 12, .n = 49 }, // 000001100101
    { .bits = 12, .n = 62 }, // 000001100110
    { .bits = 12, .n = 63 }, // 000001100111
    { .bits = 12, .n = 30 }, // 000001101000
    { .bits = 12, .n = 31 }, // 000001101001
    { .bits = 12, .n = 32 }, // 000001101010
    { .bits = 12, .n = 33 }, // 000001101011
    { .bits = 12, .n = 40 }, // 000001101100
    { .bits = 12, .n = 41 }, // 000001101101
    { .bits = 11, .n = 22 },  { .bits = 11, .n = 22 }, // 00000110111x
    { .bits = 8, .n = 14 },   { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 }, // 00000111xxxx
    { .bits = 8, .n = 14 },   { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },   { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 }, { .bits = 8, .n = 14 },
    { .bits = 8, .n = 14 },   { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },  { .bits = 8, .n = 14 },  { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 }, { .bits = 7, .n = 10 }, // 0000100xxxxx
    { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 }, { .bits = 7, .n = 10 },
    { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 }, { .bits = 7, .n = 10 },
    { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 }, { .bits = 7, .n = 10 },
    { .bits = 7, .n = 10 },   { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 10 },  { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 }, { .bits = 7, .n = 11 }, // 0000101xxxxx
    { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 }, { .bits = 7, .n = 11 },
    { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 }, { .bits = 7, .n = 11 },
    { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 }, { .bits = 7, .n = 11 },
    { .bits = 7, .n = 11 },   { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 7, .n = 11 },  { .bits = 9, .n = 15 },   { .bits = 9, .n = 15 },  { .bits = 9, .n = 15 }, { .bits = 9, .n = 15 }, // 000011000xxx
    { .bits = 9, .n = 15 },   { .bits = 9, .n = 15 },  { .bits = 9, .n = 15 },  { .bits = 9, .n = 15 },  { .bits = 12, .n = 128 }, // 000011001000
    { .bits = 12, .n = 192 }, // 000011001001
    { .bits = 12, .n = 26 }, // 000011001010
    { .bits = 12, .n = 27 }, // 000011001011
    { .bits = 12, .n = 28 }, // 000011001100
    { .bits = 12, .n = 29 }, // 000011001101
    { .bits = 11, .n = 19 },  { .bits = 11, .n = 19 }, // 00001100111x
    { .bits = 11, .n = 20 },  { .bits = 11, .n = 20 }, // 00001101000x
    { .bits = 12, .n = 34 }, // 000011010010
    { .bits = 12, .n = 35 }, // 000011010011
    { .bits = 12, .n = 36 }, // 000011010100
    { .bits = 12, .n = 37 }, // 000011010101
    { .bits = 12, .n = 38 }, // 000011010110
    { .bits = 12, .n = 39 }, // 000011010111
    { .bits = 11, .n = 21 },  { .bits = 11, .n = 21 }, // 00001101100x
    { .bits = 12, .n = 42 }, // 000011011010
    { .bits = 12, .n = 43 }, // 000011011011
    { .bits = 10, .n = 0 },   { .bits = 10, .n = 0 },  { .bits = 10, .n = 0 },  { .bits = 10, .n = 0 }, // 0000110111xx
    { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 }, // 0000111xxxxx
    { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 }, { .bits = 7, .n = 12 },
    { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 }, { .bits = 7, .n = 12 },
    { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 }, { .bits = 7, .n = 12 },
    { .bits = 7, .n = 12 },   { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 },  { .bits = 7, .n = 12 }
};

// 2-6 bit codes
static const CCITTCode blackTab3[64] = {
    { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, { .bits = -1, .n = -1 }, // 0000xx
    { .bits = 6, .n = 9 }, // 000100
    { .bits = 6, .n = 8 }, // 000101
    { .bits = 5, .n = 7 },   { .bits = 5, .n = 7 }, // 00011x
    { .bits = 4, .n = 6 },   { .bits = 4, .n = 6 },   { .bits = 4, .n = 6 },   { .bits = 4, .n = 6 }, // 0010xx
    { .bits = 4, .n = 5 },   { .bits = 4, .n = 5 },   { .bits = 4, .n = 5 },   { .bits = 4, .n = 5 }, // 0011xx
    { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 }, // 010xxx
    { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 },   { .bits = 3, .n = 1 },   { .bits = 3, .n = 4 }, { .bits = 3, .n = 4 }, { .bits = 3, .n = 4 }, { .bits = 3, .n = 4 }, // 011xxx
    { .bits = 3, .n = 4 },   { .bits = 3, .n = 4 },   { .bits = 3, .n = 4 },   { .bits = 3, .n = 4 },   { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 }, // 10xxxx
    { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 }, { .bits = 2, .n = 3 },
    { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 3 },   { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 }, // 11xxxx
    { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 }, { .bits = 2, .n = 2 },
    { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 },   { .bits = 2, .n = 2 }
};

#endif
