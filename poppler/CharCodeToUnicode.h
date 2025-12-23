//========================================================================
//
// CharCodeToUnicode.h
//
// Mapping from character codes to Unicode.
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
// Copyright (C) 2007 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2007 Koji Otani <sho@bbr.jp>
// Copyright (C) 2008, 2011, 2012, 2018, 2019, 2021, 2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 <corentinf@free.fr>
// Copyright (C) 2024, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef CHARCODETOUNICODE_H
#define CHARCODETOUNICODE_H

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <deque>

#include "CharTypes.h"

class GooString;

//------------------------------------------------------------------------

class CharCodeToUnicode
{
    friend class UnicodeToCharCode;
    struct PrivateTag
    {
    };
    struct CharCodeToUnicodeString
    {
        CharCode c;
        std::vector<Unicode> u;
    };

public:
    // Create an identity mapping (Unicode = CharCode).
    static std::unique_ptr<CharCodeToUnicode> makeIdentityMapping();

    // Read the CID-to-Unicode mapping for <collection> from the file
    // specified by <fileName>.  Sets the initial reference count to 1.
    // Returns NULL on failure.
    static std::unique_ptr<CharCodeToUnicode> parseCIDToUnicode(const char *fileName, const std::string &collection);

    // Create the CharCode-to-Unicode mapping for an 8-bit font.
    // <toUnicode> is an array of 256 Unicode indexes.  Sets the initial
    // reference count to 1.
    static std::unique_ptr<CharCodeToUnicode> make8BitToUnicode(Unicode *toUnicode);

    // Parse a ToUnicode CMap for an 8- or 16-bit font.
    static std::unique_ptr<CharCodeToUnicode> parseCMap(const std::string &buf, int nBits);
    static std::unique_ptr<CharCodeToUnicode> parseCMapFromFile(const std::string &fileName, int nBits);

    // Parse a ToUnicode CMap for an 8- or 16-bit font, merging it into
    // <this>.
    void mergeCMap(const std::string &buf, int nBits);

    ~CharCodeToUnicode() = default;

    CharCodeToUnicode(const CharCodeToUnicode &) = delete;
    CharCodeToUnicode &operator=(const CharCodeToUnicode &) = delete;

    // Return true if this mapping matches the specified <tagA>.
    bool match(const std::string &tagA);

    // Set the mapping for <c>.
    void setMapping(CharCode c, Unicode *u, int len);

    // Map a CharCode to Unicode. Returns a pointer in u to internal storage
    // so never store the pointers it returns, just the data, otherwise
    // your pointed values might get changed by future calls
    int mapToUnicode(CharCode c, Unicode const **u) const;

    // Map a Unicode to CharCode.
    int mapToCharCode(const Unicode *u, CharCode *c, int usize) const;

    explicit CharCodeToUnicode(PrivateTag t = {});
    explicit CharCodeToUnicode(const std::optional<std::string> &tagA, PrivateTag t = {});
    CharCodeToUnicode(const std::optional<std::string> &tagA, std::vector<Unicode> &&mapA, std::vector<CharCodeToUnicodeString> &&sMapA, PrivateTag t = {});

private:
    bool parseCMap1(int (*getCharFunc)(void *), void *data, int nBits);
    void addMapping(CharCode code, char *uStr, int n, int offset);
    void addMappingInt(CharCode code, Unicode u);

    const std::optional<std::string> tag;
    std::vector<Unicode> map;
    std::vector<CharCodeToUnicodeString> sMap;
    bool isIdentity;
};

//------------------------------------------------------------------------

class CharCodeToUnicodeCache
{
public:
    explicit CharCodeToUnicodeCache(int sizeA);
    ~CharCodeToUnicodeCache();

    CharCodeToUnicodeCache(const CharCodeToUnicodeCache &) = delete;
    CharCodeToUnicodeCache &operator=(const CharCodeToUnicodeCache &) = delete;

    // Get the CharCodeToUnicode object for <tag>.
    // Returns NULL on failure.
    std::shared_ptr<CharCodeToUnicode> getCharCodeToUnicode(const std::string &tag);

    // Insert <ctu> into the cache, in the most-recently-used position.
    void add(std::shared_ptr<CharCodeToUnicode> ctu);

private:
    size_t size;
    std::deque<std::shared_ptr<CharCodeToUnicode>> cache;
};

#endif
