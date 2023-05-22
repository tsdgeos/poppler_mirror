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
#include <memory>

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
#include "CryptoSignBackend.h"

class HashContext
{
    class private_tag
    {
    };

public:
    HashContext(HashAlgorithm algorithm, private_tag);
    void updateHash(unsigned char *data_block, int data_len);
    std::vector<unsigned char> endHash();
    HashAlgorithm getHashAlgorithm() const;
    ~HashContext() = default;
    static std::unique_ptr<HashContext> create(HashAlgorithm algorithm);

private:
    struct HashDestroyer
    {
        void operator()(HASHContext *hash) { HASH_Destroy(hash); }
    };
    std::unique_ptr<HASHContext, HashDestroyer> hash_context;
    HashAlgorithm digest_alg_tag;
};

class POPPLER_PRIVATE_EXPORT SignatureVerificationHandler final : public CryptoSign::VerificationInterface
{
public:
    explicit SignatureVerificationHandler(std::vector<unsigned char> &&p7data);
    ~SignatureVerificationHandler() final;
    SignatureValidationStatus validateSignature() final;
    std::chrono::system_clock::time_point getSigningTime() const final;
    std::string getSignerName() const final;
    std::string getSignerSubjectDN() const final;
    // Use -1 as validation_time for now
    CertificateValidationStatus validateCertificate(std::chrono::system_clock::time_point validation_time, bool ocspRevocationCheck, bool useAIACertFetch) final;
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const final;
    void addData(unsigned char *data_block, int data_len) final;
    HashAlgorithm getHashAlgorithm() const final;

    SignatureVerificationHandler(const SignatureVerificationHandler &) = delete;
    SignatureVerificationHandler &operator=(const SignatureVerificationHandler &) = delete;

private:
    std::vector<unsigned char> p7;
    NSSCMSMessage *CMSMessage;
    NSSCMSSignedData *CMSSignedData;
    NSSCMSSignerInfo *CMSSignerInfo;
    SECItem CMSitem;
    std::unique_ptr<HashContext> hashContext;
};

class POPPLER_PRIVATE_EXPORT SignatureSignHandler final : public CryptoSign::SigningInterface
{
public:
    SignatureSignHandler(const std::string &certNickname, HashAlgorithm digestAlgTag);
    ~SignatureSignHandler() final;
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const final;
    void addData(unsigned char *data_block, int data_len) final;
    std::optional<GooString> signDetached(const std::string &password) final;

    SignatureSignHandler(const SignatureSignHandler &) = delete;
    SignatureSignHandler &operator=(const SignatureSignHandler &) = delete;

private:
    std::unique_ptr<HashContext> hashContext;
    CERTCertificate *signing_cert;
};

class POPPLER_PRIVATE_EXPORT SignatureHandler
{
public:
    static std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates();

    // Initializes the NSS dir with the custom given directory
    // calling it with an empty string means use the default firefox db, /etc/pki/nssdb, ~/.pki/nssdb
    // If you don't want a custom NSS dir and the default entries are fine for you, not calling this function is fine
    // If wanted, this has to be called before doing signature validation calls
    static void setNSSDir(const GooString &nssDir);

    // Gets the currently in use NSS dir
    static std::string getNSSDir();

    static void setNSSPasswordCallback(const std::function<char *(const char *)> &f);

    SignatureHandler() = delete;

private:
    static std::string sNssDir;
};

class NSSCryptoSignBackend final : public CryptoSign::Backend
{
public:
    std::unique_ptr<CryptoSign::VerificationInterface> createVerificationHandler(std::vector<unsigned char> &&pkcs7) final { return std::make_unique<SignatureVerificationHandler>(std::move(pkcs7)); }
    std::unique_ptr<CryptoSign::SigningInterface> createSigningHandler(const std::string &certID, HashAlgorithm digestAlgTag) final { return std::make_unique<SignatureSignHandler>(certID, digestAlgTag); }
    std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates() final { return SignatureHandler::getAvailableSigningCertificates(); }
    ~NSSCryptoSignBackend() final;
};

#endif
