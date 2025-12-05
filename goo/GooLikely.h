//========================================================================
//
// GooLikely.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2008 Kees Cook <kees@outflux.net>
// Copyright (C) 2025 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#ifndef GOOLIKELY_H
#define GOOLIKELY_H

#if defined(__GNUC__) && defined(__OPTIMIZE__)
#    define likely(x) __builtin_expect((x), 1)
#    define unlikely(x) __builtin_expect((x), 0)
#else
#    define likely(x) (x)
#    define unlikely(x) (x)
#endif

#endif
