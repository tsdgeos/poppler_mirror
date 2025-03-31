//========================================================================
//
// GPGMECryptoSignBackendConfiguration.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023, 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================
#ifndef GPGME_CRYPTO_SIGN_BACKEND_CONFIGURATION_H
#define GPGME_CRYPTO_SIGN_BACKEND_CONFIGURATION_H
#include "poppler_private_export.h"
#include <vector>

class POPPLER_PRIVATE_EXPORT GpgSignatureConfiguration
{
public:
    static bool arePgpSignaturesAllowed();
    static void setPgpSignaturesAllowed(bool allowed);
    GpgSignatureConfiguration() = delete;

private:
    static bool allowPgp;
};

#endif //  GPGME_CRYPTO_SIGN_BACKEND_CONFIGURATION_H
