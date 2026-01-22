//========================================================================
//
// FlateStream.h
//
// Copyright (C) 2005, Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2010, 2011, 2019, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// This file is under the GPLv2 or later license
//
//========================================================================

#ifndef FLATESTREAM_H
#define FLATESTREAM_H

#include <cstdio>

#include "Stream.h"

extern "C" {
#include <zlib.h>
}

class FlateStream : public OwnedFilterStream
{
public:
    FlateStream(std::unique_ptr<Stream> strA, int predictor, int columns, int colors, int bits);
    virtual ~FlateStream();
    StreamKind getKind() const override { return strFlate; }
    [[nodiscard]] bool rewind() override;
    int getChar() override;
    int lookChar() override;
    int getRawChar() override;
    void getRawChars(int nChars, int *buffer) override;
    std::optional<std::string> getPSFilter(int psLevel, const char *indent) override;
    bool isBinary(bool last = true) const override;

private:
    inline int doGetRawChar()
    {
        if (fill_buffer())
            return EOF;

        return out_buf[out_pos++];
    }

    int fill_buffer(void);
    z_stream d_stream;
    StreamPredictor *pred;
    int status;
    /* in_buf currently needs to be 1 or we over read from EmbedStreams */
    unsigned char in_buf[1];
    unsigned char out_buf[4096];
    int out_pos;
    int out_buf_len;
};

#endif
