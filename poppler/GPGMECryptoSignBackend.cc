//========================================================================
//
// GPGMECryptoSignBackend.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2025 Albert Astals Cid <aacid@kde.org>
//========================================================================

#include "CryptoSignBackend.h"
#include "config.h"
#include "GPGMECryptoSignBackend.h"
#include "GPGMECryptoSignBackendConfiguration.h"
#include "DistinguishedNameParser.h"
#include "Error.h"
#include <array>
#include <gpgme.h>
#include <gpgme++/key.h>
#include <gpgme++/gpgmepp_version.h>
#include <gpgme++/signingresult.h>
#include <gpgme++/engineinfo.h>
#if DUMP_SIGNATURE_DATA
#    include <fstream>
#endif
static std::vector<GpgME::Protocol> allowedTypes();

bool GpgSignatureBackend::hasSufficientVersion()
{
    // gpg 2.4.0 does not support padded signatures.
    // Most gpg signatures are padded. This is fixed for 2.4.1
    // gpg 2.4.0 does not support generating signatures
    // with definite lengths. This is also fixed for 2.4.1.
    // this has also been fixed in 2.2.42 in the 2.2 branch
    auto version = GpgME::engineInfo(GpgME::GpgSMEngine).engineVersion();
    if (version > "2.4.0") {
        return true;
    }
    if (version >= "2.3.0") { // development branch for 2.4 releases; no more releases here
        return false;
    }
    return version >= "2.2.42";
}

/// GPGME helper methods

// gpgme++'s string-like functions returns char pointers that can be nullptr
// Creating std::string from nullptr is, depending on c++ standards versions
// either undefined behavior or illegal, so we need a helper.

static std::string fromCharPtr(const char *data)
{
    if (data) {
        return std::string { data };
    }
    return {};
}

static bool isSuccess(const GpgME::Error &err)
{
    if (err) {
        return false;
    }
    if (err.isCanceled()) {
        return false;
    }
    return true;
}

template<typename Result>
static bool isValidResult(const Result &result)
{
    return isSuccess(result.error());
}

static std::string errorString(const GpgME::Error &err)
{
#if GPGMEPP_VERSION < ((1 << 16) | (24 << 8) | (0))
    return fromCharPtr(err.asString());
#else
    return err.asStdString();
#endif
}

template<typename Result>
static bool hasValidResult(const std::optional<Result> &result)
{
    if (!result) {
        return false;
    }
    return isValidResult(result.value());
}

static std::optional<GpgME::Signature> getSignature(const GpgME::VerificationResult &result, size_t signatureNumber)
{
    if (result.numSignatures() > signatureNumber) {
        return result.signature(signatureNumber);
    }
    return std::nullopt;
}

static X509CertificateInfo::Validity getValidityFromSubkey(const GpgME::Subkey &key)
{
    X509CertificateInfo::Validity validity;
    validity.notBefore = key.creationTime();
    if (key.neverExpires()) {
        validity.notAfter = std::numeric_limits<time_t>::max();
    } else {
        validity.notAfter = key.expirationTime();
    }
    return validity;
}

static X509CertificateInfo::EntityInfo getEntityInfoFromKey(std::string_view dnString)
{
    const auto dn = DN::parseString(dnString);
    X509CertificateInfo::EntityInfo info;
    info.commonName = DN::FindFirstValue(dn, "CN").value_or(std::string {});
    info.organization = DN::FindFirstValue(dn, "O").value_or(std::string {});
    info.email = DN::FindFirstValue(dn, "EMAIL").value_or(std::string {});
    info.distinguishedName = std::string { dnString };
    return info;
}

static std::unique_ptr<X509CertificateInfo> getCertificateInfoFromKey(const GpgME::Key &key, GpgME::Protocol protocol)
{
    auto certificateInfo = std::make_unique<X509CertificateInfo>();
    if (protocol == GpgME::CMS) {
        certificateInfo->setIssuerInfo(getEntityInfoFromKey(fromCharPtr(key.issuerName())));
        auto subjectInfo = getEntityInfoFromKey(fromCharPtr(key.userID(0).id()));
        if (subjectInfo.email.empty()) {
            subjectInfo.email = fromCharPtr(key.userID(1).email());
        }
        certificateInfo->setSubjectInfo(std::move(subjectInfo));
    } else if (protocol == GpgME::OpenPGP) {
        X509CertificateInfo::EntityInfo info;
        info.email = fromCharPtr(key.userID(0).email());
        info.commonName = fromCharPtr(key.userID(0).name());
        info.distinguishedName = fromCharPtr(key.userID(0).id());
        certificateInfo->setSubjectInfo(std::move(info));
        certificateInfo->setCertificateType(CertificateType::PGP);
    }
    certificateInfo->setValidity(getValidityFromSubkey(key.subkey(0)));
    certificateInfo->setSerialNumber(GooString { DN::detail::parseHexString(fromCharPtr(key.issuerSerial())).value_or("") });
    certificateInfo->setNickName(GooString(fromCharPtr(key.primaryFingerprint())));
    X509CertificateInfo::PublicKeyInfo pkInfo;
    pkInfo.publicKeyStrength = key.subkey(0).length();
    switch (key.subkey(0).publicKeyAlgorithm()) {
    case GpgME::Subkey::AlgoDSA:
        pkInfo.publicKeyType = DSAKEY;
        break;
    case GpgME::Subkey::AlgoECC:
    case GpgME::Subkey::AlgoECDH:
    case GpgME::Subkey::AlgoECDSA:
    case GpgME::Subkey::AlgoEDDSA:
        pkInfo.publicKeyType = ECKEY;
        break;
    case GpgME::Subkey::AlgoRSA:
    case GpgME::Subkey::AlgoRSA_E:
    case GpgME::Subkey::AlgoRSA_S:
        pkInfo.publicKeyType = RSAKEY;
        break;
    case GpgME::Subkey::AlgoELG:
    case GpgME::Subkey::AlgoELG_E:
    case GpgME::Subkey::AlgoMax:
    case GpgME::Subkey::AlgoUnknown:
#if GPGMEPP_VERSION > ((1 << 16) | (24 << 8) | (0))
    case GpgME::Subkey::AlgoKyber:
#endif
        pkInfo.publicKeyType = OTHERKEY;
    }
    {
        auto ctx = GpgME::Context::create(protocol);
        GpgME::Data pubkeydata;
        const auto err = ctx->exportPublicKeys(key.primaryFingerprint(), pubkeydata);
        if (isSuccess(err)) {
            certificateInfo->setCertificateDER(GooString(pubkeydata.toString()));
        }
    }

    certificateInfo->setPublicKeyInfo(std::move(pkInfo));

    int kue = 0;
    // this block is kind of a hack. GPGSM collapses multiple
    // into one bit, so trying to match it back can never be good
    if (key.canSign()) {
        kue |= KU_NON_REPUDIATION;
        kue |= KU_DIGITAL_SIGNATURE;
    }
    if (key.canEncrypt()) {
        kue |= KU_KEY_ENCIPHERMENT;
        kue |= KU_DATA_ENCIPHERMENT;
    }
    if (key.canCertify()) {
        kue |= KU_KEY_CERT_SIGN;
    }
    certificateInfo->setKeyUsageExtensions(kue);

    auto subkey = key.subkey(0);
    if (subkey.isCardKey()) {
        certificateInfo->setKeyLocation(KeyLocation::HardwareToken);
    } else if (subkey.isSecret()) {
        certificateInfo->setKeyLocation(KeyLocation::Computer);
    }

    certificateInfo->setQualified(subkey.isQualified());

    return certificateInfo;
}

/// implementation of header file

GpgSignatureBackend::GpgSignatureBackend()
{
    GpgME::initializeLibrary();
}

std::unique_ptr<CryptoSign::SigningInterface> GpgSignatureBackend::createSigningHandler(const std::string &certID, HashAlgorithm /*digestAlgTag*/)
{
    return std::make_unique<GpgSignatureCreation>(certID);
}

std::unique_ptr<CryptoSign::VerificationInterface> GpgSignatureBackend::createVerificationHandler(std::vector<unsigned char> &&pkcs7, CryptoSign::SignatureType type)
{
    switch (type) {
    case CryptoSign::SignatureType::unknown_signature_type:
    case CryptoSign::SignatureType::unsigned_signature_field:
        return {};
    case CryptoSign::SignatureType::ETSI_CAdES_detached:
    case CryptoSign::SignatureType::adbe_pkcs7_detached:
    case CryptoSign::SignatureType::adbe_pkcs7_sha1:
        return std::make_unique<GpgSignatureVerification>(std::move(pkcs7), GpgME::CMS);
    case CryptoSign::SignatureType::g10c_pgp_signature_detached:
        return std::make_unique<GpgSignatureVerification>(std::move(pkcs7), GpgME::OpenPGP);
    }
    return {};
}

std::vector<std::unique_ptr<X509CertificateInfo>> GpgSignatureBackend::getAvailableSigningCertificates()
{
    std::vector<std::unique_ptr<X509CertificateInfo>> certificates;
    for (auto type : allowedTypes()) {
        const auto context = GpgME::Context::create(type);
        auto err = context->startKeyListing(static_cast<const char *>(nullptr), true /*secretOnly*/);
        while (isSuccess(err)) {
            const auto key = context->nextKey(err);
            if (!key.isNull() && isSuccess(err)) {
                if (key.isBad()) {
                    continue;
                }
                if (!key.canSign()) {
                    continue;
                }
                certificates.push_back(getCertificateInfoFromKey(key, type));
            } else {
                break;
            }
        }
    }
    return certificates;
}

GpgSignatureCreation::GpgSignatureCreation(const std::string &certId)
{
    for (auto type : allowedTypes()) {
        GpgME::Error error;
        auto context = GpgME::Context::create(type);
        const auto signingKey = context->key(certId.c_str(), error, true);
        if (isSuccess(error)) {
            context->addSigningKey(signingKey);
            key = signingKey;
            gpgContext = std::move(context);
            protocol = type;
            break;
        }
    }
}

void GpgSignatureCreation::addData(unsigned char *dataBlock, int dataLen)
{
    gpgData.write(dataBlock, dataLen);
}
std::variant<std::vector<unsigned char>, CryptoSign::SigningErrorMessage> GpgSignatureCreation::signDetached(const std::string & /*password*/)
{
    if (!key) {
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::KeyMissing, .message = ERROR_IN_CODE_LOCATION };
    }
    gpgData.rewind();
    GpgME::Data signatureData;
    const auto signingResult = gpgContext->sign(gpgData, signatureData, GpgME::SignatureMode::Detached);
    if (!isValidResult(signingResult)) {
        if (signingResult.error().isCanceled()) {
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::UserCancelled, .message = ERROR_IN_CODE_LOCATION };
        }
        switch (signingResult.error().code()) {
        case GPG_ERR_NO_PASSPHRASE: // this is likely the user pressing enter. Let's treat it as cancelled for now
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::UserCancelled, .message = ErrorString { .text = errorString(signingResult.error()), .type = ErrorStringType::UserString } };
        case GPG_ERR_BAD_PASSPHRASE:
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::BadPassphrase, .message = ErrorString { .text = errorString(signingResult.error()), .type = ErrorStringType::UserString } };
        }
        error(errInternal, -1, "Signing error from gpgme: '%s'", errorString(signingResult.error()).c_str());
        return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::GenericError, .message = ErrorString { .text = errorString(signingResult.error()), .type = ErrorStringType::UserString } };
    }

    auto signatureString = signatureData.toString();
    if (protocol == GpgME::OpenPGP) {
        // We need to prepare for padding PGP doesn't like padding as such
        // so we disguise it as a comment packet.
        // the size of the comment packet prefix and size is the first 6 bytes.
        static const int prefixandsize = 6;
        std::string prefix { std::initializer_list<char> {
                (char)0xfd,
                (char)0xff,
        } };

        if (CryptoSign::maxSupportedSignatureSize - prefixandsize <= signatureString.size()) {
            return CryptoSign::SigningErrorMessage { .type = CryptoSign::SigningError::InternalError, .message = ERROR_IN_CODE_LOCATION };
        }
        std::array<unsigned char, 4> bytes;
        int n = CryptoSign::maxSupportedSignatureSize - prefixandsize - signatureString.size();

        bytes[0] = (n >> 24) & 0xFF;
        bytes[1] = (n >> 16) & 0xFF;
        bytes[2] = (n >> 8) & 0xFF;
        bytes[3] = n & 0xFF;

        prefix.append(std::begin(bytes), std::end(bytes));
        signatureString.append(prefix);
    }
    return std::vector<unsigned char>(signatureString.begin(), signatureString.end());
}
CryptoSign::SignatureType GpgSignatureCreation::signatureType() const
{
    if (protocol == GpgME::CMS) {
        return CryptoSign::SignatureType::adbe_pkcs7_detached;
    }
    if (protocol == GpgME::OpenPGP) {
        return CryptoSign::SignatureType::g10c_pgp_signature_detached;
    }
    return CryptoSign::SignatureType::unknown_signature_type;
}

std::unique_ptr<X509CertificateInfo> GpgSignatureCreation::getCertificateInfo() const
{
    if (!key) {
        return nullptr;
    }
    return getCertificateInfoFromKey(*key, protocol);
}

GpgSignatureVerification::GpgSignatureVerification(const std::vector<unsigned char> &p7data, GpgME::Protocol protocol_)
    : gpgContext { GpgME::Context::create(protocol_) }, signatureData(reinterpret_cast<const char *>(p7data.data()), p7data.size()), protocol { protocol_ }
{
    gpgContext->setOffline(true);
    signatureData.setEncoding(GpgME::Data::BinaryEncoding);
#if DUMP_SIGNATURE_DATA
    static int debugFileCounter = 0;
    debugFileCounter++;
    std::ofstream debugSignatureData("/tmp/popplerstuff/signatureData" + std::to_string(debugFileCounter) + ".sig", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    debugSignedData = std::make_unique<std::ofstream>("/tmp/popplerstuff/signedData" + std::to_string(debugFileCounter) + ".data", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    debugSignatureData.write(reinterpret_cast<const char *>(p7data.data()), p7data.size());
#endif
}

void GpgSignatureVerification::addData(unsigned char *dataBlock, int dataLen)
{
    signedData.write(dataBlock, dataLen);
#if DUMP_SIGNATURE_DATA
    debugSignedData->write(reinterpret_cast<char *>(dataBlock), dataLen);
#endif
}

std::unique_ptr<X509CertificateInfo> GpgSignatureVerification::getCertificateInfo() const
{
    if (!hasValidResult(gpgResult)) {
        return nullptr;
    }
    auto signature = getSignature(gpgResult.value(), 0);
    if (!signature) {
        return nullptr;
    }
    auto gpgInfo = getCertificateInfoFromKey(signature->key(true, false), protocol);
    return gpgInfo;
}

HashAlgorithm GpgSignatureVerification::getHashAlgorithm() const
{
    if (gpgResult) {
        const auto signature = getSignature(gpgResult.value(), 0);
        if (!signature) {
            return HashAlgorithm::Unknown;
        }
        switch (signature->hashAlgorithm()) {
        case GPGME_MD_MD5:
            return HashAlgorithm::Md5;
        case GPGME_MD_SHA1:
            return HashAlgorithm::Sha1;
        case GPGME_MD_MD2:
            return HashAlgorithm::Md2;
        case GPGME_MD_SHA256:
            return HashAlgorithm::Sha256;
        case GPGME_MD_SHA384:
            return HashAlgorithm::Sha384;
        case GPGME_MD_SHA512:
            return HashAlgorithm::Sha512;
        case GPGME_MD_SHA224:
            return HashAlgorithm::Sha224;
        case GPGME_MD_NONE:
        case GPGME_MD_RMD160:
        case GPGME_MD_TIGER:
        case GPGME_MD_HAVAL:
        case GPGME_MD_MD4:
        case GPGME_MD_CRC32:
        case GPGME_MD_CRC32_RFC1510:
        case GPGME_MD_CRC24_RFC2440:
        default:
            return HashAlgorithm::Unknown;
        }
    }
    return HashAlgorithm::Unknown;
}

std::string GpgSignatureVerification::getSignerName() const
{
    if (!hasValidResult(gpgResult)) {
        return {};
    }

    const auto signature = getSignature(gpgResult.value(), 0);
    if (!signature) {
        return {};
    }
    if (protocol == GpgME::CMS) {
        const auto dn = DN::parseString(fromCharPtr(signature->key(true, false).userID(0).id()));
        return DN::FindFirstValue(dn, "CN").value_or("");
    }
    if (protocol == GpgME::OpenPGP) {
        return fromCharPtr(signature->key(true, false).userID(0).name());
    }

    return {};
}

std::string GpgSignatureVerification::getSignerSubjectDN() const
{
    if (!hasValidResult(gpgResult)) {
        return {};
    }
    const auto signature = getSignature(gpgResult.value(), 0);
    if (!signature) {
        return {};
    }
    return fromCharPtr(signature->key(true, false).userID(0).id());
}

std::chrono::system_clock::time_point GpgSignatureVerification::getSigningTime() const
{
    if (!hasValidResult(gpgResult)) {
        return {};
    }
    const auto signature = getSignature(gpgResult.value(), 0);
    if (!signature) {
        return {};
    }
    return std::chrono::system_clock::from_time_t(signature->creationTime());
}

void GpgSignatureVerification::validateCertificateAsync(std::chrono::system_clock::time_point /*validation_time*/, bool ocspRevocationCheck, bool useAIACertFetch, const std::function<void()> &doneFunction)
{
    cachedValidationStatus.reset();
    if (!gpgResult) {
        validationStatus = std::async([doneFunction]() {
            if (doneFunction) {
                doneFunction();
            }
            return CERTIFICATE_NOT_VERIFIED;
        });
        return;
    }
    if (gpgResult->error()) {
        validationStatus = std::async([doneFunction]() {
            if (doneFunction) {
                doneFunction();
            }
            return CERTIFICATE_GENERIC_ERROR;
        });
        return;
    }
    const auto signature = getSignature(gpgResult.value(), 0);
    if (!signature) {
        validationStatus = std::async([doneFunction]() {
            if (doneFunction) {
                doneFunction();
            }
            return CERTIFICATE_GENERIC_ERROR;
        });
        return;
    }
    std::string keyFP = fromCharPtr(signature->key().primaryFingerprint());
    validationStatus = std::async([keyFP = std::move(keyFP), protocol = protocol, doneFunction, ocspRevocationCheck, useAIACertFetch]() {
        auto context = GpgME::Context::create(protocol);
        context->setOffline((!ocspRevocationCheck) || useAIACertFetch);
        context->setKeyListMode(GpgME::KeyListMode::Local | GpgME::KeyListMode::Validate);
        GpgME::Error e;
        const auto key = context->key(keyFP.c_str(), e, false);
        if (doneFunction) {
            doneFunction();
        }
        if (e.isCanceled()) {
            return CERTIFICATE_NOT_VERIFIED;
        }
        if (e) {
            return CERTIFICATE_GENERIC_ERROR;
        }
        if (key.isExpired()) {
            return CERTIFICATE_EXPIRED;
        }
        if (key.isRevoked()) {
            return CERTIFICATE_REVOKED;
        }
        if (key.isBad()) {
            return CERTIFICATE_NOT_VERIFIED;
        }
        return CERTIFICATE_TRUSTED;
    });
}

CertificateValidationStatus GpgSignatureVerification::validateCertificateResult()
{
    if (cachedValidationStatus) {
        return cachedValidationStatus.value();
    }
    if (!validationStatus.valid()) {
        return CERTIFICATE_NOT_VERIFIED;
    }
    validationStatus.wait();
    cachedValidationStatus = validationStatus.get();
    return cachedValidationStatus.value();
}

SignatureValidationStatus GpgSignatureVerification::validateSignature()
{
    signedData.rewind();
    const auto result = gpgContext->verifyDetachedSignature(signatureData, signedData);
    gpgResult = result;

    if (!isValidResult(result)) {
        return SIGNATURE_DECODING_ERROR;
    }
    const auto signature = getSignature(result, 0);
    if (!signature) {
        return SIGNATURE_DECODING_ERROR;
    }
    switch (signature->status().code()) {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CERT_EXPIRED: // was valid
    case GPG_ERR_SIG_EXPIRED: // was valid
    case GPG_ERR_CERT_REVOKED: // was valid
        return SIGNATURE_VALID;
    case GPG_ERR_BAD_SIGNATURE:
        return SIGNATURE_INVALID;
    default:
        return SIGNATURE_GENERIC_ERROR;
    }
}

#if ENABLE_PGP_SIGNATURES
bool GpgSignatureConfiguration::allowPgp = true;
#else
bool GpgSignatureConfiguration::allowPgp = false;
#endif
bool GpgSignatureConfiguration::arePgpSignaturesAllowed()
{
    return allowPgp;
}

void GpgSignatureConfiguration::setPgpSignaturesAllowed(bool allow)
{
    allowPgp = allow;
}

static std::vector<GpgME::Protocol> allowedTypes()
{
    std::vector<GpgME::Protocol> allowedTypes;
    allowedTypes.push_back(GpgME::CMS);
    if (GpgSignatureConfiguration::arePgpSignaturesAllowed()) {
        allowedTypes.push_back(GpgME::OpenPGP);
    }
    return allowedTypes;
}
