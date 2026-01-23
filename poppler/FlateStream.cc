//========================================================================
//
// FlateStream.cc
//
// Copyright (C) 2005, Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2010, 2021, 2025, Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2016, William Bader <williambader@hotmail.com>
// Copyright (C) 2017, Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// This file is under the GPLv2 or later license
//
//========================================================================

#include <config.h>

#include "poppler-config.h"

#if ENABLE_ZLIB_UNCOMPRESS

#    include "FlateStream.h"

FlateStream::FlateStream(std::unique_ptr<Stream> strA, int predictor, int columns, int colors, int bits) : OwnedFilterStream(std::move(strA))
{
    if (predictor != 1) {
        pred = new StreamPredictor(this, predictor, columns, colors, bits);
        if (!pred->isOk()) {
            delete pred;
            pred = nullptr;
        }
    } else {
        pred = NULL;
    }
    out_pos = 0;
    memset(&d_stream, 0, sizeof(d_stream));
    inflateInit(&d_stream);
}

FlateStream::~FlateStream()
{
    inflateEnd(&d_stream);
    delete pred;
}

bool FlateStream::rewind()
{
    // FIXME: what are the semantics of rewind?
    // i.e. how much initialization has to happen in the constructor?

    /* reinitialize zlib */
    inflateEnd(&d_stream);
    memset(&d_stream, 0, sizeof(d_stream));
    inflateInit(&d_stream);

    str->rewind();
    d_stream.avail_in = 0;
    status = Z_OK;
    out_pos = 0;
    out_buf_len = 0;

    return true;
}

int FlateStream::getRawChar()
{
    return doGetRawChar();
}

void FlateStream::getRawChars(int nChars, int *buffer)
{
    for (int i = 0; i < nChars; ++i)
        buffer[i] = doGetRawChar();
}

int FlateStream::getChar()
{
    if (pred)
        return pred->getChar();
    else
        return getRawChar();
}

int FlateStream::lookChar()
{
    if (pred)
        return pred->lookChar();

    if (fill_buffer())
        return EOF;

    return out_buf[out_pos];
}

int FlateStream::fill_buffer()
{
    /* only fill the buffer if it has all been used */
    if (out_pos >= out_buf_len) {
        /* check if the flatestream has been exhausted */
        if (status == Z_STREAM_END) {
            return -1;
        }

        /* set to the beginning of out_buf */
        d_stream.avail_out = sizeof(out_buf);
        d_stream.next_out = out_buf;
        out_pos = 0;

        while (1) {
            /* buffer is empty so we need to fill it */
            if (d_stream.avail_in == 0) {
                int c;
                /* read from the source stream */
                while (d_stream.avail_in < sizeof(in_buf) && (c = str->getChar()) != EOF) {
                    in_buf[d_stream.avail_in++] = c;
                }
                d_stream.next_in = in_buf;
            }

            /* keep decompressing until we can't anymore */
            if (d_stream.avail_out == 0 || d_stream.avail_in == 0 || (status != Z_OK && status != Z_BUF_ERROR))
                break;
            status = inflate(&d_stream, Z_SYNC_FLUSH);
        }

        out_buf_len = sizeof(out_buf) - d_stream.avail_out;
        if (status != Z_OK && status != Z_STREAM_END)
            return -1;
        if (!out_buf_len)
            return -1;
    }

    return 0;
}

std::optional<std::string> FlateStream::getPSFilter(int psLevel, const char *indent)
{
    if (psLevel < 3 || pred) {
        return std::nullopt;
    }

    std::optional<std::string> s = str->getPSFilter(psLevel, indent);
    if (!s.has_value()) {
        return std::nullopt;
    }
    s->append(indent).append("<< >> /FlateDecode filter\n");

    return s;
}

bool FlateStream::isBinary(bool /*last*/) const
{
    return str->isBinary(true);
}

#endif
