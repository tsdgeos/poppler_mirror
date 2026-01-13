//========================================================================
//
// UnicodeMapTables.h
//
// Copyright 2001-2009 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2024 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "UnicodeMap.h"

static const UnicodeMapRange latin1UnicodeMapRanges[] = {
    { .start = 0x000a, .end = 0x000a, .code = 0x0a, .nBytes = 1 },     { .start = 0x000c, .end = 0x000d, .code = 0x0c, .nBytes = 1 },   { .start = 0x0020, .end = 0x007e, .code = 0x20, .nBytes = 1 },
    { .start = 0x00a0, .end = 0x00a0, .code = 0x20, .nBytes = 1 },     { .start = 0x00a1, .end = 0x00ac, .code = 0xa1, .nBytes = 1 },   { .start = 0x00ae, .end = 0x00ff, .code = 0xae, .nBytes = 1 },
    { .start = 0x010c, .end = 0x010c, .code = 0x43, .nBytes = 1 },     { .start = 0x010d, .end = 0x010d, .code = 0x63, .nBytes = 1 },   { .start = 0x0131, .end = 0x0131, .code = 0x69, .nBytes = 1 },
    { .start = 0x0141, .end = 0x0141, .code = 0x4c, .nBytes = 1 },     { .start = 0x0142, .end = 0x0142, .code = 0x6c, .nBytes = 1 },   { .start = 0x0152, .end = 0x0152, .code = 0x4f45, .nBytes = 2 },
    { .start = 0x0153, .end = 0x0153, .code = 0x6f65, .nBytes = 2 },   { .start = 0x0160, .end = 0x0160, .code = 0x53, .nBytes = 1 },   { .start = 0x0161, .end = 0x0161, .code = 0x73, .nBytes = 1 },
    { .start = 0x0178, .end = 0x0178, .code = 0x59, .nBytes = 1 },     { .start = 0x017d, .end = 0x017d, .code = 0x5a, .nBytes = 1 },   { .start = 0x017e, .end = 0x017e, .code = 0x7a, .nBytes = 1 },
    { .start = 0x02c6, .end = 0x02c6, .code = 0x5e, .nBytes = 1 },     { .start = 0x02da, .end = 0x02da, .code = 0xb0, .nBytes = 1 },   { .start = 0x02dc, .end = 0x02dc, .code = 0x7e, .nBytes = 1 },
    { .start = 0x2013, .end = 0x2013, .code = 0xad, .nBytes = 1 },     { .start = 0x2014, .end = 0x2014, .code = 0x2d2d, .nBytes = 2 }, { .start = 0x2018, .end = 0x2018, .code = 0x60, .nBytes = 1 },
    { .start = 0x2019, .end = 0x2019, .code = 0x27, .nBytes = 1 },     { .start = 0x201a, .end = 0x201a, .code = 0x2c, .nBytes = 1 },   { .start = 0x201c, .end = 0x201c, .code = 0x22, .nBytes = 1 },
    { .start = 0x201d, .end = 0x201d, .code = 0x22, .nBytes = 1 },     { .start = 0x201e, .end = 0x201e, .code = 0x2c2c, .nBytes = 2 }, { .start = 0x2022, .end = 0x2022, .code = 0xb7, .nBytes = 1 },
    { .start = 0x2026, .end = 0x2026, .code = 0x2e2e2e, .nBytes = 3 }, { .start = 0x2039, .end = 0x2039, .code = 0x3c, .nBytes = 1 },   { .start = 0x203a, .end = 0x203a, .code = 0x3e, .nBytes = 1 },
    { .start = 0x2044, .end = 0x2044, .code = 0x2f, .nBytes = 1 },     { .start = 0x2122, .end = 0x2122, .code = 0x544d, .nBytes = 2 }, { .start = 0x2212, .end = 0x2212, .code = 0x2d, .nBytes = 1 },
    { .start = 0xf6f9, .end = 0xf6f9, .code = 0x4c, .nBytes = 1 },     { .start = 0xf6fa, .end = 0xf6fa, .code = 0x4f45, .nBytes = 2 }, { .start = 0xf6fc, .end = 0xf6fc, .code = 0xb0, .nBytes = 1 },
    { .start = 0xf6fd, .end = 0xf6fd, .code = 0x53, .nBytes = 1 },     { .start = 0xf6fe, .end = 0xf6fe, .code = 0x7e, .nBytes = 1 },   { .start = 0xf6ff, .end = 0xf6ff, .code = 0x5a, .nBytes = 1 },
    { .start = 0xf721, .end = 0xf721, .code = 0x21, .nBytes = 1 },     { .start = 0xf724, .end = 0xf724, .code = 0x24, .nBytes = 1 },   { .start = 0xf726, .end = 0xf726, .code = 0x26, .nBytes = 1 },
    { .start = 0xf730, .end = 0xf739, .code = 0x30, .nBytes = 1 },     { .start = 0xf73f, .end = 0xf73f, .code = 0x3f, .nBytes = 1 },   { .start = 0xf761, .end = 0xf77a, .code = 0x41, .nBytes = 1 },
    { .start = 0xf7a1, .end = 0xf7a2, .code = 0xa1, .nBytes = 1 },     { .start = 0xf7bf, .end = 0xf7bf, .code = 0xbf, .nBytes = 1 },   { .start = 0xf7e0, .end = 0xf7f6, .code = 0xc0, .nBytes = 1 },
    { .start = 0xf7f8, .end = 0xf7fe, .code = 0xd8, .nBytes = 1 },     { .start = 0xf7ff, .end = 0xf7ff, .code = 0x59, .nBytes = 1 },   { .start = 0xfb00, .end = 0xfb00, .code = 0x6666, .nBytes = 2 },
    { .start = 0xfb01, .end = 0xfb01, .code = 0x6669, .nBytes = 2 },   { .start = 0xfb02, .end = 0xfb02, .code = 0x666c, .nBytes = 2 }, { .start = 0xfb03, .end = 0xfb03, .code = 0x666669, .nBytes = 3 },
    { .start = 0xfb04, .end = 0xfb04, .code = 0x66666c, .nBytes = 3 }, { .start = 0xfb05, .end = 0xfb05, .code = 0x7374, .nBytes = 2 }, { .start = 0xfb06, .end = 0xfb06, .code = 0x7374, .nBytes = 2 }
};
#define latin1UnicodeMapLen (sizeof(latin1UnicodeMapRanges) / sizeof(UnicodeMapRange))

static const UnicodeMapRange ascii7UnicodeMapRanges[] = {
    { .start = 0x000a, .end = 0x000a, .code = 0x0a, .nBytes = 1 },     { .start = 0x000c, .end = 0x000d, .code = 0x0c, .nBytes = 1 },     { .start = 0x0020, .end = 0x005f, .code = 0x20, .nBytes = 1 },
    { .start = 0x0061, .end = 0x007e, .code = 0x61, .nBytes = 1 },     { .start = 0x00a6, .end = 0x00a6, .code = 0x7c, .nBytes = 1 },     { .start = 0x00a9, .end = 0x00a9, .code = 0x286329, .nBytes = 3 },
    { .start = 0x00ae, .end = 0x00ae, .code = 0x285229, .nBytes = 3 }, { .start = 0x00b7, .end = 0x00b7, .code = 0x2a, .nBytes = 1 },     { .start = 0x00bc, .end = 0x00bc, .code = 0x312f34, .nBytes = 3 },
    { .start = 0x00bd, .end = 0x00bd, .code = 0x312f32, .nBytes = 3 }, { .start = 0x00be, .end = 0x00be, .code = 0x332f34, .nBytes = 3 }, { .start = 0x00c0, .end = 0x00c0, .code = 0x41, .nBytes = 1 },
    { .start = 0x00c1, .end = 0x00c1, .code = 0x41, .nBytes = 1 },     { .start = 0x00c2, .end = 0x00c2, .code = 0x41, .nBytes = 1 },     { .start = 0x00c3, .end = 0x00c3, .code = 0x41, .nBytes = 1 },
    { .start = 0x00c4, .end = 0x00c4, .code = 0x41, .nBytes = 1 },     { .start = 0x00c5, .end = 0x00c5, .code = 0x41, .nBytes = 1 },     { .start = 0x00c6, .end = 0x00c6, .code = 0x4145, .nBytes = 2 },
    { .start = 0x00c7, .end = 0x00c7, .code = 0x43, .nBytes = 1 },     { .start = 0x00c8, .end = 0x00c8, .code = 0x45, .nBytes = 1 },     { .start = 0x00c9, .end = 0x00c9, .code = 0x45, .nBytes = 1 },
    { .start = 0x00ca, .end = 0x00ca, .code = 0x45, .nBytes = 1 },     { .start = 0x00cb, .end = 0x00cb, .code = 0x45, .nBytes = 1 },     { .start = 0x00cc, .end = 0x00cc, .code = 0x49, .nBytes = 1 },
    { .start = 0x00cd, .end = 0x00cd, .code = 0x49, .nBytes = 1 },     { .start = 0x00ce, .end = 0x00ce, .code = 0x49, .nBytes = 1 },     { .start = 0x00cf, .end = 0x00cf, .code = 0x49, .nBytes = 1 },
    { .start = 0x00d1, .end = 0x00d2, .code = 0x4e, .nBytes = 1 },     { .start = 0x00d3, .end = 0x00d3, .code = 0x4f, .nBytes = 1 },     { .start = 0x00d4, .end = 0x00d4, .code = 0x4f, .nBytes = 1 },
    { .start = 0x00d5, .end = 0x00d5, .code = 0x4f, .nBytes = 1 },     { .start = 0x00d6, .end = 0x00d6, .code = 0x4f, .nBytes = 1 },     { .start = 0x00d7, .end = 0x00d7, .code = 0x78, .nBytes = 1 },
    { .start = 0x00d8, .end = 0x00d8, .code = 0x4f, .nBytes = 1 },     { .start = 0x00d9, .end = 0x00d9, .code = 0x55, .nBytes = 1 },     { .start = 0x00da, .end = 0x00da, .code = 0x55, .nBytes = 1 },
    { .start = 0x00db, .end = 0x00db, .code = 0x55, .nBytes = 1 },     { .start = 0x00dc, .end = 0x00dc, .code = 0x55, .nBytes = 1 },     { .start = 0x00dd, .end = 0x00dd, .code = 0x59, .nBytes = 1 },
    { .start = 0x00e0, .end = 0x00e0, .code = 0x61, .nBytes = 1 },     { .start = 0x00e1, .end = 0x00e1, .code = 0x61, .nBytes = 1 },     { .start = 0x00e2, .end = 0x00e2, .code = 0x61, .nBytes = 1 },
    { .start = 0x00e3, .end = 0x00e3, .code = 0x61, .nBytes = 1 },     { .start = 0x00e4, .end = 0x00e4, .code = 0x61, .nBytes = 1 },     { .start = 0x00e5, .end = 0x00e5, .code = 0x61, .nBytes = 1 },
    { .start = 0x00e6, .end = 0x00e6, .code = 0x6165, .nBytes = 2 },   { .start = 0x00e7, .end = 0x00e7, .code = 0x63, .nBytes = 1 },     { .start = 0x00e8, .end = 0x00e8, .code = 0x65, .nBytes = 1 },
    { .start = 0x00e9, .end = 0x00e9, .code = 0x65, .nBytes = 1 },     { .start = 0x00ea, .end = 0x00ea, .code = 0x65, .nBytes = 1 },     { .start = 0x00eb, .end = 0x00eb, .code = 0x65, .nBytes = 1 },
    { .start = 0x00ec, .end = 0x00ec, .code = 0x69, .nBytes = 1 },     { .start = 0x00ed, .end = 0x00ed, .code = 0x69, .nBytes = 1 },     { .start = 0x00ee, .end = 0x00ee, .code = 0x69, .nBytes = 1 },
    { .start = 0x00ef, .end = 0x00ef, .code = 0x69, .nBytes = 1 },     { .start = 0x00f1, .end = 0x00f2, .code = 0x6e, .nBytes = 1 },     { .start = 0x00f3, .end = 0x00f3, .code = 0x6f, .nBytes = 1 },
    { .start = 0x00f4, .end = 0x00f4, .code = 0x6f, .nBytes = 1 },     { .start = 0x00f5, .end = 0x00f5, .code = 0x6f, .nBytes = 1 },     { .start = 0x00f6, .end = 0x00f6, .code = 0x6f, .nBytes = 1 },
    { .start = 0x00f7, .end = 0x00f7, .code = 0x2f, .nBytes = 1 },     { .start = 0x00f8, .end = 0x00f8, .code = 0x6f, .nBytes = 1 },     { .start = 0x00f9, .end = 0x00f9, .code = 0x75, .nBytes = 1 },
    { .start = 0x00fa, .end = 0x00fa, .code = 0x75, .nBytes = 1 },     { .start = 0x00fb, .end = 0x00fb, .code = 0x75, .nBytes = 1 },     { .start = 0x00fc, .end = 0x00fc, .code = 0x75, .nBytes = 1 },
    { .start = 0x00fd, .end = 0x00fd, .code = 0x79, .nBytes = 1 },     { .start = 0x00ff, .end = 0x00ff, .code = 0x79, .nBytes = 1 },     { .start = 0x0131, .end = 0x0131, .code = 0x69, .nBytes = 1 },
    { .start = 0x0141, .end = 0x0141, .code = 0x4c, .nBytes = 1 },     { .start = 0x0152, .end = 0x0152, .code = 0x4f45, .nBytes = 2 },   { .start = 0x0153, .end = 0x0153, .code = 0x6f65, .nBytes = 2 },
    { .start = 0x0160, .end = 0x0160, .code = 0x53, .nBytes = 1 },     { .start = 0x0178, .end = 0x0178, .code = 0x59, .nBytes = 1 },     { .start = 0x017d, .end = 0x017d, .code = 0x5a, .nBytes = 1 },
    { .start = 0x2013, .end = 0x2013, .code = 0x2d, .nBytes = 1 },     { .start = 0x2014, .end = 0x2014, .code = 0x2d2d, .nBytes = 2 },   { .start = 0x2018, .end = 0x2018, .code = 0x60, .nBytes = 1 },
    { .start = 0x2019, .end = 0x2019, .code = 0x27, .nBytes = 1 },     { .start = 0x201c, .end = 0x201c, .code = 0x22, .nBytes = 1 },     { .start = 0x201d, .end = 0x201d, .code = 0x22, .nBytes = 1 },
    { .start = 0x2022, .end = 0x2022, .code = 0x2a, .nBytes = 1 },     { .start = 0x2026, .end = 0x2026, .code = 0x2e2e2e, .nBytes = 3 }, { .start = 0x2122, .end = 0x2122, .code = 0x544d, .nBytes = 2 },
    { .start = 0x2212, .end = 0x2212, .code = 0x2d, .nBytes = 1 },     { .start = 0xf6f9, .end = 0xf6f9, .code = 0x4c, .nBytes = 1 },     { .start = 0xf6fa, .end = 0xf6fa, .code = 0x4f45, .nBytes = 2 },
    { .start = 0xf6fd, .end = 0xf6fd, .code = 0x53, .nBytes = 1 },     { .start = 0xf6fe, .end = 0xf6fe, .code = 0x7e, .nBytes = 1 },     { .start = 0xf6ff, .end = 0xf6ff, .code = 0x5a, .nBytes = 1 },
    { .start = 0xf721, .end = 0xf721, .code = 0x21, .nBytes = 1 },     { .start = 0xf724, .end = 0xf724, .code = 0x24, .nBytes = 1 },     { .start = 0xf726, .end = 0xf726, .code = 0x26, .nBytes = 1 },
    { .start = 0xf730, .end = 0xf739, .code = 0x30, .nBytes = 1 },     { .start = 0xf73f, .end = 0xf73f, .code = 0x3f, .nBytes = 1 },     { .start = 0xf761, .end = 0xf77a, .code = 0x41, .nBytes = 1 },
    { .start = 0xf7e0, .end = 0xf7e0, .code = 0x41, .nBytes = 1 },     { .start = 0xf7e1, .end = 0xf7e1, .code = 0x41, .nBytes = 1 },     { .start = 0xf7e2, .end = 0xf7e2, .code = 0x41, .nBytes = 1 },
    { .start = 0xf7e3, .end = 0xf7e3, .code = 0x41, .nBytes = 1 },     { .start = 0xf7e4, .end = 0xf7e4, .code = 0x41, .nBytes = 1 },     { .start = 0xf7e5, .end = 0xf7e5, .code = 0x41, .nBytes = 1 },
    { .start = 0xf7e6, .end = 0xf7e6, .code = 0x4145, .nBytes = 2 },   { .start = 0xf7e7, .end = 0xf7e7, .code = 0x43, .nBytes = 1 },     { .start = 0xf7e8, .end = 0xf7e8, .code = 0x45, .nBytes = 1 },
    { .start = 0xf7e9, .end = 0xf7e9, .code = 0x45, .nBytes = 1 },     { .start = 0xf7ea, .end = 0xf7ea, .code = 0x45, .nBytes = 1 },     { .start = 0xf7eb, .end = 0xf7eb, .code = 0x45, .nBytes = 1 },
    { .start = 0xf7ec, .end = 0xf7ec, .code = 0x49, .nBytes = 1 },     { .start = 0xf7ed, .end = 0xf7ed, .code = 0x49, .nBytes = 1 },     { .start = 0xf7ee, .end = 0xf7ee, .code = 0x49, .nBytes = 1 },
    { .start = 0xf7ef, .end = 0xf7ef, .code = 0x49, .nBytes = 1 },     { .start = 0xf7f1, .end = 0xf7f2, .code = 0x4e, .nBytes = 1 },     { .start = 0xf7f3, .end = 0xf7f3, .code = 0x4f, .nBytes = 1 },
    { .start = 0xf7f4, .end = 0xf7f4, .code = 0x4f, .nBytes = 1 },     { .start = 0xf7f5, .end = 0xf7f5, .code = 0x4f, .nBytes = 1 },     { .start = 0xf7f6, .end = 0xf7f6, .code = 0x4f, .nBytes = 1 },
    { .start = 0xf7f8, .end = 0xf7f8, .code = 0x4f, .nBytes = 1 },     { .start = 0xf7f9, .end = 0xf7f9, .code = 0x55, .nBytes = 1 },     { .start = 0xf7fa, .end = 0xf7fa, .code = 0x55, .nBytes = 1 },
    { .start = 0xf7fb, .end = 0xf7fb, .code = 0x55, .nBytes = 1 },     { .start = 0xf7fc, .end = 0xf7fc, .code = 0x55, .nBytes = 1 },     { .start = 0xf7fd, .end = 0xf7fd, .code = 0x59, .nBytes = 1 },
    { .start = 0xf7ff, .end = 0xf7ff, .code = 0x59, .nBytes = 1 },     { .start = 0xfb00, .end = 0xfb00, .code = 0x6666, .nBytes = 2 },   { .start = 0xfb01, .end = 0xfb01, .code = 0x6669, .nBytes = 2 },
    { .start = 0xfb02, .end = 0xfb02, .code = 0x666c, .nBytes = 2 },   { .start = 0xfb03, .end = 0xfb03, .code = 0x666669, .nBytes = 3 }, { .start = 0xfb04, .end = 0xfb04, .code = 0x66666c, .nBytes = 3 },
    { .start = 0xfb05, .end = 0xfb05, .code = 0x7374, .nBytes = 2 },   { .start = 0xfb06, .end = 0xfb06, .code = 0x7374, .nBytes = 2 }
};
#define ascii7UnicodeMapLen (sizeof(ascii7UnicodeMapRanges) / sizeof(UnicodeMapRange))

static const UnicodeMapRange symbolUnicodeMapRanges[] = {
    { .start = 0x0020, .end = 0x0021, .code = 0x20, .nBytes = 1 }, { .start = 0x0023, .end = 0x0023, .code = 0x23, .nBytes = 1 }, { .start = 0x0025, .end = 0x0026, .code = 0x25, .nBytes = 1 },
    { .start = 0x0028, .end = 0x0029, .code = 0x28, .nBytes = 1 }, { .start = 0x002b, .end = 0x002c, .code = 0x2b, .nBytes = 1 }, { .start = 0x002e, .end = 0x003f, .code = 0x2e, .nBytes = 1 },
    { .start = 0x005b, .end = 0x005b, .code = 0x5b, .nBytes = 1 }, { .start = 0x005d, .end = 0x005d, .code = 0x5d, .nBytes = 1 }, { .start = 0x005f, .end = 0x005f, .code = 0x5f, .nBytes = 1 },
    { .start = 0x007b, .end = 0x007d, .code = 0x7b, .nBytes = 1 }, { .start = 0x00ac, .end = 0x00ac, .code = 0xd8, .nBytes = 1 }, { .start = 0x00b0, .end = 0x00b1, .code = 0xb0, .nBytes = 1 },
    { .start = 0x00b5, .end = 0x00b5, .code = 0x6d, .nBytes = 1 }, { .start = 0x00d7, .end = 0x00d7, .code = 0xb4, .nBytes = 1 }, { .start = 0x00f7, .end = 0x00f7, .code = 0xb8, .nBytes = 1 },
    { .start = 0x0192, .end = 0x0192, .code = 0xa6, .nBytes = 1 }, { .start = 0x0391, .end = 0x0392, .code = 0x41, .nBytes = 1 }, { .start = 0x0393, .end = 0x0393, .code = 0x47, .nBytes = 1 },
    { .start = 0x0395, .end = 0x0395, .code = 0x45, .nBytes = 1 }, { .start = 0x0396, .end = 0x0396, .code = 0x5a, .nBytes = 1 }, { .start = 0x0397, .end = 0x0397, .code = 0x48, .nBytes = 1 },
    { .start = 0x0398, .end = 0x0398, .code = 0x51, .nBytes = 1 }, { .start = 0x0399, .end = 0x0399, .code = 0x49, .nBytes = 1 }, { .start = 0x039a, .end = 0x039d, .code = 0x4b, .nBytes = 1 },
    { .start = 0x039e, .end = 0x039e, .code = 0x58, .nBytes = 1 }, { .start = 0x039f, .end = 0x03a0, .code = 0x4f, .nBytes = 1 }, { .start = 0x03a1, .end = 0x03a1, .code = 0x52, .nBytes = 1 },
    { .start = 0x03a3, .end = 0x03a5, .code = 0x53, .nBytes = 1 }, { .start = 0x03a6, .end = 0x03a6, .code = 0x46, .nBytes = 1 }, { .start = 0x03a7, .end = 0x03a7, .code = 0x43, .nBytes = 1 },
    { .start = 0x03a8, .end = 0x03a8, .code = 0x59, .nBytes = 1 }, { .start = 0x03b1, .end = 0x03b2, .code = 0x61, .nBytes = 1 }, { .start = 0x03b3, .end = 0x03b3, .code = 0x67, .nBytes = 1 },
    { .start = 0x03b4, .end = 0x03b5, .code = 0x64, .nBytes = 1 }, { .start = 0x03b6, .end = 0x03b6, .code = 0x7a, .nBytes = 1 }, { .start = 0x03b7, .end = 0x03b7, .code = 0x68, .nBytes = 1 },
    { .start = 0x03b8, .end = 0x03b8, .code = 0x71, .nBytes = 1 }, { .start = 0x03b9, .end = 0x03b9, .code = 0x69, .nBytes = 1 }, { .start = 0x03ba, .end = 0x03bb, .code = 0x6b, .nBytes = 1 },
    { .start = 0x03bd, .end = 0x03bd, .code = 0x6e, .nBytes = 1 }, { .start = 0x03be, .end = 0x03be, .code = 0x78, .nBytes = 1 }, { .start = 0x03bf, .end = 0x03c0, .code = 0x6f, .nBytes = 1 },
    { .start = 0x03c1, .end = 0x03c1, .code = 0x72, .nBytes = 1 }, { .start = 0x03c2, .end = 0x03c2, .code = 0x56, .nBytes = 1 }, { .start = 0x03c3, .end = 0x03c5, .code = 0x73, .nBytes = 1 },
    { .start = 0x03c6, .end = 0x03c6, .code = 0x66, .nBytes = 1 }, { .start = 0x03c7, .end = 0x03c7, .code = 0x63, .nBytes = 1 }, { .start = 0x03c8, .end = 0x03c8, .code = 0x79, .nBytes = 1 },
    { .start = 0x03c9, .end = 0x03c9, .code = 0x77, .nBytes = 1 }, { .start = 0x03d1, .end = 0x03d1, .code = 0x4a, .nBytes = 1 }, { .start = 0x03d2, .end = 0x03d2, .code = 0xa1, .nBytes = 1 },
    { .start = 0x03d5, .end = 0x03d5, .code = 0x6a, .nBytes = 1 }, { .start = 0x03d6, .end = 0x03d6, .code = 0x76, .nBytes = 1 }, { .start = 0x2022, .end = 0x2022, .code = 0xb7, .nBytes = 1 },
    { .start = 0x2026, .end = 0x2026, .code = 0xbc, .nBytes = 1 }, { .start = 0x2032, .end = 0x2032, .code = 0xa2, .nBytes = 1 }, { .start = 0x2033, .end = 0x2033, .code = 0xb2, .nBytes = 1 },
    { .start = 0x2044, .end = 0x2044, .code = 0xa4, .nBytes = 1 }, { .start = 0x2111, .end = 0x2111, .code = 0xc1, .nBytes = 1 }, { .start = 0x2118, .end = 0x2118, .code = 0xc3, .nBytes = 1 },
    { .start = 0x211c, .end = 0x211c, .code = 0xc2, .nBytes = 1 }, { .start = 0x2126, .end = 0x2126, .code = 0x57, .nBytes = 1 }, { .start = 0x2135, .end = 0x2135, .code = 0xc0, .nBytes = 1 },
    { .start = 0x2190, .end = 0x2193, .code = 0xac, .nBytes = 1 }, { .start = 0x2194, .end = 0x2194, .code = 0xab, .nBytes = 1 }, { .start = 0x21b5, .end = 0x21b5, .code = 0xbf, .nBytes = 1 },
    { .start = 0x21d0, .end = 0x21d3, .code = 0xdc, .nBytes = 1 }, { .start = 0x21d4, .end = 0x21d4, .code = 0xdb, .nBytes = 1 }, { .start = 0x2200, .end = 0x2200, .code = 0x22, .nBytes = 1 },
    { .start = 0x2202, .end = 0x2202, .code = 0xb6, .nBytes = 1 }, { .start = 0x2203, .end = 0x2203, .code = 0x24, .nBytes = 1 }, { .start = 0x2205, .end = 0x2205, .code = 0xc6, .nBytes = 1 },
    { .start = 0x2206, .end = 0x2206, .code = 0x44, .nBytes = 1 }, { .start = 0x2207, .end = 0x2207, .code = 0xd1, .nBytes = 1 }, { .start = 0x2208, .end = 0x2209, .code = 0xce, .nBytes = 1 },
    { .start = 0x220b, .end = 0x220b, .code = 0x27, .nBytes = 1 }, { .start = 0x220f, .end = 0x220f, .code = 0xd5, .nBytes = 1 }, { .start = 0x2211, .end = 0x2211, .code = 0xe5, .nBytes = 1 },
    { .start = 0x2212, .end = 0x2212, .code = 0x2d, .nBytes = 1 }, { .start = 0x2217, .end = 0x2217, .code = 0x2a, .nBytes = 1 }, { .start = 0x221a, .end = 0x221a, .code = 0xd6, .nBytes = 1 },
    { .start = 0x221d, .end = 0x221d, .code = 0xb5, .nBytes = 1 }, { .start = 0x221e, .end = 0x221e, .code = 0xa5, .nBytes = 1 }, { .start = 0x2220, .end = 0x2220, .code = 0xd0, .nBytes = 1 },
    { .start = 0x2227, .end = 0x2228, .code = 0xd9, .nBytes = 1 }, { .start = 0x2229, .end = 0x222a, .code = 0xc7, .nBytes = 1 }, { .start = 0x222b, .end = 0x222b, .code = 0xf2, .nBytes = 1 },
    { .start = 0x2234, .end = 0x2234, .code = 0x5c, .nBytes = 1 }, { .start = 0x223c, .end = 0x223c, .code = 0x7e, .nBytes = 1 }, { .start = 0x2245, .end = 0x2245, .code = 0x40, .nBytes = 1 },
    { .start = 0x2248, .end = 0x2248, .code = 0xbb, .nBytes = 1 }, { .start = 0x2260, .end = 0x2261, .code = 0xb9, .nBytes = 1 }, { .start = 0x2264, .end = 0x2264, .code = 0xa3, .nBytes = 1 },
    { .start = 0x2265, .end = 0x2265, .code = 0xb3, .nBytes = 1 }, { .start = 0x2282, .end = 0x2282, .code = 0xcc, .nBytes = 1 }, { .start = 0x2283, .end = 0x2283, .code = 0xc9, .nBytes = 1 },
    { .start = 0x2284, .end = 0x2284, .code = 0xcb, .nBytes = 1 }, { .start = 0x2286, .end = 0x2286, .code = 0xcd, .nBytes = 1 }, { .start = 0x2287, .end = 0x2287, .code = 0xca, .nBytes = 1 },
    { .start = 0x2295, .end = 0x2295, .code = 0xc5, .nBytes = 1 }, { .start = 0x2297, .end = 0x2297, .code = 0xc4, .nBytes = 1 }, { .start = 0x22a5, .end = 0x22a5, .code = 0x5e, .nBytes = 1 },
    { .start = 0x22c5, .end = 0x22c5, .code = 0xd7, .nBytes = 1 }, { .start = 0x2320, .end = 0x2320, .code = 0xf3, .nBytes = 1 }, { .start = 0x2321, .end = 0x2321, .code = 0xf5, .nBytes = 1 },
    { .start = 0x2329, .end = 0x2329, .code = 0xe1, .nBytes = 1 }, { .start = 0x232a, .end = 0x232a, .code = 0xf1, .nBytes = 1 }, { .start = 0x25ca, .end = 0x25ca, .code = 0xe0, .nBytes = 1 },
    { .start = 0x2660, .end = 0x2660, .code = 0xaa, .nBytes = 1 }, { .start = 0x2663, .end = 0x2663, .code = 0xa7, .nBytes = 1 }, { .start = 0x2665, .end = 0x2665, .code = 0xa9, .nBytes = 1 },
    { .start = 0x2666, .end = 0x2666, .code = 0xa8, .nBytes = 1 }, { .start = 0xf6d9, .end = 0xf6d9, .code = 0xd3, .nBytes = 1 }, { .start = 0xf6da, .end = 0xf6da, .code = 0xd2, .nBytes = 1 },
    { .start = 0xf6db, .end = 0xf6db, .code = 0xd4, .nBytes = 1 }, { .start = 0xf8e5, .end = 0xf8e5, .code = 0x60, .nBytes = 1 }, { .start = 0xf8e6, .end = 0xf8e7, .code = 0xbd, .nBytes = 1 },
    { .start = 0xf8e8, .end = 0xf8ea, .code = 0xe2, .nBytes = 1 }, { .start = 0xf8eb, .end = 0xf8f4, .code = 0xe6, .nBytes = 1 }, { .start = 0xf8f5, .end = 0xf8f5, .code = 0xf4, .nBytes = 1 },
    { .start = 0xf8f6, .end = 0xf8fe, .code = 0xf6, .nBytes = 1 }
};
#define symbolUnicodeMapLen (sizeof(symbolUnicodeMapRanges) / sizeof(UnicodeMapRange))

static const UnicodeMapRange zapfDingbatsUnicodeMapRanges[] = {
    { .start = 0x0020, .end = 0x0020, .code = 0x20, .nBytes = 1 }, { .start = 0x2192, .end = 0x2192, .code = 0xd5, .nBytes = 1 }, { .start = 0x2194, .end = 0x2195, .code = 0xd6, .nBytes = 1 },
    { .start = 0x2460, .end = 0x2469, .code = 0xac, .nBytes = 1 }, { .start = 0x25a0, .end = 0x25a0, .code = 0x6e, .nBytes = 1 }, { .start = 0x25b2, .end = 0x25b2, .code = 0x73, .nBytes = 1 },
    { .start = 0x25bc, .end = 0x25bc, .code = 0x74, .nBytes = 1 }, { .start = 0x25c6, .end = 0x25c6, .code = 0x75, .nBytes = 1 }, { .start = 0x25cf, .end = 0x25cf, .code = 0x6c, .nBytes = 1 },
    { .start = 0x25d7, .end = 0x25d7, .code = 0x77, .nBytes = 1 }, { .start = 0x2605, .end = 0x2605, .code = 0x48, .nBytes = 1 }, { .start = 0x260e, .end = 0x260e, .code = 0x25, .nBytes = 1 },
    { .start = 0x261b, .end = 0x261b, .code = 0x2a, .nBytes = 1 }, { .start = 0x261e, .end = 0x261e, .code = 0x2b, .nBytes = 1 }, { .start = 0x2660, .end = 0x2660, .code = 0xab, .nBytes = 1 },
    { .start = 0x2663, .end = 0x2663, .code = 0xa8, .nBytes = 1 }, { .start = 0x2665, .end = 0x2665, .code = 0xaa, .nBytes = 1 }, { .start = 0x2666, .end = 0x2666, .code = 0xa9, .nBytes = 1 },
    { .start = 0x2701, .end = 0x2704, .code = 0x21, .nBytes = 1 }, { .start = 0x2706, .end = 0x2709, .code = 0x26, .nBytes = 1 }, { .start = 0x270c, .end = 0x2727, .code = 0x2c, .nBytes = 1 },
    { .start = 0x2729, .end = 0x274b, .code = 0x49, .nBytes = 1 }, { .start = 0x274d, .end = 0x274d, .code = 0x6d, .nBytes = 1 }, { .start = 0x274f, .end = 0x2752, .code = 0x6f, .nBytes = 1 },
    { .start = 0x2756, .end = 0x2756, .code = 0x76, .nBytes = 1 }, { .start = 0x2758, .end = 0x275e, .code = 0x78, .nBytes = 1 }, { .start = 0x2761, .end = 0x2767, .code = 0xa1, .nBytes = 1 },
    { .start = 0x2776, .end = 0x2794, .code = 0xb6, .nBytes = 1 }, { .start = 0x2798, .end = 0x27af, .code = 0xd8, .nBytes = 1 }, { .start = 0x27b1, .end = 0x27be, .code = 0xf1, .nBytes = 1 }
};
#define zapfDingbatsUnicodeMapLen (sizeof(zapfDingbatsUnicodeMapRanges) / sizeof(UnicodeMapRange))
