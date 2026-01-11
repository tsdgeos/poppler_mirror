//========================================================================
//
// CryptoSignBackend.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023, 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2024, 2025 Albert Astals Cid <aacid@kde.org>
//========================================================================
#include "CryptoSignBackend.h"
#include "config.h"
#if ENABLE_GPGME
#    include "GPGMECryptoSignBackend.h"
#endif
#if ENABLE_NSS3
#    include "NSSCryptoSignBackend.h"
#endif

namespace CryptoSign {

SignatureType signatureTypeFromString(std::string_view data)
{
    if (data == std::string_view("ETSI.CAdES.detached")) {
        return CryptoSign::SignatureType::ETSI_CAdES_detached;
    }
    if (data == std::string_view("adbe.pkcs7.detached")) {
        return CryptoSign::SignatureType::adbe_pkcs7_detached;
    }
    if (data == std::string_view("adbe.pkcs7.sha1")) {
        return CryptoSign::SignatureType::adbe_pkcs7_sha1;
    }
    if (data == std::string_view("g10c.pgp.signature.detached")) {
        return CryptoSign::SignatureType::g10c_pgp_signature_detached;
    }
    return CryptoSign::SignatureType::unknown_signature_type;
}

std::string toStdString(SignatureType type)
{
    switch (type) {
    case CryptoSign::SignatureType::unsigned_signature_field:
        return "Unsigned";
    case CryptoSign::SignatureType::unknown_signature_type:
        return "Unknown";
    case CryptoSign::SignatureType::ETSI_CAdES_detached:
        return "ETSI.CAdES.detached";
    case CryptoSign::SignatureType::adbe_pkcs7_detached:
        return "adbe.pkcs7.detached";
    case CryptoSign::SignatureType::adbe_pkcs7_sha1:
        return "adbe.pkcs7.sha1";
    case CryptoSign::SignatureType::g10c_pgp_signature_detached:
        return "g10c.pgp.signature.detached";
    }
    return "Unhandled";
}

void Factory::setPreferredBackend(CryptoSign::Backend::Type backend)
{
    preferredBackend = backend;
}
static std::string_view toStringView(const char *str)
{
    if (str) {
        return std::string_view(str);
    }
    return {};
}

std::optional<CryptoSign::Backend::Type> Factory::typeFromString(std::string_view string)
{
    if (string.empty()) {
        return std::nullopt;
    }
    if ("GPG" == string) {
        return Backend::Type::GPGME;
    }
    if ("NSS" == string) {
        return Backend::Type::NSS3;
    }
    return std::nullopt;
}

std::optional<CryptoSign::Backend::Type> Factory::getActive()
{
    if (preferredBackend) {
        return preferredBackend;
    }
    static auto backendFromEnvironment = typeFromString(toStringView(getenv("POPPLER_SIGNATURE_BACKEND")));
    if (backendFromEnvironment) {
        return backendFromEnvironment;
    }
    static auto backendFromCompiledDefault = typeFromString(toStringView(DEFAULT_SIGNATURE_BACKEND));
    if (backendFromCompiledDefault) {
        return backendFromCompiledDefault;
    }

    return std::nullopt;
}
static std::vector<Backend::Type> createAvailableBackends()
{
    std::vector<Backend::Type> backends;
#if ENABLE_NSS3
    backends.push_back(Backend::Type::NSS3);
#endif
#if ENABLE_GPGME
    if (GpgSignatureBackend::hasSufficientVersion()) {
        backends.push_back(Backend::Type::GPGME);
    }
#endif
    return backends;
}
std::vector<Backend::Type> Factory::getAvailable()
{
    static std::vector<Backend::Type> availableBackends = createAvailableBackends();
    return availableBackends;
}
std::unique_ptr<Backend> Factory::createActive()
{
    auto active = getActive();
    if (active) {
        return create(active.value());
    }
    return nullptr;
}
std::unique_ptr<CryptoSign::Backend> CryptoSign::Factory::create(Backend::Type backend)
{
    switch (backend) {
    case Backend::Type::NSS3:
#if ENABLE_NSS3
        return std::make_unique<NSSCryptoSignBackend>();
#else
        return nullptr;
#endif
    case Backend::Type::GPGME: {
#if ENABLE_GPGME
        return std::make_unique<GpgSignatureBackend>();
#else
        return nullptr;
#endif
    }
    }
    return nullptr;
}
/// backend specific settings

// Android build wants some methods out of line in the interfaces
Backend::~Backend() = default;
SigningInterface::~SigningInterface() = default;
VerificationInterface::~VerificationInterface() = default;

std::optional<Backend::Type> Factory::preferredBackend = std::nullopt;

} // namespace Signature;
