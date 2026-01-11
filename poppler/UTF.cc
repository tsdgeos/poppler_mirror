//========================================================================
//
// UTF.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2008 Koji Otani <sho@bbr.jp>
// Copyright (C) 2012, 2017, 2021, 2023, 2024 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2012 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2016, 2018-2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2016 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018, 2020 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright (C) 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2023 Even Rouault <even.rouault@spatialys.com>
// Copyright (C) 2023, 2024 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "goo/gmem.h"
#include "PDFDocEncoding.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"
#include "UTF.h"
#include "UnicodeMapFuncs.h"
#include <algorithm>

#include <config.h>

std::vector<Unicode> UTF16toUCS4(std::span<const Unicode> utf16)
{
    // count characters
    int len = 0;
    for (size_t i = 0; i < utf16.size(); i++) {
        if (utf16[i] >= 0xd800 && utf16[i] < 0xdc00 && i + 1 < utf16.size() && utf16[i + 1] >= 0xdc00 && utf16[i + 1] < 0xe000) {
            i++; /* surrogate pair */
        }
        len++;
    }
    std::vector<Unicode> u;
    u.reserve(len);
    // convert string
    for (size_t i = 0; i < utf16.size(); i++) {
        if (utf16[i] >= 0xd800 && utf16[i] < 0xdc00) { /* surrogate pair */
            if (i + 1 < utf16.size() && utf16[i + 1] >= 0xdc00 && utf16[i + 1] < 0xe000) {
                /* next code is a low surrogate */
                u.push_back((((utf16[i] & 0x3ff) << 10) | (utf16[i + 1] & 0x3ff)) + 0x10000);
                ++i;
            } else {
                /* missing low surrogate
                   replace it with REPLACEMENT CHARACTER (U+FFFD) */
                u.push_back(0xfffd);
            }
        } else if (utf16[i] >= 0xdc00 && utf16[i] < 0xe000) {
            /* invalid low surrogate
               replace it with REPLACEMENT CHARACTER (U+FFFD) */
            u.push_back(0xfffd);
        } else {
            u.push_back(utf16[i]);
        }
        if (!UnicodeIsValid(u.back())) {
            u.back() = 0xfffd;
        }
    }
    return u;
}

std::vector<Unicode> TextStringToUCS4(std::string_view textStr)
{
    bool isUnicode, isUnicodeLE;

    int len = textStr.size();
    if (len == 0) {
        return {};
    }

    if (hasUnicodeByteOrderMark(textStr)) {
        isUnicode = true;
        isUnicodeLE = false;
    } else if (hasUnicodeByteOrderMarkLE(textStr)) {
        isUnicode = false;
        isUnicodeLE = true;
    } else {
        isUnicode = false;
        isUnicodeLE = false;
    }

    if (isUnicode || isUnicodeLE) {
        len = len / 2 - 1;
        if (len > 0) {
            std::vector<Unicode> utf16;
            utf16.reserve(len);
            for (int i = 0; i < len; i++) {
                if (isUnicode) {
                    utf16.push_back((textStr[2 + i * 2] & 0xff) << 8 | (textStr[3 + i * 2] & 0xff));
                } else { // UnicodeLE
                    utf16.push_back((textStr[3 + i * 2] & 0xff) << 8 | (textStr[2 + i * 2] & 0xff));
                }
            }
            return UTF16toUCS4(utf16);
        }
        return {};
    }
    std::vector<Unicode> u;
    u.reserve(len);
    for (int i = 0; i < len; i++) {
        u.push_back(pdfDocEncoding[textStr[i] & 0xff]);
    }
    return u;
}

bool UnicodeIsWhitespace(Unicode ucs4)
{
    static Unicode const spaces[] = { 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x0020, 0x0085, 0x00A0, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x2028, 0x2029, 0x202F, 0x205F, 0x3000 };
    Unicode const *end = spaces + sizeof(spaces) / sizeof(spaces[0]);
    Unicode const *i = std::lower_bound(spaces, end, ucs4);
    return (i != end && *i == ucs4);
}

//
// decodeUtf8() and decodeUtf8Table are:
//
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
//
static const uint32_t UTF8_ACCEPT = 0;
static const uint32_t UTF8_REJECT = 12;
static const uint32_t UCS4_MAX = 0x10FFFF;
static const Unicode REPLACEMENT_CHAR = 0xFFFD;

// clang-format off
static const uint8_t decodeUtf8Table[] = {
  // The first part of the table maps bytes to character classes
  // to reduce the size of the transition table and create bitmasks.
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8, // e0..ff

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};
// clang-format on

// Decode utf8 state machine for fast UTF-8 decoding. Initialise state
// to 0 and call decodeUtf8() for each byte of UTF-8. Return value
// (and state) is UTF8_ACCEPT when it has found a valid codepoint
// (codepoint returned in codep), UTF8_REJECT when the byte is not
// allowed to occur at its position, and some other positive value if
// more bytes have to be read.  Reset state to 0 to recover from
// errors.
inline uint32_t decodeUtf8(uint32_t *state, uint32_t *codep, char byte)
{
    uint32_t b = (unsigned char)byte;
    uint32_t type = decodeUtf8Table[b];

    *codep = (*state != UTF8_ACCEPT) ? (b & 0x3fU) | (*codep << 6) : (0xff >> type) & (b);

    *state = decodeUtf8Table[256 + *state + type];
    return *state;
}

std::vector<Unicode> utf8ToUCS4(std::string_view utf8)
{
    uint32_t codepoint;
    uint32_t state = 0;

    std::vector<Unicode> u;

    for (auto c : utf8) {
        decodeUtf8(&state, &codepoint, c);
        if (state == UTF8_ACCEPT) {
            u.push_back(codepoint);
        } else if (state == UTF8_REJECT) {
            u.push_back(REPLACEMENT_CHAR); // invalid byte for this position
            state = 0;
        }
    }
    if (state != UTF8_ACCEPT && state != UTF8_REJECT) {
        u.push_back(REPLACEMENT_CHAR); // invalid byte for this position
    }
    u.shrink_to_fit();

    return u;
}

// Count number of UTF-16 code units required to convert a UTF-8 string
// (excluding terminating NULL). Each invalid byte is counted as a
// code point since the UTF-8 conversion functions will replace it with
// REPLACEMENT_CHAR.
int utf8CountUtf16CodeUnits(std::string_view utf8)
{
    uint32_t codepoint;
    uint32_t state = 0;
    int count = 0;

    for (auto c : utf8) {
        decodeUtf8(&state, &codepoint, c);
        if (state == UTF8_ACCEPT) {
            if (codepoint < 0x10000) {
                count++;
            } else if (codepoint <= UCS4_MAX) {
                count += 2;
            } else {
                count++; // replace with REPLACEMENT_CHAR
            }
        } else if (state == UTF8_REJECT) {
            count++; // replace with REPLACEMENT_CHAR
            state = 0;
        }
    }
    if (state != UTF8_ACCEPT && state != UTF8_REJECT) {
        count++; // replace with REPLACEMENT_CHAR
    }

    return count;
}

std::u16string utf8ToUtf16(std::string_view utf8)
{
    uint32_t codepoint;
    uint32_t state = 0;
    if (isUtf8WithBom(utf8)) {
        utf8 = utf8.substr(3);
    }
    std::u16string utf16;
    for (auto c : utf8) {
        decodeUtf8(&state, &codepoint, c);
        if (state == UTF8_ACCEPT) {
            if (codepoint < 0x10000) {
                utf16.push_back((uint16_t)codepoint);
            } else if (codepoint <= UCS4_MAX) {
                utf16.push_back((uint16_t)(0xD7C0 + (codepoint >> 10)));
                utf16.push_back((uint16_t)(0xDC00 + (codepoint & 0x3FF)));
            } else {
                utf16.push_back(REPLACEMENT_CHAR);
                state = 0;
            }
        } else if (state == UTF8_REJECT) {
            utf16.push_back(REPLACEMENT_CHAR); // invalid byte for this position
        }
    }
    // replace any trailing bytes too short for a valid UTF-8 with a replacement char
    if (state != UTF8_ACCEPT && state != UTF8_REJECT) {
        utf16.push_back(REPLACEMENT_CHAR);
    }
    return utf16;
}

std::string utf8ToUtf16WithBom(std::string_view utf8)
{
    if (utf8.empty()) {
        return {};
    }
    std::u16string utf16 = utf8ToUtf16(utf8);
    char *tmp_str = (char *)utf16.data();
#if !WORDS_BIGENDIAN
    for (size_t i = 0; i < utf16.size(); i++) {
        std::swap(tmp_str[i * 2], tmp_str[i * 2 + 1]);
    }
#endif

    std::string result(unicodeByteOrderMark);
    result.append(tmp_str, utf16.size() * 2);
    return result;
}

static const uint32_t UTF16_ACCEPT = 0;
static const uint32_t UTF16_REJECT = -1;

// Initialise state to 0. Returns UTF16_ACCEPT when a valid code point
// has been found, UTF16_REJECT when invalid code unit for this state,
// some other valid if another code unit needs to be read.
inline uint32_t decodeUtf16(uint32_t *state, uint32_t *codePoint, uint16_t codeUnit)
{
    if (*state == 0) {
        if (codeUnit >= 0xd800 && codeUnit < 0xdc00) { /* surrogate pair */
            *state = codeUnit;
            return *state;
        }
        if (codeUnit >= 0xdc00 && codeUnit < 0xe000) {
            /* invalid low surrogate */
            return UTF16_REJECT;
        }
        *codePoint = codeUnit;
        return UTF16_ACCEPT;
    }
    if (codeUnit >= 0xdc00 && codeUnit < 0xe000) {
        *codePoint = (((*state & 0x3ff) << 10) | (codeUnit & 0x3ff)) + 0x10000;
        *state = 0;
        return UTF16_ACCEPT;
    }
    /* invalid high surrogate */
    return UTF16_REJECT;
}

// Count number of UTF-8 bytes required to convert a UTF-16 string to
// UTF-8 (excluding terminating NULL).
int utf16CountUtf8Bytes(const uint16_t *utf16)
{
    uint32_t codepoint = 0;
    uint32_t state = 0;
    int count = 0;

    while (*utf16) {
        decodeUtf16(&state, &codepoint, *utf16);
        if (state == UTF16_ACCEPT) {
            if (codepoint < 0x80) {
                count++;
            } else if (codepoint < 0x800) {
                count += 2;
            } else if (codepoint < 0x10000) {
                count += 3;
            } else if (codepoint <= UCS4_MAX) {
                count += 4;
            } else {
                count += 3; // replace with REPLACEMENT_CHAR
            }
        } else if (state == UTF16_REJECT) {
            count += 3; // replace with REPLACEMENT_CHAR
            state = 0;
        }
        utf16++;
    }
    if (state != UTF8_ACCEPT && state != UTF8_REJECT) {
        count += 3; // replace with REPLACEMENT_CHAR
    }

    return count;
}

std::string utf16ToUtf8(const uint16_t *utf16, int maxUtf16)
{
    uint32_t codepoint = 0;
    uint32_t state = 0;
    int nIn = 0;
    char p[4];
    std::string utf8;
    while (*utf16 && nIn < maxUtf16) {
        decodeUtf16(&state, &codepoint, *utf16);
        if (state == UTF16_ACCEPT || state == UTF16_REJECT) {
            if (state == UTF16_REJECT || codepoint > UCS4_MAX) {
                codepoint = REPLACEMENT_CHAR;
                state = 0;
            }

            int count = mapUTF8(codepoint, p, 4);
            utf8.append(std::string_view(p, count));
        }
        utf16++;
        nIn++;
    }
    // replace any trailing bytes too short for a valid UTF-8 with a replacement char
    if (state != UTF16_ACCEPT && state != UTF16_REJECT) {
        int count = mapUTF8(REPLACEMENT_CHAR, p, 4);
        utf8.append(std::string_view(p, count));
    }
    return utf8;
}

void unicodeToAscii7(std::span<const Unicode> in, Unicode **ucs4_out, int *out_len, const int *in_idx, int **indices)
{
    const UnicodeMap *uMap = globalParams->getUnicodeMap("ASCII7");
    int *idx = nullptr;

    if (in.empty()) {
        *ucs4_out = nullptr;
        *out_len = 0;
        return;
    }

    if (indices) {
        if (!in_idx) {
            indices = nullptr;
        } else {
            idx = (int *)gmallocn(in.size() * 8 + 1, sizeof(int));
        }
    }

    std::string str;

    char buf[8]; // 8 is enough for mapping an unicode char to a string
    size_t i;
    int n, k;

    for (i = k = 0; i < in.size(); ++i) {
        n = uMap->mapUnicode(in[i], buf, sizeof(buf));
        if (!n) {
            // the Unicode char could not be converted to ascii7 counterpart
            // so just fill with a non-printable ascii char
            buf[0] = 31;
            n = 1;
        }
        str.append(buf, n);
        if (indices) {
            for (; n > 0; n--) {
                idx[k++] = in_idx[i];
            }
        }
    }

    std::vector<Unicode> ucs4 = TextStringToUCS4(str);
    *out_len = ucs4.size();
    *ucs4_out = (Unicode *)gmallocn(ucs4.size(), sizeof(Unicode));
    memcpy(*ucs4_out, ucs4.data(), ucs4.size() * sizeof(Unicode));

    if (indices) {
        idx[k] = in_idx[in.size()];
        *indices = idx;
    }
}

// Convert a PDF Text String to UTF-8
//   textStr    - PDF text string
//   returns UTF-8 string.
std::string TextStringToUtf8(std::string_view textStr)
{
    int i, len;
    std::string utf8;

    len = textStr.size();
    if (hasUnicodeByteOrderMark(textStr)) {
        std::vector<uint16_t> utf16;
        len = len / 2 - 1;
        utf16.resize(len + 1);
        for (i = 0; i < len; i++) {
            utf16[i] = (textStr[2 + i * 2] & 0xff) << 8 | (textStr[3 + i * 2] & 0xff);
        }
        utf16[i] = 0;
        utf8 = utf16ToUtf8(utf16.data(), utf16.size());
    } else {
        utf8.resize(len + 1);
        for (i = 0; i < len; i++) {
            utf8[i] = pdfDocEncoding[textStr[i] & 0xff];
        }
        utf8[i] = 0;
    }
    return utf8;
}
