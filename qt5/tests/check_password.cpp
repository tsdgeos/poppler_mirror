#include <QtTest/QTest>

#include <poppler-qt5.h>

class TestPassword : public QObject
{
    Q_OBJECT
public:
    explicit TestPassword(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void password1();
    static void password1a();
    static void password2();
    static void password2a();
    static void password2b();
    static void password3();
    static void password4();
    static void password4b();
    static void password5();
};

// BUG:4557
void TestPassword::password1()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - open.pdf"), "", QString::fromUtf8("garçon").toLatin1()); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password1a()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - open.pdf")); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", QString::fromUtf8("garçon").toLatin1())); // clazy:exclude=qstring-allocations
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password2()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"), QString::fromUtf8("garçon").toLatin1(), ""); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password2a()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"), QString::fromUtf8("garçon").toLatin1()); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password2b()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"));
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());
    QVERIFY(!doc->unlock(QString::fromUtf8("garçon").toLatin1(), "")); // clazy:exclude=qstring-allocations
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password3()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/PasswordEncrypted.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "password"));
    QVERIFY(!doc->isLocked());

    delete doc;
}

// issue 690
void TestPassword::password4()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/encrypted-256.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("owner-secret", ""));
    QVERIFY(!doc->isLocked());

    delete doc;
}

// issue 690
void TestPassword::password4b()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/encrypted-256.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "user-secret"));
    QVERIFY(!doc->isLocked());

    delete doc;
}

void TestPassword::password5()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/PasswordEncryptedReconstructed.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "test"));
    QVERIFY(!doc->isLocked());

    delete doc;
}

QTEST_GUILESS_MAIN(TestPassword)
#include "check_password.moc"
