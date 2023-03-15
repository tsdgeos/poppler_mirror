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
#include "config.h"

class TestSignatureBasics : public QObject
{
    Q_OBJECT
public:
    explicit TestSignatureBasics(QObject *parent = nullptr) : QObject(parent) { }

private:
    std::unique_ptr<PDFDoc> doc;
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testSignatureCount();
    void testSignatureSizes();
    void testSignerInfo(); // names and stuff
    void testSignedRanges();
};

void TestSignatureBasics::initTestCase()
{
    globalParams = std::make_unique<GlobalParams>();
    doc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/pdf-signature-sample-2sigs.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isOk());
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
    // Note for later. Unpadding a signature on a command line with openssl can
    // be done just by rewriting after using e.g. pdfsig -dump to extract them
    // openssl pkcs7 -inform der -in pdf-signature-sample-2sigs.pdf.sig0 -outform der -out pdf-signature-sample-2sigs.pdf.sig0.unpadded
    QCOMPARE(signatureFields[0]->getSignature()->getLength(), 10230); // This is technically wrong, because the signatures in this document has been padded. The correct size is 2340
    QCOMPARE(signatureFields[1]->getSignature()->getLength(), 10196); // This is technically wrong, because the signatures in this document has been padded. The correct size is 2340
}

void TestSignatureBasics::testSignerInfo()
{
    auto signatureFields = doc->getSignatureFields();
    QCOMPARE(signatureFields[0]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature0_B_" });
    QCOMPARE(signatureFields[0]->getSignatureType(), ETSI_CAdES_detached);
    auto siginfo0 = signatureFields[0]->validateSignature(false, false, -1 /* now */, false, false);
#ifdef ENABLE_NSS3
    QCOMPARE(siginfo0->getSignerName(), std::string { "Koch, Werner" });
    QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Sha256);
#else
    QCOMPARE(siginfo0->getSignerName(), std::string {});
    QCOMPARE(siginfo0->getHashAlgorithm(), HashAlgorithm::Unknown);
#endif
    QCOMPARE(siginfo0->getSigningTime(), time_t(1677570911));

    QCOMPARE(signatureFields[1]->getCreateWidget()->getField()->getFullyQualifiedName()->toStr(), std::string { "P2.AnA_Signature1_B_" });
    QCOMPARE(signatureFields[1]->getSignatureType(), ETSI_CAdES_detached);
    auto siginfo1 = signatureFields[1]->validateSignature(false, false, -1 /* now */, false, false);
#ifdef ENABLE_NSS3
    QCOMPARE(siginfo1->getSignerName(), std::string { "Koch, Werner" });
    QCOMPARE(siginfo1->getHashAlgorithm(), HashAlgorithm::Sha256);
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
