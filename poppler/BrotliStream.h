//========================================================================
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

#include <Stream.h>

#include "brotli/decode.h"
#include <vector>

class BrotliStream : public OwnedFilterStream
{
public:
    explicit BrotliStream(std::unique_ptr<Stream> strA);
    ~BrotliStream() override;
    StreamKind getKind() const override { return strBrotli; }
    [[nodiscard]] bool rewind() override;
    int getChar() override;
    int lookChar() override;
    bool isBinary(bool last = true) const override;
    bool hasGetChars() override { return true; };

    int getChars(int nChar, unsigned char *buffer) override;

private:
    // ensures there is at least 1 element in the buffer
    // returns false if it fails
    bool ensureBuffer();
    BrotliDecoderState *m_state = nullptr;
    std::vector<uint8_t> m_buffer;
    size_t m_pos = 0;
};
