//========================================================================
//
// NameToCharCode.cc
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
// Copyright (C) 2019, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "NameToCharCode.h"

void NameToCharCode::add(const char *name, CharCode c)
{
    tab.insert_or_assign(std::string { name }, c);
}

CharCode NameToCharCode::lookup(const char *name) const
{
    auto it = tab.find(name);
    if (it != tab.end()) {
        return it->second;
    }
    return 0;
}
