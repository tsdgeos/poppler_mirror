//========================================================================
//
// check_signature_basics.cpp
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

// Simple tests of reading signatures
//

#include "config.h"
#include <filesystem>
#include <QtTest/QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include "Form.h"
#include "PDFDoc.h"
#include "GlobalParams.h"
#include "SignatureInfo.h"
#include "CryptoSignBackend.h"
#include "NSSCryptoSignBackend.h"
#include "GPGMECryptoSignBackendConfiguration.h"
#include <gpgme++/importresult.h>
#include <gpgme++/context.h>
#include <gpgme++/data.h>

class CheckSignatureCrossValidation : public QObject
{
    Q_OBJECT
public:
    explicit CheckSignatureCrossValidation(QObject *parent = nullptr) : QObject(parent) { }

    static std::unique_ptr<QTemporaryDir> nssdir;
    static std::unique_ptr<QTemporaryDir> gpgdir;
    static std::string keyId;
    static void initMain()
    {
        nssdir = std::make_unique<QTemporaryDir>();
        std::filesystem::copy(TESTDATADIR "/unittestcases/signature-cross-validation/nsshome", nssdir->path().toStdString(), std::filesystem::copy_options::recursive);

        NSSSignatureConfiguration::setNSSDir(GooString(nssdir->path().toStdString()));
        NSSSignatureConfiguration::setNSSPasswordCallback([](const char *) { return strdup("passphrase"); });
        gpgdir = std::make_unique<QTemporaryDir>();
        // copy out the data for two reasons:
        // 1) gpg-agent might get angry if the path is too long
        // 2) Ensure that accidental writes from the test (and e
        //    specially other tests getting inspired by this)
        //    does not carry over to the next tests
        std::filesystem::copy(TESTDATADIR "/unittestcases/signature-cross-validation/gnupghome", gpgdir->path().toStdString(), std::filesystem::copy_options::recursive);
        qputenv("GNUPGHOME", gpgdir->path().toLocal8Bit());
        qDebug() << gpgdir->path();
    }

private Q_SLOTS:
    static void init();
    static void initTestCase_data();
    void initTestCase() { }
    static void cleanupTestCase();
    static void testKeyList();
    static void testSignVerify();
};

std::unique_ptr<QTemporaryDir> CheckSignatureCrossValidation::nssdir;
std::unique_ptr<QTemporaryDir> CheckSignatureCrossValidation::gpgdir;
std::string CheckSignatureCrossValidation::keyId;

void CheckSignatureCrossValidation::init()
{
    QFETCH_GLOBAL(CryptoSign::Backend::Type, backend);
    QFETCH_GLOBAL(std::string, keyid);
    CryptoSign::Factory::setPreferredBackend(backend);
    QCOMPARE(CryptoSign::Factory::getActive(), backend);
    keyId = keyid;

    globalParams = std::make_unique<GlobalParams>();
}

void CheckSignatureCrossValidation::initTestCase_data()
{
    QTest::addColumn<CryptoSign::Backend::Type>("backend");
    QTest::addColumn<std::string>("keyid");

    const auto availableBackends = CryptoSign::Factory::getAvailable();

    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::NSS3) != availableBackends.end()) {
        QTest::newRow("nss") << CryptoSign::Backend::Type::NSS3 << std::string { "Alice Bob" };
    } else {
        QFAIL("Compiled with NSS3, but NSS not functional");
    }
    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
        QTest::newRow("gpg") << CryptoSign::Backend::Type::GPGME << std::string { "69A3AF7FD6422E2811530676984754AFB60D07C7" };
    } else {
        QFAIL("Compiled with GPGME, but GPGME not functional");
    }
}

void CheckSignatureCrossValidation::cleanupTestCase()
{
    globalParams.reset();
}

void CheckSignatureCrossValidation::testKeyList()
{
    // main thing here is to test we have found our test data
    auto backend = CryptoSign::Factory::createActive();
    QVERIFY(backend);

    auto certificateList = backend->getAvailableSigningCertificates();
    QCOMPARE(certificateList.size(), 1);

    QCOMPARE(certificateList[0]->getNickName().toStr(), keyId);
    QCOMPARE(certificateList[0]->getSubjectInfo().commonName, "Alice Bob");
}

static void switchBackend()
{
    qWarning() << "switching backend; was " << static_cast<int>(CryptoSign::Factory::getActive().value());
    auto activeBackendType = CryptoSign::Factory::getActive();
    if (activeBackendType == CryptoSign::Backend::Type::NSS3) {
        CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::GPGME);
        QCOMPARE(CryptoSign::Factory::getActive(), CryptoSign::Backend::Type::GPGME);
    } else if (activeBackendType == CryptoSign::Backend::Type::GPGME) {
        CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::NSS3);
        QCOMPARE(CryptoSign::Factory::getActive(), CryptoSign::Backend::Type::NSS3);
    } else {
        QFAIL("Unexpected backend status");
    }
    qWarning() << "now backend is " << static_cast<int>(CryptoSign::Factory::getActive().value());
}

void CheckSignatureCrossValidation::testSignVerify()
{
    auto doc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY(doc->isOk());
    {
        auto signatureFields = doc->getSignatureFields();
        QCOMPARE(signatureFields.size(), 0);
    }

    QTemporaryDir d;

    auto signingResult = doc->sign(std::string { d.filePath(QStringLiteral("signedFile.pdf")).toStdString() }, keyId, std::string {}, std::make_unique<GooString>("newSignatureFieldName"), /*page*/ 1,
                                   /*rect */ { 0, 0, 0, 0 }, /*signatureText*/ {}, /*signatureTextLeft*/ {}, /*fontSize */ 0, /*leftFontSize*/ 0,
                                   /*fontColor*/ {}, /*borderWidth*/ 0, /*borderColor*/ {}, /*backgroundColor*/ {}, /*reason*/ {}, /* location */ nullptr, /* image path */ "", {}, {});

    QVERIFY(!signingResult.has_value());

    switchBackend();

    // validate
    auto signedDoc = std::make_unique<PDFDoc>(std::make_unique<GooString>(d.filePath(QStringLiteral("signedFile.pdf")).toStdString()));
    QVERIFY(signedDoc->isOk());
    auto signatureFields = signedDoc->getSignatureFields();
    QCOMPARE(signatureFields.size(), 1);
    QCOMPARE(signatureFields[0]->getSignatureType(), CryptoSign::SignatureType::adbe_pkcs7_detached);
    auto *siginfo0 = signatureFields[0]->validateSignatureAsync(false, false, -1 /* now */, false, false, {});
    signatureFields[0]->validateSignatureResult();
    QCOMPARE(siginfo0->getSignatureValStatus(), SignatureValidationStatus::SIGNATURE_VALID);
    QCOMPARE(siginfo0->getSignerName(), std::string { "Alice Bob" });
}

QTEST_GUILESS_MAIN(CheckSignatureCrossValidation)
#include "check_signature_cross_validation.moc"
