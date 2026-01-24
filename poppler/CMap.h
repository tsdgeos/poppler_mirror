//========================================================================
//
// CMap.h
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
// Copyright (C) 2009, 2018-2020, 2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2012, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef CMAP_H
#define CMAP_H

#include <array>
#include <memory>

#include "Object.h"
#include "CharTypes.h"
#include "GfxFont.h"

class GooString;
class Object;
struct CMapVectorEntry;
class CMapCache;
class Stream;

//------------------------------------------------------------------------

class CMap
{
public:
    // Parse a CMap from <obj>, which can be a name or a stream.  Sets
    // the initial reference count to 1.  Returns NULL on failure.
    static std::shared_ptr<CMap> parse(const std::string &collectionA, Object *obj);

    // Create the CMap specified by <collection> and <cMapName>.  Sets
    // the initial reference count to 1.  Returns NULL on failure.
    static std::shared_ptr<CMap> parse(CMapCache *cache, const std::string &collectionA, const std::string &cMapNameA);

    ~CMap();

    CMap(const CMap &) = delete;
    CMap &operator=(const CMap &) = delete;

    // Return collection name (<registry>-<ordering>).
    const GooString *getCollection() const { return collection.get(); }

    const GooString *getCMapName() const { return cMapName.get(); }

    // Return true if this CMap matches the specified <collectionA>, and
    // <cMapNameA>.
    bool match(const std::string &collectionA, const std::string &cMapNameA);

    // Return the CID corresponding to the character code starting at
    // <s>, which contains <len> bytes.  Sets *<c> to the char code, and
    // *<nUsed> to the number of bytes used by the char code.
    CID getCID(const char *s, int len, CharCode *c, int *nUsed);

    // Return the writing mode
    GfxFont::WritingMode getWMode() const { return wMode; }

    void setReverseMap(unsigned int *rmap, unsigned int rmapSize, unsigned int ncand);

private:
    static std::shared_ptr<CMap> parse(const std::string &collectionA, Object *obj, RefRecursionChecker &recursion);
    static std::shared_ptr<CMap> parse(CMapCache *cache, const std::string &collectionA, Stream *str, RefRecursionChecker &recursion);

    void parse2(CMapCache *cache, int (*getCharFunc)(void *), void *data);
    CMap(std::unique_ptr<GooString> &&collectionA, std::unique_ptr<GooString> &&cMapNameA);
    CMap(std::unique_ptr<GooString> &&collectionA, std::unique_ptr<GooString> &&cMapNameA, GfxFont::WritingMode wModeA);
    void useCMap(CMapCache *cache, const char *useName);
    void useCMap(Object *obj, RefRecursionChecker &recursion);
    void copyVector(CMapVectorEntry *dest, CMapVectorEntry *src);
    void addCIDs(unsigned int start, unsigned int end, unsigned int nBytes, CID firstCID);
    void freeCMapVector(CMapVectorEntry *vec);
    void setReverseMapVector(unsigned int startCode, CMapVectorEntry *vec, unsigned int *rmap, unsigned int rmapSize, unsigned int ncand);

    const std::unique_ptr<GooString> collection;
    const std::unique_ptr<GooString> cMapName;
    bool isIdent; // true if this CMap is an identity mapping,
                  //   or is based on one (via usecmap)
    GfxFont::WritingMode wMode;
    CMapVectorEntry *vector; // vector for first byte (NULL for
                             //   identity CMap)
};

//------------------------------------------------------------------------

#define cMapCacheSize 4

class CMapCache
{
public:
    CMapCache();
    ~CMapCache() = default;

    CMapCache(const CMapCache &) = delete;
    CMapCache &operator=(const CMapCache &) = delete;

    // Get the <cMapName> CMap for the specified character collection.
    // Increments its reference count; there will be one reference for
    // the cache plus one for the caller of this function.
    // Stream is a stream containing the CMap, can be NULL and
    // this means the CMap will be searched in the CMap files
    // Returns NULL on failure.
    std::shared_ptr<CMap> getCMap(const std::string &collection, const std::string &cMapName);

private:
    std::array<std::shared_ptr<CMap>, cMapCacheSize> cache;
};

#endif
