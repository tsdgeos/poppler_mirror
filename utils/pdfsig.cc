//========================================================================
//
// pdfsig.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2015 André Guerreiro <aguerreiro1985@gmail.com>
// Copyright 2015 André Esser <bepandre@hotmail.com>
// Copyright 2015, 2017-2026 Albert Astals Cid <aacid@kde.org>
// Copyright 2016 Markus Kilås <digital@markuspage.com>
// Copyright 2017, 2019 Hans-Ulrich Jüttner <huj@froreich-bioscientia.de>
// Copyright 2017, 2019 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@protonmail.com>
// Copyright 2019 Alexey Pavlov <alexpux@gmail.com>
// Copyright 2019. 2023, 2024 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2019 Nelson Efrain A. Cruz <neac03@gmail.com>
// Copyright 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright 2021 Theofilos Intzoglou <int.teo@gmail.com>
// Copyright 2022 Felix Jung <fxjung@posteo.de>
// Copyright 2022, 2024 Erich E. Hoover <erich.e.hoover@gmail.com>
// Copyright 2023-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2025 Blair Bonnett <blair.bonnett@gmail.com>
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fstream>
#include <random>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "parseargs.h"
#include "goo/gbasename.h"
#include "Page.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "DateInfo.h"
#include "GlobalParams.h"
#if ENABLE_NSS3
#    include "NSSCryptoSignBackend.h"
#endif
#if ENABLE_GPGME
#    include "GPGMECryptoSignBackendConfiguration.h"
#endif
#include "CryptoSignBackend.h"
#include "SignatureInfo.h"
#include "Win32Console.h"
#include "UTF.h"
#if __has_include(<libgen.h>)
#    include <libgen.h>
#endif

#ifdef HAVE_GETTEXT
#    include <libintl.h>
#    include <clocale>
#    define _(STRING) gettext(STRING)
#else
#    define _(STRING) STRING
#endif

static const char *getReadableSigState(SignatureValidationStatus sig_vs)
{
    switch (sig_vs) {
    case SIGNATURE_VALID:
        return "Signature is Valid.";

    case SIGNATURE_INVALID:
        return "Signature is Invalid.";

    case SIGNATURE_DIGEST_MISMATCH:
        return "Digest Mismatch.";

    case SIGNATURE_DECODING_ERROR:
        return "Document isn't signed or corrupted data.";

    case SIGNATURE_NOT_VERIFIED:
        return "Signature has not yet been verified.";

    case SIGNATURE_NOT_FOUND:
        return "Signature not found.";

    default:
        return "Unknown Validation Failure.";
    }
}

static const char *getReadableCertState(CertificateValidationStatus cert_vs)
{
    switch (cert_vs) {
    case CERTIFICATE_TRUSTED:
        return "Certificate is Trusted.";

    case CERTIFICATE_UNTRUSTED_ISSUER:
        return "Certificate issuer isn't Trusted.";

    case CERTIFICATE_UNKNOWN_ISSUER:
        return "Certificate issuer is unknown.";

    case CERTIFICATE_REVOKED:
        return "Certificate has been Revoked.";

    case CERTIFICATE_EXPIRED:
        return "Certificate has Expired";

    case CERTIFICATE_NOT_VERIFIED:
        return "Certificate has not yet been verified.";

    default:
        return "Unknown issue with Certificate or corrupted data.";
    }
}

static std::string getReadableTime(time_t unix_time)
{
    std::stringstream stringStream;
    const std::tm tm = *std::localtime(&unix_time);
    stringStream << std::put_time(&tm, "%b %d %Y %H:%M:%S");
    return stringStream.str();
}

static std::string_view trim(std::string_view input)
{
    size_t first = input.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    size_t last = input.find_last_not_of(" \t");
    return input.substr(first, last - first + 1);
}

static std::vector<std::string> parseAssertSignerFile(std::string_view input)
{
    std::fstream file(std::string { input });
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::string_view trimmedLine = trim(line);

        if (!trimmedLine.empty()) {
            lines.emplace_back(trimmedLine);
        }
    }
    return lines;
}

static std::vector<std::string> parseAssertSigner(std::string_view input)
{
    if (std::filesystem::exists(input)) {
        return parseAssertSignerFile(input);
    }
    return std::vector<std::string> { std::string { input } };
}

static bool dumpSignature(int sig_num, FormFieldSignature *s, const char *filename)
{
    const std::vector<unsigned char> &signature = s->getSignature();
    if (signature.empty()) {
        printf("Cannot dump signature #%d\n", sig_num);
        return false;
    }

    const std::string filenameWithExtension = GooString::format("{0:s}.sig", gbasename(filename).c_str());
    const std::string sig_numString = std::to_string(sig_num);
    const std::string path = filenameWithExtension + sig_numString;
    printf("Signature #%d (%lu bytes) => %s\n", sig_num, signature.size(), path.c_str());
    std::ofstream outfile(path.c_str(), std::ofstream::binary);
    outfile.write(reinterpret_cast<const char *>(signature.data()), signature.size());
    outfile.close();

    return true;
}

static GooString nssDir;
static GooString nssPassword;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool printVersion = false;
static bool printHelp = false;
static bool printCryptoSignBackends = false;
static bool dontVerifyCert = false;
#if ENABLE_GPGME
static bool allowPgp = GpgSignatureConfiguration::arePgpSignaturesAllowed();
#else
static bool allowPgp = false;
#endif
static bool noOCSPRevocationCheck = false;
static bool noAppearance = false;
static bool dumpSignatures = false;
static bool etsiCAdESdetached = false;
static char backendString[256] = "";
static char signatureName[256] = "";
static char certNickname[256] = "";
static char password[256] = "";
static char digestName[256] = "SHA256";
static GooString reason;
static bool listNicknames = false;
static bool addNewSignature = false;
static bool useAIACertFetch = false;
static GooString newSignatureFieldName;
static char assertSigner[256] = "";

static const ArgDesc argDesc[] = {
    { .arg = "-nssdir", .kind = argGooString, .val = &nssDir, .size = 0, .usage = "path to directory of libnss3 database" },
    { .arg = "-nss-pwd", .kind = argGooString, .val = &nssPassword, .size = 0, .usage = "password to access the NSS database (if any)" },
    { .arg = "-nocert", .kind = argFlag, .val = &dontVerifyCert, .size = 0, .usage = "don't perform certificate validation" },
    { .arg = "-no-ocsp", .kind = argFlag, .val = &noOCSPRevocationCheck, .size = 0, .usage = "don't perform online OCSP certificate revocation check" },
    { .arg = "-no-appearance", .kind = argFlag, .val = &noAppearance, .size = 0, .usage = "don't add appearance information when signing existing fields" },
    { .arg = "-aia", .kind = argFlag, .val = &useAIACertFetch, .size = 0, .usage = "use Authority Information Access (AIA) extension for certificate fetching" },
    { .arg = "-assert-signer",
      .kind = argString,
      .val = &assertSigner,
      .size = 256,
      .usage = "This option checks whether the signature covering the full document been made with the specified key. The key is either specified as a fingerprint or a file listing fingerprints. The fingerprint must "
               "be given or listed in compact format (no colons or spaces in between). If fpr_or_file specifies a file, empty lines are ignored as well as all lines starting with a hash sign. Only available for GnuPG backend." },
    { .arg = "-dump", .kind = argFlag, .val = &dumpSignatures, .size = 0, .usage = "dump all signatures into current directory" },
    { .arg = "-add-signature", .kind = argFlag, .val = &addNewSignature, .size = 0, .usage = "adds a new signature to the document" },
    { .arg = "-new-signature-field-name", .kind = argGooString, .val = &newSignatureFieldName, .size = 0, .usage = "field name used for the newly added signature. A random ID will be used if empty" },
    { .arg = "-sign", .kind = argString, .val = &signatureName, .size = 256, .usage = "sign the document in the given signature field (by name or number)" },
    { .arg = "-etsi", .kind = argFlag, .val = &etsiCAdESdetached, .size = 0, .usage = "create a signature of type ETSI.CAdES.detached instead of adbe.pkcs7.detached" },
    { .arg = "-backend", .kind = argString, .val = &backendString, .size = 256, .usage = "use given backend for signing/verification" },
    { .arg = "-enable-pgp", .kind = argFlag, .val = &allowPgp, .size = 0, .usage = "Enable pgp signatures in the GnuPG backend. Only available for GnuPG backend" },
    { .arg = "-nick", .kind = argString, .val = &certNickname, .size = 256, .usage = "use the certificate with the given nickname/fingerprint for signing" },
    { .arg = "-kpw", .kind = argString, .val = &password, .size = 256, .usage = "password for the signing key (might be missing if the key isn't password protected)" },
    { .arg = "-digest", .kind = argString, .val = &digestName, .size = 256, .usage = "name of the digest algorithm (default: SHA256)" },
    { .arg = "-reason", .kind = argGooString, .val = &reason, .size = 0, .usage = "reason for signing (default: no reason given)" },
    { .arg = "-list-nicks", .kind = argFlag, .val = &listNicknames, .size = 0, .usage = "list available nicknames in the NSS database" },
    { .arg = "-list-backends", .kind = argFlag, .val = &printCryptoSignBackends, .size = 0, .usage = "print cryptographic signature backends" },
    { .arg = "-opw", .kind = argString, .val = ownerPassword, .size = sizeof(ownerPassword), .usage = "owner password (for encrypted files)" },
    { .arg = "-upw", .kind = argString, .val = userPassword, .size = sizeof(userPassword), .usage = "user password (for encrypted files)" },
    { .arg = "-v", .kind = argFlag, .val = &printVersion, .size = 0, .usage = "print copyright and version info" },
    { .arg = "-h", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "-help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "--help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "-?", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    {}
};

static void print_version_usage(bool usage)
{
    fprintf(stderr, "pdfsig version %s\n", PACKAGE_VERSION);
    fprintf(stderr, "%s\n", popplerCopyright);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (usage) {
        printUsage("pdfsig", "<PDF-file> [<output-file>]", argDesc);
    }
}

static void print_backends()
{
    fprintf(stderr, "pdfsig backends:\n");
    for (const auto &backend : CryptoSign::Factory::getAvailable()) {
        switch (backend) {
        case CryptoSign::Backend::Type::NSS3:
            fprintf(stderr, "NSS");
            break;
        case CryptoSign::Backend::Type::GPGME:
            fprintf(stderr, "GPG");
            break;
        }
        if (backend == CryptoSign::Factory::getActive()) {
            fprintf(stderr, " (active)\n");
        } else {
            fprintf(stderr, "\n");
        }
    }
}

static std::vector<std::unique_ptr<X509CertificateInfo>> getAvailableSigningCertificates(bool *error)
{
#if ENABLE_NSS3
    bool wrongPassword = false;
    bool passwordNeeded = false;
    auto passwordCallback = [&passwordNeeded, &wrongPassword](const char *) -> char * {
        static bool firstTime = true;
        if (!firstTime) {
            wrongPassword = true;
            return nullptr;
        }
        firstTime = false;
        if (!nssPassword.empty()) {
            return strdup(nssPassword.c_str());
        }
        passwordNeeded = true;
        return nullptr;
    };
    NSSSignatureConfiguration::setNSSPasswordCallback(passwordCallback);
#endif
    auto backend = CryptoSign::Factory::createActive();
    if (!backend) {
        *error = true;
        printf("No backends for cryptographic signatures available");
        return {};
    }
    std::vector<std::unique_ptr<X509CertificateInfo>> vCerts = backend->getAvailableSigningCertificates();
#if ENABLE_NSS3
    NSSSignatureConfiguration::setNSSPasswordCallback({});
    if (passwordNeeded) {
        *error = true;
        printf("Password is needed to access the NSS database.\n");
        printf("\tPlease provide one with -nss-pwd.\n");
        return {};
    }
    if (wrongPassword) {
        *error = true;
        printf("Password was not accepted to open the NSS database.\n");
        printf("\tPlease provide the correct one with -nss-pwd.\n");
        return {};
    }

#endif
    *error = false;
    return vCerts;
}

static std::string locationToString(KeyLocation location)
{
    switch (location) {
    case KeyLocation::Unknown:
        return {};
    case KeyLocation::Other:
        return "(Other)";
    case KeyLocation::Computer:
        return "(Computer)";
    case KeyLocation::HardwareToken:
        return "(Hardware Token)";
    }
    return {};
}

static const char *typeToString(CertificateType type)
{
    switch (type) {
    case CertificateType::PGP:
        return "PGP";
    case CertificateType::X509:
        return "S/Mime";
    }
    return "";
}

static std::string TextStringToUTF8(const std::string &str)
{
    const UnicodeMap *utf8Map = globalParams->getUtf8Map();

    std::vector<Unicode> u = TextStringToUCS4(str);

    std::string convertedStr;
    for (auto &c : u) {
        char buf[8];
        const int n = utf8Map->mapUnicode(c, buf, sizeof(buf));
        convertedStr.append(buf, n);
    }

    return convertedStr;
}

int main(int argc, char *argv[])
{
    globalParams = std::make_unique<GlobalParams>();

    Win32Console win32Console(&argc, &argv);

    const bool ok = parseArgs(argDesc, &argc, argv);

    if (!ok) {
        print_version_usage(true);
        return 99;
    }

    if (printVersion) {
        print_version_usage(false);
        return 0;
    }

    if (printHelp) {
        print_version_usage(true);
        return 0;
    }

    if (strlen(backendString) > 0) {
        auto backend = CryptoSign::Factory::typeFromString(backendString);
        if (backend) {
            CryptoSign::Factory::setPreferredBackend(backend.value());
        } else {
            fprintf(stderr, "Unsupported backend\n");
            return 98;
        }
    }

    if (printCryptoSignBackends) {
        print_backends();
        return 0;
    }

#if ENABLE_NSS3
    NSSSignatureConfiguration::setNSSDir(nssDir);
#endif
#if ENABLE_GPGME
    GpgSignatureConfiguration::setPgpSignaturesAllowed(allowPgp);
#else
    if (allowPgp) {
        printf("Pgp support not enabled in this build.\n");
        return 99;
    }
#endif

    if (listNicknames) {
        bool getCertsError;
        const std::vector<std::unique_ptr<X509CertificateInfo>> vCerts = getAvailableSigningCertificates(&getCertsError);
        if (getCertsError) {
            return 2;
        }
        if (vCerts.empty()) {
            printf("There are no certificates available.\n");
        } else {
            printf("Certificate nicknames available:\n");
            for (const auto &cert : vCerts) {
                const GooString &nick = cert->getNickName();
                const auto location = locationToString(cert->getKeyLocation());
                printf("%s %s %s %s\n", nick.c_str(), (cert->isQualified() ? "(*)" : "   "), location.c_str(), allowPgp ? typeToString(cert->getCertificateType()) : "");
            }
        }

        return 0;
    }

    if (argc < 2) {
        // no filename was given
        print_version_usage(true);
        return 99;
    }

    std::unique_ptr<GooString> fileName = std::make_unique<GooString>(argv[1]);

    std::optional<GooString> ownerPW, userPW;
    if (ownerPassword[0] != '\001') {
        ownerPW = GooString(ownerPassword);
    }
    if (userPassword[0] != '\001') {
        userPW = GooString(userPassword);
    }
    // open PDF file
    std::unique_ptr<PDFDoc> doc(PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW));

    if (!doc->isOk()) {
        return 1;
    }

    std::optional<unsigned int> signatureNumber;
    if (strlen(signatureName) > 0) {
        signatureNumber = atoi(signatureName);
        if (signatureNumber == 0) {
            signatureNumber = {};
        }
    }

    if (addNewSignature && signatureNumber.has_value()) {
        // incompatible options
        print_version_usage(true);
        return 99;
    }

    if (addNewSignature) {
        if (argc == 2) {
            fprintf(stderr, "An output filename for the signed document must be given\n");
            return 2;
        }

        if (strlen(certNickname) == 0) {
            printf("A nickname of the signing certificate must be given\n");
            return 2;
        }

        if (etsiCAdESdetached) {
            printf("-etsi is not supported yet with -add-signature\n");
            printf("Please file a bug report if this is important for you\n");
            return 2;
        }

        if (digestName != std::string("SHA256")) {
            printf("Only digest SHA256 is supported at the moment with -add-signature\n");
            printf("Please file a bug report if this is important for you\n");
            return 2;
        }

        if (doc->getPage(1) == nullptr) {
            printf("Error getting first page of the document.\n");
            return 2;
        }

        bool getCertsError;
        // We need to call this otherwise NSS spins forever
        getAvailableSigningCertificates(&getCertsError);
        if (getCertsError) {
            return 2;
        }

        const auto rs = std::unique_ptr<GooString>(reason.toStr().empty() ? nullptr : std::make_unique<GooString>(utf8ToUtf16WithBom(reason.toStr())));

        if (newSignatureFieldName.empty()) {
            // Create a random field name, it could be anything but 32 hex numbers should
            // hopefully give us something that is not already in the document
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 15);
            for (int i = 0; i < 32; ++i) {
                const int value = distrib(gen);
                newSignatureFieldName.push_back(value < 10 ? 48 + value : 65 + (value - 10));
            }
        }

        // We don't provide a way to customize the UI from pdfsig for now
        const auto failure = doc->sign(std::string { argv[2] }, std::string { certNickname }, std::string { password }, newSignatureFieldName.copy(), /*page*/ 1,
                                       /*rect */ { 0, 0, 0, 0 }, /*signatureText*/ {}, /*signatureTextLeft*/ {}, /*fontSize */ 0, /*leftFontSize*/ 0,
                                       /*fontColor*/ {}, /*borderWidth*/ 0, /*borderColor*/ {}, /*backgroundColor*/ {}, rs.get(), /* location */ nullptr, /* image path */ "", ownerPW, userPW);
        return !failure.has_value() ? 0 : 3;
    }

    const std::vector<FormFieldSignature *> signatures = doc->getSignatureFields();
    const unsigned int sigCount = signatures.size();

    if (!signatureNumber.has_value() && strlen(signatureName)) {
        for (unsigned int i = 0; i < sigCount; i++) {
            const GooString *goo = signatures.at(i)->getCreateWidget()->getField()->getFullyQualifiedName();
            if (!goo) {
                continue;
            }

            const std::string name = TextStringToUTF8(goo->toStr());
            if (name == signatureName) {
                signatureNumber = i + 1;
                break;
            }
        }

        if (!signatureNumber.has_value()) {
            fprintf(stderr, "Did not find signature field with name: %s\n", signatureName);
            return 2;
        }
    }

    if (signatureNumber.has_value()) {
        // We are signing an existing signature field
        if (argc == 2) {
            fprintf(stderr, "An output filename for the signed document must be given\n");
            return 2;
        }

        if (signatureNumber > sigCount) {
            printf("File '%s' does not contain a signature with number %d\n", fileName->c_str(), *signatureNumber);
            return 2;
        }

        if (strlen(certNickname) == 0) {
            printf("A nickname of the signing certificate must be given\n");
            return 2;
        }

        if (digestName != std::string("SHA256")) {
            printf("Only digest SHA256 is supported at the moment\n");
            printf("Please file a bug report if this is important for you\n");
            return 2;
        }

        bool getCertsError;
        // We need to call this otherwise NSS spins forever
        getAvailableSigningCertificates(&getCertsError);
        if (getCertsError) {
            return 2;
        }

        FormFieldSignature *ffs = signatures.at(*signatureNumber - 1);
        auto [sig, file_size] = ffs->getCheckedSignature();
        if (sig) {
            printf("Signature number %d is already signed\n", *signatureNumber);
            return 2;
        }
        if (etsiCAdESdetached) {
            ffs->setSignatureType(CryptoSign::SignatureType::ETSI_CAdES_detached);
        }
        const auto rs = std::unique_ptr<GooString>(reason.toStr().empty() ? nullptr : std::make_unique<GooString>(utf8ToUtf16WithBom(reason.toStr())));
        if (ffs->getNumWidgets() != 1) {
            printf("Unexpected number of widgets for the signature: %d\n", ffs->getNumWidgets());
            return 2;
        }
#ifdef HAVE_GETTEXT
        if (!noAppearance) {
            setlocale(LC_ALL, "");
            bindtextdomain("pdfsig", CMAKE_INSTALL_LOCALEDIR);
            textdomain("pdfsig");
        }
#endif
        auto *fws = static_cast<FormWidgetSignature *>(ffs->getWidget(0));
        auto backend = CryptoSign::Factory::createActive();
        auto sigHandler = backend->createSigningHandler(certNickname, HashAlgorithm::Sha256);
        std::unique_ptr<X509CertificateInfo> certInfo = sigHandler->getCertificateInfo();
        if (!certInfo) {
            fprintf(stderr, "signDocument: error getting signature info\n");
            return 2;
        }
        const std::string signerName = certInfo->getSubjectInfo().commonName;
        const std::string timestamp = timeToStringWithFormat(nullptr, "%Y.%m.%d %H:%M:%S %z");
        const AnnotColor blackColor(0, 0, 0);
        const std::string signatureText(GooString::format(_("Digitally signed by {0:s}"), signerName.c_str()) + "\n" + GooString::format(_("Date: {0:s}"), timestamp.c_str()));
        const auto gSignatureText = std::make_unique<GooString>((signatureText.empty() || noAppearance) ? "" : utf8ToUtf16WithBom(signatureText));
        const auto gSignatureLeftText = std::make_unique<GooString>((signerName.empty() || noAppearance) ? "" : utf8ToUtf16WithBom(signerName));
        const auto failure = fws->signDocumentWithAppearance(argv[2], std::string { certNickname }, std::string { password }, rs.get(), nullptr, {}, {}, *gSignatureText, *gSignatureLeftText, 0, 0, std::make_unique<AnnotColor>(blackColor));
        return !failure.has_value() ? 0 : 3;
    }

    if (argc > 2) {
        // We are not signing and more than 1 filename was given
        print_version_usage(true);
        return 99;
    }

    if (sigCount >= 1) {
        if (dumpSignatures) {
            printf("Dumping Signatures: %u\n", sigCount);
            for (unsigned int i = 0; i < sigCount; i++) {
                const bool dumpingOk = dumpSignature(i, signatures.at(i), fileName->c_str());
                if (!dumpingOk) {
                    // for now, do nothing. We have logged a message
                    // to the user before returning false in dumpSignature
                    // and it is possible to have "holes" in the signatures
                    continue;
                }
            }
            return 0;
        }
        printf("Digital Signature Info of: %s\n", fileName->c_str());

    } else {
        printf("File '%s' does not contain any signatures\n", fileName->c_str());
        return 2;
    }
    std::unordered_map<int, SignatureInfo *> signatureInfos;
    for (unsigned int i = 0; i < sigCount; i++) {
        // Let's start the signature check first for signatures.
        // we can always wait for completion later
        FormFieldSignature *ffs = signatures.at(i);
        if (ffs->getSignatureType() == CryptoSign::SignatureType::unsigned_signature_field) {
            continue;
        }
        signatureInfos[i] = ffs->validateSignatureAsync(!dontVerifyCert, false, -1 /* now */, !noOCSPRevocationCheck, useAIACertFetch, {});
    }
    bool totalDocumentSigned = false;
    bool oneSignatureInvalid = false;
    std::string totalDocumentNick;

    for (unsigned int i = 0; i < sigCount; i++) {
        FormFieldSignature *ffs = signatures.at(i);
        printf("Signature #%u:\n", i + 1);
        const GooString *goo = ffs->getCreateWidget()->getField()->getFullyQualifiedName();
        if (goo) {
            const std::string name = TextStringToUTF8(goo->toStr());
            printf("  - Signature Field Name: %s\n", name.c_str());
        }

        if (ffs->getSignatureType() == CryptoSign::SignatureType::unsigned_signature_field) {
            printf("  The signature form field is not signed.\n");
            continue;
        }

        const SignatureInfo *sig_info = signatureInfos[i];
        CertificateValidationStatus certificateStatus = ffs->validateSignatureResult();
        if (sig_info->getSignatureValStatus() == SIGNATURE_DECODING_ERROR) {
            printf("  - Decoding failed\n");
            oneSignatureInvalid = true;
            continue;
        }
        printf("  - Signer Certificate Common Name: %s\n", sig_info->getSignerName().c_str());
        if (CryptoSign::Factory::getActive() == CryptoSign::Backend::Type::GPGME) {
            printf("  - Signer fingerprint: %s\n", sig_info->getCertificateInfo()->getNickName().c_str());
        }
        printf("  - Signer full Distinguished Name: %s\n", sig_info->getSubjectDN().c_str());
        printf("  - Signing Time: %s\n", getReadableTime(sig_info->getSigningTime()).c_str());
        printf("  - Signing Hash Algorithm: ");
        switch (sig_info->getHashAlgorithm()) {
        case HashAlgorithm::Md2:
            printf("MD2\n");
            break;
        case HashAlgorithm::Md5:
            printf("MD5\n");
            break;
        case HashAlgorithm::Sha1:
            printf("SHA1\n");
            break;
        case HashAlgorithm::Sha256:
            printf("SHA-256\n");
            break;
        case HashAlgorithm::Sha384:
            printf("SHA-384\n");
            break;
        case HashAlgorithm::Sha512:
            printf("SHA-512\n");
            break;
        case HashAlgorithm::Sha224:
            printf("SHA-224\n");
            break;
        default:
            printf("unknown\n");
        }
        printf("  - Signature Type: ");
        switch (ffs->getSignatureType()) {
        case CryptoSign::SignatureType::adbe_pkcs7_sha1:
            printf("adbe.pkcs7.sha1\n");
            break;
        case CryptoSign::SignatureType::adbe_pkcs7_detached:
            printf("adbe.pkcs7.detached\n");
            break;
        case CryptoSign::SignatureType::ETSI_CAdES_detached:
            printf("ETSI.CAdES.detached\n");
            break;
        case CryptoSign::SignatureType::g10c_pgp_signature_detached:
            printf("g10c.pgp.signature.detached\n");
            break;
        case CryptoSign::SignatureType::unknown_signature_type:
        case CryptoSign::SignatureType::unsigned_signature_field: /*shouldn't happen*/
            printf("unknown\n");
        }
        const std::vector<Goffset> ranges = ffs->getSignedRangeBounds();
        if (ranges.size() == 4) {
            printf("  - Signed Ranges: [%lld - %lld], [%lld - %lld]\n", ranges[0], ranges[1], ranges[2], ranges[3]);
            auto [signature, checked_file_size] = signatures.at(i)->getCheckedSignature();
            if (signature && checked_file_size == ranges[3]) {
                if (totalDocumentSigned) {
                    printf("multiple signatures is covering entire document. Impossible");
                    return 2;
                }
                if (sig_info->getCertificateInfo()) {
                    totalDocumentSigned = true;
                    totalDocumentNick = sig_info->getCertificateInfo()->getNickName().toStr();
                }
                printf("  - Total document signed\n");
            } else {
                printf("  - Not total document signed\n");
            }
        }
        printf("  - Signature Validation: %s\n", getReadableSigState(sig_info->getSignatureValStatus()));
        if (sig_info->getSignatureValStatus() != SIGNATURE_VALID) {
            oneSignatureInvalid = true;
            continue;
        }
        if (dontVerifyCert) {
            continue;
        }
        printf("  - Certificate Validation: %s\n", getReadableCertState(certificateStatus));
    }

    if (!oneSignatureInvalid) {
        if (strlen(assertSigner) > 1 && CryptoSign::Factory::getActive() == CryptoSign::Backend::Type::GPGME) {
            if (!totalDocumentSigned) {
                printf("  - Assert signer: Total document not signed\n");
                return 1;
            }
            std::vector<std::string> assertKeys = parseAssertSigner(assertSigner);
            if (std::ranges::find(assertKeys, totalDocumentNick) == std::end(assertKeys)) {
                printf("  - Assert signer: Key not in list\n");
                // we do't have the key in the assert list, so error out
                return 1;
            }
        }
        return 0;
    }
    return 1;
}
