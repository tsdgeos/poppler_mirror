//========================================================================
//
// Ref.h
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
// Copyright (C) 2007 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2008 Kees Cook <kees@outflux.net>
// Copyright (C) 2008, 2010, 2017-2021, 2023-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Jakub Wilk <jwilk@jwilk.net>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013, 2017, 2018 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Adrian Perez de Castro <aperez@igalia.com>
// Copyright (C) 2016, 2020 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright (C) 2023 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 Adam Sampson <ats@offog.org>
// Copyright (C) 2026 Stefan Brüns <stefan.bruens@rwth-aachen.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef REF_H
#define REF_H

#include <cassert>
#include <functional>
#include <set>

struct Ref
{
    int num; // object number
    int gen; // generation number

    static constexpr Ref INVALID() { return { .num = -1, .gen = -1 }; };
};

inline bool operator==(const Ref lhs, const Ref rhs) noexcept
{
    return lhs.num == rhs.num && lhs.gen == rhs.gen;
}

inline bool operator!=(const Ref lhs, const Ref rhs) noexcept
{
    return lhs.num != rhs.num || lhs.gen != rhs.gen;
}

inline bool operator<(const Ref lhs, const Ref rhs) noexcept
{
    if (lhs.num != rhs.num) {
        return lhs.num < rhs.num;
    }
    return lhs.gen < rhs.gen;
}

struct RefRecursionChecker
{
    RefRecursionChecker() = default;

    RefRecursionChecker(const RefRecursionChecker &) = delete;
    RefRecursionChecker &operator=(const RefRecursionChecker &) = delete;

    bool insert(Ref ref)
    {
        if (ref == Ref::INVALID()) {
            return true;
        }

        // insert returns std::pair<iterator,bool>
        // where the bool is whether the insert succeeded
        return alreadySeenRefs.insert(ref.num).second;
    }

    void remove(Ref ref) { alreadySeenRefs.erase(ref.num); }

private:
    std::set<int> alreadySeenRefs;
};

struct RefRecursionCheckerRemover
{
    // Removes ref from c when this object is removed
    RefRecursionCheckerRemover(RefRecursionChecker &c, Ref r) : checker(c), ref(r) { }
    ~RefRecursionCheckerRemover() { checker.remove(ref); }

    RefRecursionCheckerRemover(const RefRecursionCheckerRemover &) = delete;
    RefRecursionCheckerRemover &operator=(const RefRecursionCheckerRemover &) = delete;

private:
    RefRecursionChecker &checker;
    Ref ref;
};

namespace std {

template<>
struct hash<Ref>
{
    using argument_type = Ref;
    using result_type = size_t;

    result_type operator()(const argument_type ref) const noexcept { return std::hash<int> {}(ref.num) ^ (std::hash<int> {}(ref.gen) << 1); }
};

}

#endif
