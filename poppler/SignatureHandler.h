//========================================================================
//
// SignatureHandler.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015, 2017, 2019, 2021 Albert Astals Cid <aacid@kde.org>
// Copyright 2017 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
// Copyright 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright 2021 Theofilos Intzoglou <int.teo@gmail.com>
// Copyright 2021 Marek Kasik <mkasik@redhat.com>
// Copyright 2023 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#ifndef SIGNATURE_HANDLER_H
#define SIGNATURE_HANDLER_H

#include "goo/GooString.h"
#include "SignatureInfo.h"
#include "CertificateInfo.h"
#include "poppler_private_export.h"
#include "HashAlgorithm.h"

#include <vector>
#include <functional>

/* NSPR Headers */
#include <nspr.h>

/* NSS headers */
#include <cms.h>
#include <nss.h>
#include <cert.h>
#include <cryptohi.h>
#include <secerr.h>
#include <secoid.h>
#include <secmodt.h>
#include <sechash.h>

// experiments seems to say that this is a bit above
// what we have seen in the wild, and much larger than
// what we have managed to get nss and gpgme to create.
static const int maxSupportedSignatureSize = 10000;

class HashContext
{
public:
    explicit HashContext(HashAlgorithm algorithm);
    void updateHash(unsigned char *data_block, int data_len);
    std::vector<unsigned char> endHash();
    HashAlgorithm getHashAlgorithm() const;
    ~HashContext() = default;

private:
    struct HashDestroyer
    {
        void operator()(HASHContext *hash) { HASH_Destroy(hash); }
    };
    std::unique_ptr<HASHContext, HashDestroyer> hash_context;
    HashAlgorithm digest_alg_tag;
};

class POPPLER_PRIVATE_EXPORT SignatureHandler
{
public:
    explicit SignatureHandler(std::vector<unsigned char> &&p7data);
    SignatureHandler(const std::string &certNickName, HashAlgorithm digestAlgTag);
    ~SignatureHandler();
    time_t getSigningTime() const;
    std::string getSignerName() const;
    std::string getSignerSubjectDN() const;
    HashAlgorithm getHashAlgorithm() const;
    void updateHash(unsigned char *data_block, int data_len);
    SignatureValidationStatus validateSignature();
    // Use -1 as validation_time for now
    CertificateValidationStatus validateCertificate(time_t validation_time, bool ocspRevocationCheck, bool useAIACertFetch);
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const;
    static std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates();
    std::unique_ptr<GooString> signDetached(const std::string &password) const;

    // Initializes the NSS dir with the custom given directory
    // calling it with an empty string means use the default firefox db, /etc/pki/nssdb, ~/.pki/nssdb
    // If you don't want a custom NSS dir and the default entries are fine for you, not calling this function is fine
    // If wanted, this has to be called before doing signature validation calls
    static void setNSSDir(const GooString &nssDir);

    // Gets the currently in use NSS dir
    static std::string getNSSDir();

    static void setNSSPasswordCallback(const std::function<char *(const char *)> &f);

private:
    SignatureHandler(const SignatureHandler &);
    SignatureHandler &operator=(const SignatureHandler &);

    static void outputCallback(void *arg, const char *buf, unsigned long len);

    std::vector<unsigned char> p7;
    HashAlgorithm digest_alg_tag;
    SECItem CMSitem;
    std::unique_ptr<HashContext> hashContext;
    NSSCMSMessage *CMSMessage;
    NSSCMSSignedData *CMSSignedData;
    NSSCMSSignerInfo *CMSSignerInfo;
    CERTCertificate *signing_cert;

    static std::string sNssDir;
};

#endif
