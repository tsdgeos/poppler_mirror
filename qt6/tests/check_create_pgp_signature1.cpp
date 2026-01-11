//========================================================================
//
// check_signature_basics.cpp
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright 2025 Albert Astals Cid <aacid@kde.org>
//========================================================================

// Simple tests of reading signatures
//

#include "config.h"
#include <filesystem>
#if ENABLE_NSS3
#    include "NSSCryptoSignBackend.h"
#endif
#include <QtTest/QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include "PDFDoc.h"
#include "GlobalParams.h"
#include "SignatureInfo.h"
#include "CryptoSignBackend.h"
#include "GPGMECryptoSignBackendConfiguration.h"
#include <gpgme++/importresult.h>
#include <gpgme++/context.h>
#include <gpgme++/data.h>

class TestSignWithGnupgPgp : public QObject
{
    Q_OBJECT
public:
    explicit TestSignWithGnupgPgp(QObject *parent = nullptr) : QObject(parent) { }

    static std::unique_ptr<QTemporaryDir> nssdir;
    static std::unique_ptr<QTemporaryDir> gpgdir;
    static void initMain()
    {
        nssdir = std::make_unique<QTemporaryDir>();
#if ENABLE_NSS3
        NSSSignatureConfiguration::setNSSDir(GooString(nssdir->path().toStdString()));
#endif
        gpgdir = std::make_unique<QTemporaryDir>();
        // copy out the data for two reasons:
        // 1) gpg-agent might get angry if the path is too long
        // 2) Ensure that accidental writes from the test (and e
        //    specially other tests getting inspired by this)
        //    does not carry over to the next tests
        std::filesystem::copy(TESTDATADIR "/unittestcases/check_create_pgp_signature1_keyring/", gpgdir->path().toStdString(), std::filesystem::copy_options::recursive);
        qputenv("GNUPGHOME", gpgdir->path().toLocal8Bit());
    }

private Q_SLOTS:
    static void init();
    static void initTestCase_data();
    void initTestCase() { }
    static void cleanupTestCase();
    static void testPgpSignVerify();
    static void testKeyList();
};

std::unique_ptr<QTemporaryDir> TestSignWithGnupgPgp::nssdir;
std::unique_ptr<QTemporaryDir> TestSignWithGnupgPgp::gpgdir;

void TestSignWithGnupgPgp::init()
{
    QFETCH_GLOBAL(CryptoSign::Backend::Type, backend);
    CryptoSign::Factory::setPreferredBackend(backend);
    QCOMPARE(CryptoSign::Factory::getActive(), backend);
    GpgSignatureConfiguration::setPgpSignaturesAllowed(true);

    globalParams = std::make_unique<GlobalParams>();
}

void TestSignWithGnupgPgp::initTestCase_data()
{
    QTest::addColumn<CryptoSign::Backend::Type>("backend");

    const auto availableBackends = CryptoSign::Factory::getAvailable();

#if ENABLE_NSS3
    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::NSS3) != availableBackends.end()) {
        QTest::newRow("nss") << CryptoSign::Backend::Type::NSS3;
    } else {
        QWARN("Compiled with NSS3, but NSS not functional");
    }
#endif
    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
        QTest::newRow("gpg") << CryptoSign::Backend::Type::GPGME;
    } else {
        QWARN("Compiled with GPGME, but GPGME not functional");
    }
}

void TestSignWithGnupgPgp::cleanupTestCase()
{
    globalParams.reset();
}

void TestSignWithGnupgPgp::testKeyList()
{
    auto backend = CryptoSign::Factory::createActive();
    QVERIFY(backend);

    auto certificateList = backend->getAvailableSigningCertificates();

    auto activeBackendType = CryptoSign::Factory::getActive();
    if (activeBackendType == CryptoSign::Backend::Type::NSS3) {
        QVERIFY(certificateList.empty());
    } else if (activeBackendType == CryptoSign::Backend::Type::GPGME) {
        QCOMPARE(certificateList.size(), 1);
        QCOMPARE(certificateList[0]->getNickName().c_str(), "36E39802E4F49A259091DA69381B80FEF3535BC1");
    }
}

void TestSignWithGnupgPgp::testPgpSignVerify()
{
    auto doc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY(doc->isOk());
    {
        auto signatureFields = doc->getSignatureFields();
        QCOMPARE(signatureFields.size(), 0);
    }

    QTemporaryDir d;

    auto signingResult =
            doc->sign(std::string { d.filePath(QStringLiteral("signedFile.pdf")).toStdString() }, std::string { "36E39802E4F49A259091DA69381B80FEF3535BC1" }, std::string {}, std::make_unique<GooString>("newSignatureFieldName"), /*page*/ 1,
                      /*rect */ { 0, 0, 0, 0 }, /*signatureText*/ {}, /*signatureTextLeft*/ {}, /*fontSize */ 0, /*leftFontSize*/ 0,
                      /*fontColor*/ {}, /*borderWidth*/ 0, /*borderColor*/ {}, /*backgroundColor*/ {}, /*reason*/ {}, /* location */ nullptr, /* image path */ "", {}, {});

    auto activeBackendType = CryptoSign::Factory::getActive();
    if (activeBackendType == CryptoSign::Backend::Type::NSS3) {
        QVERIFY(signingResult.has_value());
        QCOMPARE(signingResult.value().type, CryptoSign::SigningError::KeyMissing);
    } else if (activeBackendType == CryptoSign::Backend::Type::GPGME) {
        QVERIFY(!signingResult.has_value());
        auto signedDoc = std::make_unique<PDFDoc>(std::make_unique<GooString>(d.filePath(QStringLiteral("signedFile.pdf")).toStdString()));
        QVERIFY(signedDoc->isOk());
        auto signatureFields = signedDoc->getSignatureFields();
        QCOMPARE(signatureFields.size(), 1);
        QCOMPARE(signatureFields[0]->getSignatureType(), CryptoSign::SignatureType::g10c_pgp_signature_detached);
        auto *siginfo0 = signatureFields[0]->validateSignatureAsync(false, false, -1 /* now */, false, false, {});
        signatureFields[0]->validateSignatureResult();
        QCOMPARE(siginfo0->getSignatureValStatus(), SignatureValidationStatus::SIGNATURE_VALID);
        QCOMPARE(siginfo0->getSignerName(), std::string { "testuser" });
        QCOMPARE(siginfo0->getCertificateInfo()->getNickName().toStr(), std::string { "36E39802E4F49A259091DA69381B80FEF3535BC1" });
    }
}

QTEST_GUILESS_MAIN(TestSignWithGnupgPgp)
#include "check_create_pgp_signature1.moc"
