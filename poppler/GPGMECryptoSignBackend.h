//========================================================================
//
// GPGMECryptoSignBackend.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================
#ifndef GPGME_CRYPTO_SIGN_BACKEND_H
#define GPGME_CRYPTO_SIGN_BACKEND_H

#include "CryptoSignBackend.h"

#include <gpgme++/data.h>
#include <gpgme++/context.h>
#include <optional>
#include <future>

#define DUMP_SIGNATURE_DATA 0

class GpgSignatureBackend : public CryptoSign::Backend
{
public:
    GpgSignatureBackend();
    std::unique_ptr<CryptoSign::VerificationInterface> createVerificationHandler(std::vector<unsigned char> &&pkcs7, CryptoSign::SignatureType type) final;
    std::unique_ptr<CryptoSign::SigningInterface> createSigningHandler(const std::string &certID, HashAlgorithm digestAlgTag) final;
    std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates() final;
    static bool hasSufficientVersion();
};

class GpgSignatureCreation : public CryptoSign::SigningInterface
{
public:
    explicit GpgSignatureCreation(const std::string &certId);
    void addData(unsigned char *dataBlock, int dataLen) final;
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const final;
    std::variant<std::vector<unsigned char>, CryptoSign::SigningErrorMessage> signDetached(const std::string &password) final;
    CryptoSign::SignatureType signatureType() const final;

private:
    std::unique_ptr<GpgME::Context> gpgContext;
    GpgME::Data gpgData;
    std::optional<GpgME::Key> key;
    GpgME::Protocol protocol;
};

class GpgSignatureVerification : public CryptoSign::VerificationInterface
{
public:
    explicit GpgSignatureVerification(const std::vector<unsigned char> &p7data, GpgME::Protocol protocol);
    SignatureValidationStatus validateSignature() final;
    void addData(unsigned char *dataBlock, int dataLen) final;
    std::chrono::system_clock::time_point getSigningTime() const final;
    std::string getSignerName() const final;
    std::string getSignerSubjectDN() const final;
    HashAlgorithm getHashAlgorithm() const final;
    CertificateValidationStatus validateCertificateResult() final;
    void validateCertificateAsync(std::chrono::system_clock::time_point validation_time, bool ocspRevocationCheck, bool useAIACertFetch, const std::function<void()> &doneFunction) final;
    std::unique_ptr<X509CertificateInfo> getCertificateInfo() const final;

private:
    std::unique_ptr<GpgME::Context> gpgContext;
    GpgME::Data signatureData;
    GpgME::Data signedData;
    std::optional<GpgME::VerificationResult> gpgResult;
    std::future<CertificateValidationStatus> validationStatus;
    std::optional<CertificateValidationStatus> cachedValidationStatus;
    GpgME::Protocol protocol;
#if DUMP_SIGNATURE_DATA
    std::unique_ptr<std::ofstream> debugSignedData;
#endif
};

#endif // GPGME_CRYPTO_SIGN_BACKEND_H
