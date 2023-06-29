//========================================================================
//
// check_signature_basics.cpp
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

// Simple tests of reading signatures
//
// Note that this does not check the actual validity because
// that will have an expiry date, and adding time bombs to unit tests is
// probably not a good idea.
#include <QtTest/QtTest>
#include "PDFDoc.h"
#include "GlobalParams.h"
#include "SignatureInfo.h"
#include "CryptoSignBackend.h"
#include "config.h"

class TestSignatureBasics : public QObject
{
    Q_OBJECT
public:
    explicit TestSignatureBasics(QObject *parent = nullptr) : QObject(parent) { }

private:
    std::unique_ptr<PDFDoc> doc;
private Q_SLOTS:
    void init();
    void initTestCase_data();
    void initTestCase() { }
    void cleanupTestCase();
    void testSignatureCount();
    void testSignatureSizes();
    void testSignerInfo(); // names and stuff
    void testSignedRanges();
};

Q_DECLARE_METATYPE(CryptoSign::Backend::Type);

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
    if (std::find(availableBackends.begin(), availableBackends.end(), CryptoSign::Backend::Type::NSS3) != availableBackends.end()) {
        QTest::newRow("nss") << CryptoSign::Backend::Type::NSS3;
    } else {
        QWARN("Compiled with NSS3, but NSS not functional");
    }
#    endif
#    ifdef ENABLE_GPGME
    if (std::find(availableBackends.begin(), availableBackends.end(), CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
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
    QVERIFY(signatureFields[0]->getSignature());
    QVERIFY(signatureFields[1]->getSignature());
    QVERIFY(!signatureFields[2]->getSignature());
    QVERIFY(!signatureFields[3]->getSignature());
}

void TestSignatureBasics::testSignatureSizes()
{
    auto signatureFields = doc->getSignatureFields();
    // These are not the actual signature lengths, but rather
    // the length of the signature field, which is likely
    // a padded field. At least the pdf specification suggest to pad
    // the field.
    // Poppler before 23.04 did not have a padded field, later versions do.
    QCOMPARE(signatureFields[0]->getSignature()->getLength(), 10230); // Signature data size is 2340
    QCOMPARE(signatureFields[1]->getSignature()->getLength(), 10196); // Signature data size is 2340
}

void TestSignatureBasics::testSignerInfo()
{
    auto signatureFields = doc->getSignatureFields();
    QCOMPARE(signatureFields[0]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature0_B_" });
    QCOMPARE(signatureFields[0]->getSignatureType(), ETSI_CAdES_detached);
    auto siginfo0 = signatureFields[0]->validateSignature(false, false, -1 /* now */, false, false);
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
    QCOMPARE(signatureFields[1]->getSignatureType(), ETSI_CAdES_detached);
    auto siginfo1 = signatureFields[1]->validateSignature(false, false, -1 /* now */, false, false);
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
    QCOMPARE(signatureFields[2]->getSignatureType(), unsigned_signature_field);
    QCOMPARE(signatureFields[3]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature3_B_" });
    QCOMPARE(signatureFields[3]->getSignatureType(), unsigned_signature_field);
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

QTEST_GUILESS_MAIN(TestSignatureBasics)
#include "check_signature_basics.moc"
