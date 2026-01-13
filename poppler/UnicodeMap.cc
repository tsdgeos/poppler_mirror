//========================================================================
//
// UnicodeMap.cc
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
// Copyright (C) 2010 Jakub Wilk <jwilk@jwilk.net>
// Copyright (C) 2017-2020, 2022, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2017 Jean Ghali <jghali@libertysurf.fr>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstdio>
#include <cstring>
#include "goo/gfile.h"
#include "goo/glibc.h"
#include "Error.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"

// helper for using std::visit to get a dependent false for static_asserts
// to help get compile errors if one ever extends variants
template<class>
inline constexpr bool always_false_v = false;

//------------------------------------------------------------------------

std::unique_ptr<UnicodeMap> UnicodeMap::parse(const std::string &encodingNameA)
{
    FILE *f;
    char buf[256];
    int line, nBytes;
    char *tok1, *tok2, *tok3;
    char *tokptr;

    if (!(f = globalParams->getUnicodeMapFile(encodingNameA))) {
        error(errSyntaxError, -1, "Couldn't find unicodeMap file for the '{0:s}' encoding", encodingNameA.c_str());
        return {};
    }

    auto map = std::unique_ptr<UnicodeMap>(new UnicodeMap(encodingNameA));

    std::vector<UnicodeMapRange> customRanges;
    std::vector<UnicodeMapExt> eMap;

    line = 1;
    while (getLine(buf, sizeof(buf), f)) {
        if ((tok1 = strtok_r(buf, " \t\r\n", &tokptr)) && (tok2 = strtok_r(nullptr, " \t\r\n", &tokptr))) {
            if (!(tok3 = strtok_r(nullptr, " \t\r\n", &tokptr))) {
                tok3 = tok2;
                tok2 = tok1;
            }
            nBytes = strlen(tok3) / 2;
            if (nBytes <= 4) {
                UnicodeMapRange range;
                sscanf(tok1, "%x", &range.start);
                sscanf(tok2, "%x", &range.end);
                sscanf(tok3, "%x", &range.code);
                range.nBytes = nBytes;
                customRanges.push_back(range);
            } else if (tok2 == tok1) {
                UnicodeMapExt ext;
                sscanf(tok1, "%x", &ext.u);
                ext.code.reserve(nBytes);
                for (int i = 0; i < nBytes; ++i) {
                    unsigned int x;
                    sscanf(tok3 + i * 2, "%2x", &x);
                    ext.code.push_back((char)x);
                }
                eMap.push_back(std::move(ext));
            } else {
                error(errSyntaxError, -1, "Bad line ({0:d}) in unicodeMap file for the '{1:s}' encoding", line, encodingNameA.c_str());
            }
        } else {
            error(errSyntaxError, -1, "Bad line ({0:d}) in unicodeMap file for the '{1:s}' encoding", line, encodingNameA.c_str());
        }
        ++line;
    }

    fclose(f);

    map->eMaps = std::move(eMap);
    map->data = std::move(customRanges);
    return map;
}

UnicodeMap::UnicodeMap(const std::string &encodingNameA)
{
    encodingName = encodingNameA;
    unicodeOut = false;
}

UnicodeMap::UnicodeMap(const char *encodingNameA, bool unicodeOutA, std::span<const UnicodeMapRange> rangesA)
{
    encodingName = encodingNameA;
    unicodeOut = unicodeOutA;
    data = rangesA;
}

UnicodeMap::UnicodeMap(const char *encodingNameA, bool unicodeOutA, UnicodeMapFunc funcA)
{
    encodingName = encodingNameA;
    unicodeOut = unicodeOutA;
    data = funcA;
}

UnicodeMap::~UnicodeMap() = default;

UnicodeMap::UnicodeMap(UnicodeMap &&other) noexcept : encodingName { std::move(other.encodingName) }, unicodeOut { other.unicodeOut }, data { std::move(other.data) }, eMaps { std::move(other.eMaps) } { }

UnicodeMap &UnicodeMap::operator=(UnicodeMap &&other) noexcept
{
    if (this != &other) {
        swap(other);
    }
    return *this;
}

void UnicodeMap::swap(UnicodeMap &other) noexcept
{
    using std::swap;
    swap(encodingName, other.encodingName);
    swap(unicodeOut, other.unicodeOut);
    swap(data, other.data);
    swap(eMaps, other.eMaps);
}

bool UnicodeMap::match(const std::string &encodingNameA) const
{
    return encodingName == encodingNameA;
}

int UnicodeMap::mapUnicode(Unicode u, char *buf, int bufSize) const
{

    return std::visit(
            [this, u, buf, bufSize](auto &&item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, UnicodeMapFunc>) {
                    return (*item)(u, buf, bufSize);
                } else if constexpr (std::is_same_v<T, std::span<const UnicodeMapRange>> || std::is_same_v<T, std::vector<UnicodeMapRange>>) {
                    int a = 0;
                    int b = (int)item.size();
                    if (u >= item[a].start) {
                        // invariant: item[a].start <= u < item[b].start
                        while (b - a > 1) {
                            int m = (a + b) / 2;
                            if (u >= item[m].start) {
                                a = m;
                            } else if (u < item[m].start) {
                                b = m;
                            }
                        }
                        if (u <= item[a].end) {
                            int n = item[a].nBytes;
                            if (n > bufSize) {
                                return 0;
                            }
                            unsigned int code = item[a].code + (u - item[a].start);
                            for (int i = n - 1; i >= 0; --i) {
                                buf[i] = (char)(code & 0xff);
                                code >>= 8;
                            }
                            return n;
                        }
                    }
                    for (const UnicodeMapExt &ext : eMaps) {
                        if (ext.u == u) {
                            const int codeSize = ext.code.size();
                            if (codeSize > bufSize) {
                                return 0;
                            }
                            for (int j = 0; j < codeSize; ++j) {
                                buf[j] = ext.code[j];
                            }
                            return codeSize;
                        }
                    }
                    return 0;
                } else {
                    static_assert(always_false_v<T>);
                }
            },
            data);
}

//------------------------------------------------------------------------

UnicodeMapCache::UnicodeMapCache() = default;

const UnicodeMap *UnicodeMapCache::getUnicodeMap(const std::string &encodingName)
{
    for (const std::unique_ptr<UnicodeMap> &map : cache) {
        if (map->match(encodingName)) {
            return map.get();
        }
    }
    std::unique_ptr<UnicodeMap> map = UnicodeMap::parse(encodingName);
    if (map) {
        UnicodeMap *m = map.get();
        cache.emplace_back(std::move(map));
        return m;
    }
    return nullptr;
}
