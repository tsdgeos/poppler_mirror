//========================================================================
//
// CachedFile.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2009 Stefan Thomas <thomas@eload24.com>
// Copyright 2010, 2011 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2018-2020, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013 Julien Nabet <serval2412@yahoo.fr>
//
//========================================================================

#include <config.h>
#include "CachedFile.h"

//------------------------------------------------------------------------
// CachedFile
//------------------------------------------------------------------------

CachedFile::CachedFile(CachedFileLoader *cacheLoader)
{
    loader = cacheLoader;

    streamPos = 0;
    chunks = new std::vector<Chunk>();
    length = 0;

    length = loader->init(this);
    refCnt = 1;

    if (length != ((size_t)-1)) {
        chunks->resize(length / CachedFileChunkSize + 1);
    } else {
        error(errInternal, -1, "Failed to initialize file cache.");
        chunks->resize(0);
    }
}

CachedFile::~CachedFile()
{
    delete loader;
    delete chunks;
}

void CachedFile::incRefCnt()
{
    refCnt++;
}

void CachedFile::decRefCnt()
{
    if (--refCnt == 0) {
        delete this;
    }
}

long int CachedFile::tell()
{
    return streamPos;
}

int CachedFile::seek(long int offset, int origin)
{
    if (origin == SEEK_SET) {
        streamPos = offset;
    } else if (origin == SEEK_CUR) {
        streamPos += offset;
    } else {
        streamPos = length + offset;
    }

    if (streamPos > length) {
        streamPos = 0;
        return 1;
    }

    return 0;
}

int CachedFile::cache(const std::vector<ByteRange> &origRanges)
{
    std::vector<int> loadChunks;
    int numChunks = length / CachedFileChunkSize + 1;
    std::vector<bool> chunkNeeded(numChunks);
    int startChunk, endChunk;
    std::vector<ByteRange> chunk_ranges, all;
    ByteRange range;
    const std::vector<ByteRange> *ranges = &origRanges;

    if (ranges->empty()) {
        range.offset = 0;
        range.length = length;
        all.push_back(range);
        ranges = &all;
    }

    for (int i = 0; i < numChunks; ++i) {
        chunkNeeded[i] = false;
    }
    for (const ByteRange &r : *ranges) {

        if (r.length == 0) {
            continue;
        }
        if (r.offset >= length) {
            continue;
        }

        const size_t start = r.offset;
        size_t end = start + r.length - 1;
        if (end >= length) {
            end = length - 1;
        }

        startChunk = start / CachedFileChunkSize;
        endChunk = end / CachedFileChunkSize;
        for (int chunk = startChunk; chunk <= endChunk; chunk++) {
            if ((*chunks)[chunk].state == chunkStateNew) {
                chunkNeeded[chunk] = true;
            }
        }
    }

    int chunk = 0;
    while (chunk < numChunks) {
        while (chunk < numChunks && !chunkNeeded[chunk]) {
            chunk++;
        }
        if (chunk == numChunks) {
            break;
        }
        startChunk = chunk;
        loadChunks.push_back(chunk);

        chunk++;
        while (chunk < numChunks && chunkNeeded[chunk]) {
            loadChunks.push_back(chunk);
            chunk++;
        }
        endChunk = chunk - 1;

        range.offset = startChunk * CachedFileChunkSize;
        range.length = (endChunk - startChunk + 1) * CachedFileChunkSize;

        chunk_ranges.push_back(range);
    }

    if (chunk_ranges.empty()) {
        return 0;
    }

    CachedFileWriter writer = CachedFileWriter(this, &loadChunks);
    return loader->load(chunk_ranges, &writer);
}

size_t CachedFile::read(void *ptr, size_t unitsize, size_t count)
{
    size_t bytes = unitsize * count;
    if (length < (streamPos + bytes)) {
        bytes = length - streamPos;
    }

    if (bytes == 0) {
        return 0;
    }

    // Load data
    if (cache(streamPos, bytes) != 0) {
        return 0;
    }

    // Copy data to buffer
    size_t toCopy = bytes;
    while (toCopy) {
        int chunk = streamPos / CachedFileChunkSize;
        int offset = streamPos % CachedFileChunkSize;
        size_t len = CachedFileChunkSize - offset;

        if (len > toCopy) {
            len = toCopy;
        }

        memcpy(ptr, (*chunks)[chunk].data + offset, len);
        streamPos += len;
        toCopy -= len;
        ptr = (char *)ptr + len;
    }

    return bytes;
}

int CachedFile::cache(size_t rangeOffset, size_t rangeLength)
{
    std::vector<ByteRange> r;
    ByteRange range;
    range.offset = rangeOffset;
    range.length = rangeLength;
    r.push_back(range);
    return cache(r);
}

//------------------------------------------------------------------------
// CachedFileWriter
//------------------------------------------------------------------------

CachedFileWriter::CachedFileWriter(CachedFile *cachedFileA, std::vector<int> *chunksA)
{
    cachedFile = cachedFileA;
    chunks = chunksA;

    if (chunks) {
        offset = 0;
        it = (*chunks).begin();
    }
}

size_t CachedFileWriter::write(const char *ptr, size_t size)
{
    const char *cp = ptr;
    size_t len = size;
    size_t nfree, ncopy;
    size_t written = 0;
    size_t chunk;

    if (!len) {
        return 0;
    }

    while (len) {
        if (chunks) {
            if (offset == CachedFileChunkSize) {
                ++it;
                if (it == (*chunks).end()) {
                    return written;
                }
                offset = 0;
            }
            chunk = *it;
        } else {
            offset = cachedFile->length % CachedFileChunkSize;
            chunk = cachedFile->length / CachedFileChunkSize;
        }

        if (chunk >= cachedFile->chunks->size()) {
            cachedFile->chunks->resize(chunk + 1);
        }

        nfree = CachedFileChunkSize - offset;
        ncopy = (len >= nfree) ? nfree : len;
        memcpy(&((*cachedFile->chunks)[chunk].data[offset]), cp, ncopy);
        len -= ncopy;
        cp += ncopy;
        offset += ncopy;
        written += ncopy;

        if (!chunks) {
            cachedFile->length += ncopy;
        }

        if (offset == CachedFileChunkSize) {
            (*cachedFile->chunks)[chunk].state = CachedFile::chunkStateLoaded;
        }
    }

    if ((chunk == (cachedFile->length / CachedFileChunkSize)) && (offset == (cachedFile->length % CachedFileChunkSize))) {
        (*cachedFile->chunks)[chunk].state = CachedFile::chunkStateLoaded;
    }

    return written;
}

CachedFileLoader::~CachedFileLoader() = default;

//------------------------------------------------------------------------
