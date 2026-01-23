//========================================================================
//
// SecurityHandler.cc
//
// Copyright 2004 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2010, 2012, 2015, 2017, 2018, 2020-2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2014 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2016 Alok Anand <alok4nand@gmail.com>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include "goo/GooString.h"
#include "PDFDoc.h"
#include "Decrypt.h"
#include "Error.h"
#include "GlobalParams.h"
#include "SecurityHandler.h"

#include <climits>

//------------------------------------------------------------------------
// SecurityHandler
//------------------------------------------------------------------------

SecurityHandler *SecurityHandler::make(PDFDoc *docA, Object *encryptDictA)
{
    SecurityHandler *secHdlr;

    Object filterObj = encryptDictA->dictLookup("Filter");
    if (filterObj.isName("Standard")) {
        secHdlr = new StandardSecurityHandler(docA, encryptDictA);
    } else if (filterObj.isName()) {
        error(errSyntaxError, -1, "Couldn't find the '{0:s}' security handler", filterObj.getName());
        secHdlr = nullptr;
    } else {
        error(errSyntaxError, -1, "Missing or invalid 'Filter' entry in encryption dictionary");
        secHdlr = nullptr;
    }
    return secHdlr;
}

SecurityHandler::SecurityHandler(PDFDoc *docA)
{
    doc = docA;
}

SecurityHandler::~SecurityHandler() = default;

bool SecurityHandler::checkEncryption(const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    void *authData;

    if (ownerPassword || userPassword) {
        authData = makeAuthData(ownerPassword, userPassword);
    } else {
        authData = nullptr;
    }
    const bool ok = authorize(authData);
    if (authData) {
        freeAuthData(authData);
    }
    if (!ok) {
        if (!ownerPassword && !userPassword) {
            return checkEncryption(GooString(), GooString());
        }
        error(errCommandLine, -1, "Incorrect password");
    }
    return ok;
}

//------------------------------------------------------------------------
// StandardSecurityHandler
//------------------------------------------------------------------------

class StandardAuthData
{
public:
    StandardAuthData(std::unique_ptr<GooString> &&ownerPasswordA, std::unique_ptr<GooString> &&userPasswordA) : ownerPassword(std::move(ownerPasswordA)), userPassword(std::move(userPasswordA)) { }

    ~StandardAuthData() = default;

    StandardAuthData(const StandardAuthData &) = delete;
    StandardAuthData &operator=(const StandardAuthData &) = delete;

    const std::unique_ptr<GooString> ownerPassword;
    const std::unique_ptr<GooString> userPassword;
};

StandardSecurityHandler::StandardSecurityHandler(PDFDoc *docA, Object *encryptDictA) : SecurityHandler(docA)
{
    ok = false;
    fileKeyLength = 0;
    encAlgorithm = cryptNone;

    Object versionObj = encryptDictA->dictLookup("V");
    Object revisionObj = encryptDictA->dictLookup("R");
    Object lengthObj = encryptDictA->dictLookup("Length");
    Object ownerKeyObj = encryptDictA->dictLookup("O");
    Object userKeyObj = encryptDictA->dictLookup("U");
    Object ownerEncObj = encryptDictA->dictLookup("OE");
    Object userEncObj = encryptDictA->dictLookup("UE");
    Object permObj = encryptDictA->dictLookup("P");
    if (permObj.isInt64()) {
        auto permUint = static_cast<unsigned int>(permObj.getInt64());
        int perms = permUint - UINT_MAX - 1;
        permObj = Object(perms);
    }
    Object fileIDObj = doc->getXRef()->getTrailerDict()->dictLookup("ID");
    if (versionObj.isInt() && revisionObj.isInt() && permObj.isInt() && ownerKeyObj.isString() && userKeyObj.isString()) {
        encVersion = versionObj.getInt();
        encRevision = revisionObj.getInt();
        if ((encRevision <= 4 && !ownerKeyObj.getString()->empty() && !userKeyObj.getString()->empty())
            || ((encRevision == 5 || encRevision == 6) &&
                // the spec says 48 bytes, but Acrobat pads them out longer
                ownerKeyObj.getString()->size() >= 48 && userKeyObj.getString()->size() >= 48 && ownerEncObj.isString() && ownerEncObj.getString()->size() == 32 && userEncObj.isString() && userEncObj.getString()->size() == 32)) {
            encAlgorithm = cryptRC4;
            // revision 2 forces a 40-bit key - some buggy PDF generators
            // set the Length value incorrectly
            if (encRevision == 2 || !lengthObj.isInt()) {
                fileKeyLength = 5;
            } else {
                fileKeyLength = lengthObj.getInt() / 8;
            }
            encryptMetadata = true;
            //~ this currently only handles a subset of crypt filter functionality
            //~ (in particular, it ignores the EFF entry in encryptDictA, and
            //~ doesn't handle the case where StmF, StrF, and EFF are not all the
            //~ same)
            if ((encVersion == 4 || encVersion == 5) && (encRevision == 4 || encRevision == 5 || encRevision == 6)) {
                Object cryptFiltersObj = encryptDictA->dictLookup("CF");
                Object streamFilterObj = encryptDictA->dictLookup("StmF");
                Object stringFilterObj = encryptDictA->dictLookup("StrF");
                if (cryptFiltersObj.isDict() && streamFilterObj.isName() && stringFilterObj.isName() && !strcmp(streamFilterObj.getName(), stringFilterObj.getName())) {
                    if (!strcmp(streamFilterObj.getName(), "Identity")) {
                        // no encryption on streams or strings
                        encVersion = encRevision = -1;
                    } else {
                        Object cryptFilterObj = cryptFiltersObj.dictLookup(streamFilterObj.getName());
                        if (cryptFilterObj.isDict()) {
                            Object cfmObj = cryptFilterObj.dictLookup("CFM");
                            if (cfmObj.isName("V2")) {
                                encVersion = 2;
                                encRevision = 3;
                                Object cfLengthObj = cryptFilterObj.dictLookup("Length");
                                if (cfLengthObj.isInt()) {
                                    //~ according to the spec, this should be cfLengthObj / 8
                                    fileKeyLength = cfLengthObj.getInt();
                                }
                            } else if (cfmObj.isName("AESV2")) {
                                encVersion = 2;
                                encRevision = 3;
                                encAlgorithm = cryptAES;
                                Object cfLengthObj = cryptFilterObj.dictLookup("Length");
                                if (cfLengthObj.isInt()) {
                                    //~ according to the spec, this should be cfLengthObj / 8
                                    fileKeyLength = cfLengthObj.getInt();
                                }
                            } else if (cfmObj.isName("AESV3")) {
                                encVersion = 5;
                                // let encRevision be 5 or 6
                                encAlgorithm = cryptAES256;
                                Object cfLengthObj = cryptFilterObj.dictLookup("Length");
                                if (cfLengthObj.isInt()) {
                                    //~ according to the spec, this should be cfLengthObj / 8
                                    fileKeyLength = cfLengthObj.getInt();
                                }
                            }
                        }
                    }
                }
                Object encryptMetadataObj = encryptDictA->dictLookup("EncryptMetadata");
                if (encryptMetadataObj.isBool()) {
                    encryptMetadata = encryptMetadataObj.getBool();
                }
            }
            permFlags = permObj.getInt();
            ownerKey = ownerKeyObj.getString()->copy();
            userKey = userKeyObj.getString()->copy();
            if (encVersion >= 1 && encVersion <= 2 && encRevision >= 2 && encRevision <= 3) {
                if (fileIDObj.isArray()) {
                    Object fileIDObj1 = fileIDObj.arrayGet(0);
                    if (fileIDObj1.isString()) {
                        fileID = fileIDObj1.getString()->copy();
                    } else {
                        fileID = std::make_unique<GooString>();
                    }
                } else {
                    fileID = std::make_unique<GooString>();
                }
                if (fileKeyLength > 16 || fileKeyLength < 0) {
                    fileKeyLength = 16;
                }
                ok = true;
            } else if (encVersion == 5 && (encRevision == 5 || encRevision == 6)) {
                fileID = std::make_unique<GooString>(); // unused for V=R=5
                if (ownerEncObj.isString() && userEncObj.isString()) {
                    ownerEnc = ownerEncObj.getString()->copy();
                    userEnc = userEncObj.getString()->copy();
                    if (fileKeyLength > 32 || fileKeyLength < 0) {
                        fileKeyLength = 32;
                    }
                    ok = true;
                } else {
                    error(errSyntaxError, -1, "Weird encryption owner/user info");
                }
            } else if (encVersion != -1 || encRevision != -1) {
                error(errUnimplemented, -1, "Unsupported version/revision ({0:d}/{1:d}) of Standard security handler", encVersion, encRevision);
            }

            if (encRevision <= 4) {
                // Adobe apparently zero-pads the U value (and maybe the O value?)
                // if it's short
                while (ownerKey->size() < 32) {
                    ownerKey->push_back((char)0x00);
                }
                while (userKey->size() < 32) {
                    userKey->push_back((char)0x00);
                }
            }
        } else {
            error(errSyntaxError, -1,
                  "Invalid encryption key length. version: {0:d} - revision: {1:d} - ownerKeyLength: {2:uld} - userKeyLength: {3:uld} - ownerEncIsString: {4:d} - ownerEncLength: {5:uld} - userEncIsString: {6:d} - userEncLength: {7:uld}",
                  encVersion, encRevision, ownerKeyObj.getString()->size(), userKeyObj.getString()->size(), ownerEncObj.isString(), ownerEncObj.isString() ? ownerEncObj.getString()->size() : -1, userEncObj.isString(),
                  userEncObj.isString() ? userEncObj.getString()->size() : -1);
        }
    } else {
        error(errSyntaxError, -1, "Weird encryption info");
    }
}

StandardSecurityHandler::~StandardSecurityHandler() = default;

bool StandardSecurityHandler::isUnencrypted() const
{
    if (!ok) {
        return true;
    }
    return encVersion == -1 && encRevision == -1;
}

void *StandardSecurityHandler::makeAuthData(const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    return new StandardAuthData(ownerPassword ? ownerPassword->copy() : std::make_unique<GooString>(), userPassword ? userPassword->copy() : std::make_unique<GooString>());
}

void StandardSecurityHandler::freeAuthData(void *authData)
{
    delete (StandardAuthData *)authData;
}

bool StandardSecurityHandler::authorize(void *authData)
{
    GooString *ownerPassword, *userPassword;

    if (!ok) {
        return false;
    }
    if (authData) {
        ownerPassword = ((StandardAuthData *)authData)->ownerPassword.get();
        userPassword = ((StandardAuthData *)authData)->userPassword.get();
    } else {
        ownerPassword = nullptr;
        userPassword = nullptr;
    }
    if (!Decrypt::makeFileKey(encRevision, fileKeyLength, ownerKey.get(), userKey.get(), ownerEnc.get(), userEnc.get(), permFlags, fileID.get(), ownerPassword, userPassword, fileKey, encryptMetadata, &ownerPasswordOk)) {
        return false;
    }
    return true;
}
