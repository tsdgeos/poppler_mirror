
//========================================================================
//
// check_untrusted_signature.cpp
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
#include "poppler-form.h"

class CheckUntrustedSignature : public QObject
{
    Q_OBJECT
public:
    explicit CheckUntrustedSignature(QObject *parent = nullptr) : QObject(parent) { }

    static std::unique_ptr<QTemporaryDir> gpgdir;
    static void initMain()
    {
        gpgdir = std::make_unique<QTemporaryDir>();
        // copy out the data for two reasons:
        // 1) gpg-agent might get angry if the path is too long
        // 2) Ensure that accidental writes from the test (and e
        //    specially other tests getting inspired by this)
        //    does not carry over to the next tests
        std::filesystem::copy(TESTDATADIR "/unittestcases/untrusted-signature/gnupghome", gpgdir->path().toStdString(), std::filesystem::copy_options::recursive);
        qputenv("GNUPGHOME", gpgdir->path().toLocal8Bit());
        qDebug() << gpgdir->path();
    }

private Q_SLOTS:
    static void testUntrusted();
};

std::unique_ptr<QTemporaryDir> CheckUntrustedSignature::gpgdir;

void CheckUntrustedSignature::testUntrusted()
{
    const auto availableBackends = CryptoSign::Factory::getAvailable();

    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
        CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::GPGME);
        QCOMPARE(CryptoSign::Factory::getActive(), CryptoSign::Backend::Type::GPGME);
    } else {
        QFAIL("Compiled with GPGME, but GPGME not functional");
    }

    globalParams = std::make_unique<GlobalParams>();
    // validate
    auto signedDoc = std::make_unique<PDFDoc>(std::make_unique<GooString>(TESTDATADIR "/unittestcases/simpleSignedFile.pdf"));
    QVERIFY(signedDoc->isOk());
    auto signatureFields = signedDoc->getSignatureFields();
    QCOMPARE(signatureFields.size(), 1);
    QCOMPARE(signatureFields[0]->getSignatureType(), CryptoSign::SignatureType::adbe_pkcs7_detached);
    auto *siginfo0 = signatureFields[0]->validateSignatureAsync(true, false, -1 /* now */, true, true, {});
    auto trustState = signatureFields[0]->validateSignatureResult();
    QCOMPARE(siginfo0->getSignatureValStatus(), SignatureValidationStatus::SIGNATURE_VALID);
    QCOMPARE(siginfo0->getSignerName(), std::string { "Alice Bob" });
    QCOMPARE(trustState, CertificateValidationStatus::CERTIFICATE_UNTRUSTED_ISSUER);
    globalParams.reset();
}

QTEST_GUILESS_MAIN(CheckUntrustedSignature)
#include "check_untrusted_signature.moc"
