//========================================================================
//
// pdf-validate-signature.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2025 Juraj Šarinay <juraj@sarinay.com>
//
//========================================================================

#include <string>
#include <config.h>
#include <poppler-config.h>

#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "GlobalParams.h"
#include "CryptoSignBackend.h"
#include "SignatureInfo.h"

using namespace std::string_literals;

int main(int argc, char *argv[])
{
    globalParams = std::make_unique<GlobalParams>();

    if (argc != 3) {
        return 1;
    }

    auto filename = std::make_unique<GooString>(argv[1]);
    auto doc { PDFDocFactory().createPDFDoc(*filename, {}, {}) };

    if (!doc->isOk()) {
        return 1;
    }

    SignatureValidationStatus expected_status = SIGNATURE_VALID;

    if (argv[2] == "--invalid"s) {
        expected_status = SIGNATURE_INVALID;
    } else if (argv[2] == "--digest-mismatch"s) {
        expected_status = SIGNATURE_DIGEST_MISMATCH;
    } else if (argv[2] != "--valid"s) {
        return 1;
    }

    CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::NSS3);

    const auto signatures = doc->getSignatureFields();
    const auto sigCount = signatures.size();

    if (sigCount != 1) {
        return 1;
    }

    auto *ffs = signatures[0];
    auto *siginfo = ffs->validateSignatureAsync(false, false, -1, false, false, {});

    return siginfo->getSignatureValStatus() != expected_status;
}
