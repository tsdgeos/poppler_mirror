//========================================================================
//
// Dict.cc
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
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2006 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright (C) 2007-2008 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2008, 2010, 2013, 2014, 2017, 2019, 2020, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010 Paweł Wiejacha <pawel.wiejacha@gmail.com>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2014 Scott West <scott.gregory.west@gmail.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <algorithm>
#include <ranges>

#include "XRef.h"
#include "Dict.h"

//------------------------------------------------------------------------
// Dict
//------------------------------------------------------------------------

#define dictLocker() const std::scoped_lock locker(mutex)

constexpr int SORT_LENGTH_LOWER_LIMIT = 32;

struct Dict::CmpDictEntry
{
    bool operator()(const DictEntry &lhs, const DictEntry &rhs) const { return lhs.first < rhs.first; }
    bool operator()(const DictEntry &lhs, std::string_view rhs) const { return lhs.first < rhs; }
    bool operator()(std::string_view lhs, const DictEntry &rhs) const { return lhs < rhs.first; }
};

Dict::Dict(XRef *xrefA)
{
    xref = xrefA;
    ref = 1;

    sorted = false;
}

Dict::Dict(const Dict *dictA)
{
    xref = dictA->xref;
    ref = 1;

    entries.reserve(dictA->entries.size());
    for (const auto &entry : dictA->entries) {
        entries.emplace_back(entry.first, entry.second.copy());
    }

    sorted = dictA->sorted.load();
}

Dict *Dict::copy(XRef *xrefA) const
{
    dictLocker();
    Dict *dictA = new Dict(this);
    dictA->xref = xrefA;
    for (auto &entry : dictA->entries) {
        if (entry.second.getType() == objDict) {
            entry.second = Object(entry.second.getDict()->copy(xrefA));
        }
    }
    return dictA;
}

Dict *Dict::deepCopy() const
{
    dictLocker();
    Dict *dictA = new Dict(xref);

    dictA->entries.reserve(entries.size());
    for (const auto &entry : entries) {
        dictA->entries.emplace_back(entry.first, entry.second.deepCopy());
    }
    return dictA;
}

void Dict::add(std::string_view key, Object &&val)
{
    dictLocker();
    entries.emplace_back(key, std::move(val));
    sorted = false;
}

inline const Dict::DictEntry *Dict::find(std::string_view key) const
{
    if (entries.size() >= SORT_LENGTH_LOWER_LIMIT) {
        if (!sorted) {
            dictLocker();
            if (!sorted) {
                Dict *that = const_cast<Dict *>(this);

                std::ranges::sort(that->entries, CmpDictEntry {});
                that->sorted = true;
            }
        }
    }

    if (sorted) {
        const auto pos = std::ranges::lower_bound(entries, key, std::less<> {}, &DictEntry::first);
        if (pos != entries.end() && pos->first == key) {
            return &*pos;
        }
    } else {
        const auto pos = std::ranges::find_if(std::ranges::reverse_view(entries), [key](const DictEntry &entry) { return entry.first == key; });
        if (pos != entries.rend()) {
            return &*pos;
        }
    }
    return nullptr;
}

inline Dict::DictEntry *Dict::find(std::string_view key)
{
    return const_cast<DictEntry *>(const_cast<const Dict *>(this)->find(key));
}

void Dict::remove(std::string_view key)
{
    dictLocker();
    if (auto *entry = find(key)) {
        if (sorted) {
            const auto index = entry - &entries.front();
            entries.erase(entries.begin() + index);
        } else {
            swap(*entry, entries.back());
            entries.pop_back();
        }
    }
}

void Dict::set(std::string_view key, Object &&val)
{
    if (val.isNull()) {
        remove(key);
        return;
    }
    dictLocker();
    if (auto *entry = find(key)) {
        entry->second = std::move(val);
    } else {
        add(key, std::move(val));
    }
}

bool Dict::is(std::string_view type) const
{
    if (const auto *entry = find("Type")) {
        return entry->second.isName(type);
    }
    return false;
}

Object Dict::lookup(std::string_view key, int recursion) const
{
    if (const auto *entry = find(key)) {
        return entry->second.fetch(xref, recursion);
    }
    return Object::null();
}

Object Dict::lookup(std::string_view key, Ref *returnRef, int recursion) const
{
    if (const auto *entry = find(key)) {
        if (entry->second.getType() == objRef) {
            *returnRef = entry->second.getRef();
        } else {
            *returnRef = Ref::INVALID();
        }
        return entry->second.fetch(xref, recursion);
    }
    *returnRef = Ref::INVALID();
    return Object::null();
}

Object Dict::lookupEnsureEncryptedIfNeeded(std::string_view key) const
{
    const auto *entry = find(key);
    if (!entry) {
        return Object::null();
    }

    if (entry->second.getType() == objRef && xref->isEncrypted() && !xref->isRefEncrypted(entry->second.getRef())) {
        GooString errKey(key);
        error(errSyntaxError, -1, "{0:t} is not encrypted and the document is. This may be a hacking attempt", &errKey);
        return Object::null();
    }

    return entry->second.fetch(xref);
}

const Object &Dict::lookupNF(std::string_view key) const
{
    if (const auto *entry = find(key)) {
        return entry->second;
    }
    static Object nullObj = Object::null();
    return nullObj;
}

bool Dict::lookupInt(std::string_view key, std::optional<std::string_view> alt_key, int *value) const
{
    auto obj1 = lookup(key);
    if (obj1.isNull() && alt_key) {
        obj1 = lookup(*alt_key);
    }
    if (obj1.isInt()) {
        *value = obj1.getInt();
        return true;
    }
    return false;
}

Object Dict::getVal(int i, Ref *returnRef) const
{
    const DictEntry &entry = entries[i];
    if (entry.second.getType() == objRef) {
        *returnRef = entry.second.getRef();
    } else {
        *returnRef = Ref::INVALID();
    }
    return entry.second.fetch(xref);
}

bool Dict::hasKey(std::string_view key) const
{
    return find(key) != nullptr;
}

std::string Dict::findAvailableKey(std::string_view suggestedKey)
{
    int i = 0;
    std::string res(suggestedKey);
    while (find(res)) {
        res = std::string(suggestedKey) + std::to_string(i++);
    }
    return res;
}
