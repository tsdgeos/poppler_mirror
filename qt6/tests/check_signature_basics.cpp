//========================================================================
//
// check_signature_basics.cpp
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023-2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

// Simple tests of reading signatures
//
// Note that this does not check the actual validity because
// that will have an expiry date, and adding time bombs to unit tests is
// probably not a good idea.
#include <QtTest/QTest>
#include <QTemporaryDir>
#include "PDFDoc.h"
#include "GlobalParams.h"
#include "SignatureInfo.h"
#include "CryptoSignBackend.h"
#include "config.h"
#ifdef ENABLE_GPGME
#    include "GPGMECryptoSignBackend.h"
#    include <gpgme++/importresult.h>
#endif

#ifdef ENABLE_GPGME
const auto pubkey = R"ZxZ(
-----BEGIN PGP PUBLIC KEY BLOCK-----

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
xujPUZsL
=x7r+
-----END PGP PUBLIC KEY BLOCK-----
)ZxZ";
#endif

class TestSignatureBasics : public QObject
{
    Q_OBJECT
public:
    explicit TestSignatureBasics(QObject *parent = nullptr) : QObject(parent) { }

    std::unique_ptr<PDFDoc> doc;

    static std::unique_ptr<QTemporaryDir> tmpdir;
    static void initMain()
    {
        tmpdir = std::make_unique<QTemporaryDir>();
        qputenv("GNUPGHOME", tmpdir->path().toLocal8Bit().data());
    }

private Q_SLOTS:
    void init();
    void initTestCase_data();
    void initTestCase() { }
    void cleanupTestCase();
    void testSignatureCount();
    void testSignatureSizes();
    void testSignerInfo(); // names and stuff
    void testSignedRanges();
    void testPgp();
};

std::unique_ptr<QTemporaryDir> TestSignatureBasics::tmpdir;

void TestSignatureBasics::init()
{
#ifdef ENABLE_SIGNATURES
    QFETCH_GLOBAL(CryptoSign::Backend::Type, backend);
    CryptoSign::Factory::setPreferredBackend(backend);
    QCOMPARE(CryptoSign::Factory::getActive(), backend);
#endif

    globalParams = std::make_unique<GlobalParams>();
    doc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/pdf-signature-sample-2sigs.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isOk());
}

void TestSignatureBasics::initTestCase_data()
{
    QTest::addColumn<CryptoSign::Backend::Type>("backend");

#ifdef ENABLE_SIGNATURES
    const auto availableBackends = CryptoSign::Factory::getAvailable();

#    ifdef ENABLE_NSS3
    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::NSS3) != availableBackends.end()) {
        QTest::newRow("nss") << CryptoSign::Backend::Type::NSS3;
    } else {
        QWARN("Compiled with NSS3, but NSS not functional");
    }
#    endif
#    ifdef ENABLE_GPGME
    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
        QTest::newRow("gpg") << CryptoSign::Backend::Type::GPGME;
    } else {
        QWARN("Compiled with GPGME, but GPGME not functional");
    }
#    endif
#endif
}

void TestSignatureBasics::cleanupTestCase()
{
    globalParams.reset();
}

void TestSignatureBasics::testSignatureCount()
{
    QVERIFY(doc);
    auto signatureFields = doc->getSignatureFields();
    QCOMPARE(signatureFields.size(), 4);
    // count active signatures
    QVERIFY(!signatureFields[0]->getSignature().empty());
    QVERIFY(!signatureFields[1]->getSignature().empty());
    QVERIFY(signatureFields[2]->getSignature().empty());
    QVERIFY(signatureFields[3]->getSignature().empty());
}

void TestSignatureBasics::testSignatureSizes()
{
    auto signatureFields = doc->getSignatureFields();
    // These are not the actual signature lengths, but rather
    // the length of the signature field, which is likely
    // a padded field. At least the pdf specification suggest to pad
    // the field.
    // Poppler before 23.04 did not have a padded field, later versions do.
    QCOMPARE(signatureFields[0]->getSignature().size(), 10230); // Signature data size is 2340
    QCOMPARE(signatureFields[1]->getSignature().size(), 10196); // Signature data size is 2340
}

void TestSignatureBasics::testSignerInfo()
{
    auto signatureFields = doc->getSignatureFields();
    QCOMPARE(signatureFields[0]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature0_B_" });
    QCOMPARE(signatureFields[0]->getSignatureType(), CryptoSign::SignatureType::ETSI_CAdES_detached);
    auto siginfo0 = signatureFields[0]->validateSignatureAsync(false, false, -1 /* now */, false, false, {});
    signatureFields[0]->validateSignatureResult();
#ifdef ENABLE_SIGNATURES
    QCOMPARE(siginfo0->getSignerName(), std::string { "Koch, Werner" });
    QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Sha256);
    QCOMPARE(siginfo0->getCertificateInfo()->getPublicKeyInfo().publicKeyStrength, 2048 / 8);
#else
    QCOMPARE(siginfo0->getSignerName(), std::string {});
    QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Unknown);
#endif
    QCOMPARE(siginfo0->getSigningTime(), time_t(1677570911));

    QCOMPARE(signatureFields[1]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature1_B_" });
    QCOMPARE(signatureFields[1]->getSignatureType(), CryptoSign::SignatureType::ETSI_CAdES_detached);
    auto siginfo1 = signatureFields[1]->validateSignatureAsync(false, false, -1 /* now */, false, false, {});
    signatureFields[1]->validateSignatureResult();
#ifdef ENABLE_SIGNATURES
    QCOMPARE(siginfo1->getSignerName(), std::string { "Koch, Werner" });
    QCOMPARE(siginfo1->getHashAlgorithm(), HashAlgorithm::Sha256);
    QFETCH_GLOBAL(CryptoSign::Backend::Type, backend);
    if (backend == CryptoSign::Backend::Type::GPGME) {
        QCOMPARE(siginfo1->getCertificateInfo()->getPublicKeyInfo().publicKeyStrength, 2048 / 8);
    } else if (backend == CryptoSign::Backend::Type::NSS3) {
        // Not fully sure why it is zero here, but it seems to be.
        QCOMPARE(siginfo1->getCertificateInfo()->getPublicKeyInfo().publicKeyStrength, 0);
    }
#else
    QCOMPARE(siginfo1->getSignerName(), std::string {});
    QCOMPARE(siginfo1->getHashAlgorithm(), HashAlgorithm::Unknown);
#endif
    QCOMPARE(siginfo1->getSigningTime(), time_t(1677840601));

    QCOMPARE(signatureFields[2]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature2_B_" });
    QCOMPARE(signatureFields[2]->getSignatureType(), CryptoSign::SignatureType::unsigned_signature_field);
    QCOMPARE(signatureFields[3]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature3_B_" });
    QCOMPARE(signatureFields[3]->getSignatureType(), CryptoSign::SignatureType::unsigned_signature_field);
}

void TestSignatureBasics::testSignedRanges()
{
    auto signatureFields = doc->getSignatureFields();

    Goffset size0;
    auto sig0 = signatureFields[0]->getCheckedSignature(&size0);
    QVERIFY(sig0);
    auto ranges0 = signatureFields[0]->getSignedRangeBounds();
    QCOMPARE(ranges0.size(), 4);
    QCOMPARE(ranges0[0], 0);
    QCOMPARE(ranges0[1], 24890);
    QCOMPARE(ranges0[2], 45352);
    QCOMPARE(ranges0[3], 58529);
    QVERIFY(ranges0[3] != size0); // signature does not cover all of it

    Goffset size1;
    auto sig1 = signatureFields[1]->getCheckedSignature(&size1);
    QVERIFY(sig1);
    auto ranges1 = signatureFields[1]->getSignedRangeBounds();
    QCOMPARE(ranges1.size(), 4);
    QCOMPARE(ranges1[0], 0);
    QCOMPARE(ranges1[1], 59257);
    QCOMPARE(ranges1[2], 79651);
    QCOMPARE(ranges1[3], 92773);
    QCOMPARE(ranges1[3], size1); // signature does cover all of it
}

void TestSignatureBasics::testPgp()
{
    std::optional<CryptoSign::Backend::Type> usedBackend;
#ifdef ENABLE_GPGME
    QFETCH_GLOBAL(CryptoSign::Backend::Type, backend);
    usedBackend = backend;
    if (backend == CryptoSign::Backend::Type::GPGME) {
        GpgSignatureConfiguration::setPgpSignaturesAllowed(true);

        /* We need to fetch the key from the internet or elsewhere in order to get info about the key
           Alternatively we could embed it in the code here*/
        auto ctx = GpgME::Context::create(GpgME::Protocol::OpenPGP);
        GpgME::Data d;
        d.write(pubkey, strlen(pubkey));
        d.rewind();
        auto result = ctx->importKeys(d);
        QCOMPARE(result.numImported(), 1);
    }
#endif
    auto gpgDoc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/some-text-pgp_signed.pdf"));
    auto signatureFields = gpgDoc->getSignatureFields();
    QCOMPARE(signatureFields.size(), 1);
    QCOMPARE(signatureFields[0]->getSignatureType(), CryptoSign::SignatureType::g10c_pgp_signature_detached);
    QVERIFY(!signatureFields[0]->getSignature().empty());
    Goffset size0;
    auto sig0 = signatureFields[0]->getCheckedSignature(&size0);
    QVERIFY(sig0);
    auto ranges0 = signatureFields[0]->getSignedRangeBounds();
    QCOMPARE(ranges0.size(), 4);
    QCOMPARE(ranges0[0], 0);
    QCOMPARE(ranges0[1], 82991);
    QCOMPARE(ranges0[2], 102993);
    QCOMPARE(ranges0[3], 103534);

    auto siginfo0 = signatureFields[0]->validateSignatureAsync(false, false, -1 /* now */, false, false, {});
    signatureFields[0]->validateSignatureResult();
    if (usedBackend == CryptoSign::Backend::Type::GPGME) {
        QCOMPARE(siginfo0->getSignerName(), std::string { "Sune Vuorela" });
        QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Sha256);
        QCOMPARE(siginfo0->getCertificateInfo()->getPublicKeyInfo().publicKeyStrength, 4096);
        QCOMPARE(siginfo0->getCertificateInfo()->getNickName().toStr().substr(32), std::string { "F1F0D3ED" });
        QCOMPARE(siginfo0->getSignatureValStatus(), SignatureValidationStatus::SIGNATURE_VALID);
    } else {
        QCOMPARE(siginfo0->getSignerName(), std::string {});
        QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Unknown);
        QCOMPARE(siginfo0->getCertificateInfo(), nullptr);
        QCOMPARE(siginfo0->getSignatureValStatus(), SignatureValidationStatus::SIGNATURE_NOT_VERIFIED);
    }
}

QTEST_GUILESS_MAIN(TestSignatureBasics)
#include "check_signature_basics.moc"
