//========================================================================
//
// StdinCachedFile.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#ifndef STDINCACHELOADER_H
#define STDINCACHELOADER_H

#include "CachedFile.h"

#include <cstdio>

class POPPLER_PRIVATE_EXPORT StdinCacheLoader : public CachedFileLoader
{
    FILE *file = stdin;

public:
    StdinCacheLoader() = default;
    ~StdinCacheLoader() override;

    explicit StdinCacheLoader(FILE *fileA) : file(fileA) { }

    size_t init(GooString *dummy, CachedFile *cachedFile) override;
    int load(const std::vector<ByteRange> &ranges, CachedFileWriter *writer) override;
};

#endif
