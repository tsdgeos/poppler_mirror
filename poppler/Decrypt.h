//========================================================================
//
// Decrypt.h
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
// Copyright (C) 2008 Julien Rebetez <julien@fhtagn.net>
// Copyright (C) 2009 David Benjamin <davidben@mit.edu>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013, 2018, 2019, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef DECRYPT_H
#define DECRYPT_H

#include "goo/GooString.h"
#include "Object.h"
#include "Stream.h"

//------------------------------------------------------------------------
// Decrypt
//------------------------------------------------------------------------

class Decrypt
{
public:
    // Generate a file key.  The <fileKey> buffer must have space for at
    // least 16 bytes.  Checks <ownerPassword> and then <userPassword>
    // and returns true if either is correct.  Sets <ownerPasswordOk> if
    // the owner password was correct.  Either or both of the passwords
    // may be NULL, which is treated as an empty string.
    static bool makeFileKey(int encRevision, int keyLength, const GooString *ownerKey, const GooString *userKey, const GooString *ownerEnc, const GooString *userEnc, int permissions, const GooString *fileID, const GooString *ownerPassword,
                            const GooString *userPassword, unsigned char *fileKey, bool encryptMetadata, bool *ownerPasswordOk);

private:
    static bool makeFileKey2(int encRevision, int keyLength, const GooString *ownerKey, const GooString *userKey, int permissions, const GooString *fileID, const GooString *userPassword, unsigned char *fileKey, bool encryptMetadata);
};

//------------------------------------------------------------------------
// Helper classes
//------------------------------------------------------------------------

/* DecryptRC4State, DecryptAESState, DecryptAES256State are named like this for
 * historical reasons, but they're used for encryption too.
 * In case of decryption, the cbc field in AES and AES-256 contains the previous
 * input block or the CBC initialization vector (IV) if the stream has just been
 * reset). In case of encryption, it always contains the IV, whereas the
 * previous output is kept in buf. The paddingReached field is only used in
 * case of encryption. */
struct DecryptRC4State
{
    unsigned char state[256];
    unsigned char x, y;
};

struct DecryptAESState
{
    unsigned int w[44];
    unsigned char state[16];
    unsigned char cbc[16];
    unsigned char buf[16];
    bool paddingReached; // encryption only
    int bufIdx;
};

struct DecryptAES256State
{
    unsigned int w[60];
    unsigned char state[16];
    unsigned char cbc[16];
    unsigned char buf[16];
    bool paddingReached; // encryption only
    int bufIdx;
};

class BaseCryptStream : public FilterStream
{
public:
    BaseCryptStream(std::unique_ptr<Stream> strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    BaseCryptStream(Stream &strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    ~BaseCryptStream() override;
    StreamKind getKind() const override { return strCrypt; }
    [[nodiscard]] bool rewind() override;
    int getChar() override;
    int lookChar() override = 0;
    Goffset getPos() override;
    bool isBinary(bool last) const override;
    Stream *getUndecodedStream() override { return this; }

protected:
    CryptAlgorithm algo;
    int objKeyLength;
    unsigned char objKey[32];
    Goffset charactersRead; // so that getPos() can be correct
    int nextCharBuff; // EOF means not read yet
    std::unique_ptr<Stream> ownedStream;

    union {
        DecryptRC4State rc4;
        DecryptAESState aes;
        DecryptAES256State aes256;
    } state;
};

//------------------------------------------------------------------------
// EncryptStream / DecryptStream
//------------------------------------------------------------------------

class EncryptStream : public BaseCryptStream
{
public:
    EncryptStream(Stream &strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    EncryptStream(std::unique_ptr<Stream> strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    ~EncryptStream() override;
    [[nodiscard]] bool rewind() override;
    int lookChar() override;

private:
    void init();
};

class DecryptStream : public BaseCryptStream
{
public:
    DecryptStream(std::unique_ptr<Stream> strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    DecryptStream(Stream &strA, const unsigned char *fileKey, CryptAlgorithm algoA, int keyLength, Ref ref);
    ~DecryptStream() override;
    [[nodiscard]] bool rewind() override;
    int lookChar() override;
};

//------------------------------------------------------------------------

extern void md5(const unsigned char *msg, int msgLen, unsigned char *digest);

#endif
