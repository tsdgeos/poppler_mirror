//========================================================================
//
// NameToCharCode.h
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
// Copyright (C) 2018, 2019 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef NAMETOCHARCODE_H
#define NAMETOCHARCODE_H

#include "CharTypes.h"
#include <unordered_map>
#include <string>

struct NameToCharCodeEntry;

//------------------------------------------------------------------------

class NameToCharCode
{
public:
    void add(const char *name, CharCode c);
    CharCode lookup(const char *name) const;

private:
    /*This is a helper struct to allow looking up by char* into a std string unordered_map*/
    struct string_hasher
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char *txt) const { return std::hash<std::string_view> {}(txt); }
        [[nodiscard]] size_t operator()(const std::string &txt) const { return std::hash<std::string> {}(txt); }
    };

    std::unordered_map<std::string, CharCode, string_hasher, std::equal_to<>> tab;
};

#endif
