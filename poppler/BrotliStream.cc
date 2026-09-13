//========================================================================
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

#include "BrotliStream.h"
#include "Stream.h"
#include <brotli/decode.h>

BrotliStream::BrotliStream(std::unique_ptr<Stream> strA) : OwnedFilterStream(std::move(strA))
{
    m_state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
}

BrotliStream::~BrotliStream()
{
    BrotliDecoderDestroyInstance(m_state);
}

bool BrotliStream::rewind()
{
    if (!str) {
        return false;
    }
    if (BrotliDecoderIsUsed(m_state)) {
        // We have used it, we need to recreate
        BrotliDecoderDestroyInstance(m_state);
        m_state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    }
    m_pos = 0;
    m_buffer = {};
    return str->rewind();
}

bool BrotliStream::isBinary(bool last) const
{
    std::ignore = last;
    return str->isBinary(true);
}

int BrotliStream::getChar()
{
    if (!ensureBuffer()) {
        return EOF;
    }
    return m_buffer[m_pos++];
}
int BrotliStream::lookChar()
{
    if (!ensureBuffer()) {
        return EOF;
    }
    return m_buffer[m_pos];
}

bool BrotliStream::ensureBuffer()
{
    static const size_t kKeepBuffer = 1024;
    static const size_t kUseExisting = 2048;
    static const size_t kReadBufferSize = 4096;
    static const size_t kDecompressBufferSize = 8192;
    // We have a data in our buffer, let's just use that
    if (m_pos < kKeepBuffer && m_buffer.size() > kKeepBuffer) {
        return true;
    }
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_pos);
    m_pos = 0;
    if (m_buffer.size() > kUseExisting) {
        return true;
    }

    unsigned char buf[kReadBufferSize];
    size_t got = str->doGetChars(kReadBufferSize, buf);
    if (got == 0) {
        if (BrotliDecoderHasMoreOutput(m_state)) {
            // Continue. We still have data to consume
        } else {
            return !m_buffer.empty();
        }
    }

    const uint8_t *in = buf;

    do {
        size_t size = kDecompressBufferSize;
        uint8_t decompressed[kDecompressBufferSize];
        uint8_t *out = decompressed;
        auto result = BrotliDecoderDecompressStream(m_state, &got, &in, &size, &out, nullptr);
        if (result == BROTLI_DECODER_RESULT_ERROR) {
            error(errSyntaxError, str->getPos(), "Brotli compression error {0:s}", BrotliDecoderErrorString(BrotliDecoderGetErrorCode(m_state)));
            return false;
        }
        auto oldSize = m_buffer.size();
        m_buffer.resize(oldSize + (kDecompressBufferSize - size));
        memcpy(m_buffer.data() + oldSize, decompressed, (kDecompressBufferSize - size));
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            continue;
        }
        break;
    } while (true);
    // We should either have data or have returned from the BROTLI_DECODER_RESULT_ERROR codepath
    // given that m_pos is expected to be 0 here, we can just tell if we have data or not.
    return !m_buffer.empty();
}

int BrotliStream::getChars(int nChar, unsigned char *buffer)
{
    ensureBuffer();
    int amountToCopy = std::min<int>(m_buffer.size() - m_pos, nChar);
    memcpy(buffer, m_buffer.data() + m_pos, amountToCopy);
    m_pos += amountToCopy;
    return amountToCopy;
}
