
//========================================================================
//
// check_untrusted_signature_keylist.cpp
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

class CheckUntrustedSignatureKeylist : public QObject
{
    Q_OBJECT
public:
    explicit CheckUntrustedSignatureKeylist(QObject *parent = nullptr) : QObject(parent) { }

    static std::unique_ptr<QTemporaryDir> gpgdir;
    static void initMain()
    {
        gpgdir = std::make_unique<QTemporaryDir>();
        // copy out the data for two reasons:
        // 1) gpg-agent might get angry if the path is too long
        // 2) Ensure that accidental writes from the test (and e
        //    specially other tests getting inspired by this)
        //    does not carry over to the next tests

        // take the keys from cross validation
        std::filesystem::copy(TESTDATADIR "/unittestcases/signature-cross-validation/gnupghome", gpgdir->path().toStdString(), std::filesystem::copy_options::recursive);
        // copy in teh untrusted-marking from untrusted signature
        std::filesystem::copy(TESTDATADIR "/unittestcases/untrusted-signature/gnupghome/trustlist.txt", gpgdir->path().toStdString(), std::filesystem::copy_options::overwrite_existing);
        qputenv("GNUPGHOME", gpgdir->path().toLocal8Bit());
        qDebug() << gpgdir->path();
    }

private Q_SLOTS:
    static void testUntrustedKeylist();
};

std::unique_ptr<QTemporaryDir> CheckUntrustedSignatureKeylist::gpgdir;

void CheckUntrustedSignatureKeylist::testUntrustedKeylist()
{
    const auto availableBackends = CryptoSign::Factory::getAvailable();

    if (std::ranges::find(availableBackends, CryptoSign::Backend::Type::GPGME) != availableBackends.end()) {
        CryptoSign::Factory::setPreferredBackend(CryptoSign::Backend::Type::GPGME);
        QCOMPARE(CryptoSign::Factory::getActive(), CryptoSign::Backend::Type::GPGME);
    } else {
        QFAIL("Compiled with GPGME, but GPGME not functional");
    }

    auto backend = CryptoSign::Factory::createActive();
    QVERIFY(backend);

    auto certificateList = backend->getAvailableSigningCertificates();
    QCOMPARE(certificateList.size(), 0);
}

QTEST_GUILESS_MAIN(CheckUntrustedSignatureKeylist)
#include "check_untrusted_signature_keylist.moc"
