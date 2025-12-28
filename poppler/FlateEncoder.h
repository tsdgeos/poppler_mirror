//========================================================================
//
// FlateEncoder.h
//
// Copyright (C) 2016, William Bader <williambader@hotmail.com>
// Copyright (C) 2018, 2019, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// This file is under the GPLv2 or later license
//
//========================================================================

#ifndef FLATEENCODE_H
#define FLATEENCODE_H

#include <cstdio>
#include "Object.h"

#include "Stream.h"

extern "C" {
#include <zlib.h>
}

//------------------------------------------------------------------------
// FlateEncoder
//------------------------------------------------------------------------

class FlateEncoder : public FilterStream
{
public:
    explicit FlateEncoder(Stream *strA);
    ~FlateEncoder() override;
    StreamKind getKind() const override { return strWeird; }
    [[nodiscard]] bool rewind() override;
    int getChar() override { return (outBufPtr >= outBufEnd && !fillBuf()) ? EOF : (*outBufPtr++ & 0xff); }
    int lookChar() override { return (outBufPtr >= outBufEnd && !fillBuf()) ? EOF : (*outBufPtr & 0xff); }
    std::optional<std::string> getPSFilter(int /*psLevel*/, const char * /*indent*/) override { return {}; }
    bool isBinary(bool /*last*/ = true) const override { return true; }
    bool isEncoder() const override { return true; }

private:
    static const int inBufSize = 16384;
    static const int outBufSize = inBufSize;
    unsigned char inBuf[inBufSize];
    unsigned char outBuf[outBufSize];
    unsigned char *outBufPtr;
    unsigned char *outBufEnd;
    bool inBufEof;
    bool outBufEof;
    z_stream zlib_stream;

    bool fillBuf();
};

#endif
