//========================================================================
//
// pdf-signing-nss.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2025 Juraj Šarinay <juraj@sarinay.com>
//
//========================================================================

#include <config.h>
#include <poppler-config.h>

#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "GlobalParams.h"
#include "NSSCryptoSignBackend.h"
#include "SignatureInfo.h"
#include "goo/GooString.h"

using namespace std::string_literals;

int sign_file(const char *nssdir, const char *nick, const char *input_file, const char *output_file);
int verify_file(const char *nssdir, const char *input_file);

int main(int argc, char *argv[])
{
    globalParams = std::make_unique<GlobalParams>();
    CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::NSS3);

    if (argc == 6 && argv[1] == "--sign"s) {
        // usage:
        // pdf-signing-nss --sign nssdir nick input output
        return sign_file(argv[2], argv[3], argv[4], argv[5]);
    }
    if (argc == 4 && argv[1] == "--verify"s) {
        // usage:
        // pdf-signing-nss --verify nssdir input
        return verify_file(argv[2], argv[3]);
    }
    return 1;
}

int sign_file(const char *nssdir, const char *nick, const char *input_file, const char *output_file)
{
    auto doc { PDFDocFactory().createPDFDoc(GooString { input_file }, {}, {}) };

    if (!doc->isOk()) {
        return 1;
    }

    NSSSignatureConfiguration::setNSSDir(GooString { nssdir });

    auto result = doc->sign({ output_file }, nick, {}, std::make_unique<GooString>("sig_creation_test"), /*page*/ 1,
                            /*rect */ { 0, 0, 0, 0 }, /*signatureText*/ {}, /*signatureTextLeft*/ {}, /*fontSize */ 0, /*leftFontSize*/ 0,
                            /*fontColor*/ {}, /*borderWidth*/ 0, /*borderColor*/ {}, /*backgroundColor*/ {}, {}, /* location */ nullptr, /* image path */ "", {}, {});
    return result.has_value();
}

int verify_file(const char *nssdir, const char *input_file)
{
    auto doc { PDFDocFactory().createPDFDoc(GooString { input_file }, {}, {}) };

    if (!doc->isOk()) {
        return 1;
    }

    NSSSignatureConfiguration::setNSSDir(GooString { nssdir });

    const auto signatures = doc->getSignatureFields();
    const auto sigCount = signatures.size();

    if (sigCount != 1) {
        return 1;
    }

    auto *ffs = signatures[0];
    auto *siginfo = ffs->validateSignatureAsync(true, false, -1, false, false, {});
    return siginfo->getSignatureValStatus() != SIGNATURE_VALID || ffs->validateSignatureResult() != CERTIFICATE_TRUSTED;
}
