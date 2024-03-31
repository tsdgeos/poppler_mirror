#include <QtTest/QTest>

#include <poppler-qt6.h>

class TestPassword : public QObject
{
    Q_OBJECT
public:
    explicit TestPassword(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void password1();
    void password1a();
    void password2();
    void password2a();
    void password2b();
    void password3();
    void password4();
    void password4b();
    void password5();
};

// BUG:4557
void TestPassword::password1()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - open.pdf"), "", QString::fromUtf8("garçon").toLatin1()); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());
}

void TestPassword::password1a()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - open.pdf")); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", QString::fromUtf8("garçon").toLatin1())); // clazy:exclude=qstring-allocations
    QVERIFY(!doc->isLocked());
}

void TestPassword::password2()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"), QString::fromUtf8("garçon").toLatin1(), ""); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());
}

void TestPassword::password2a()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"), QString::fromUtf8("garçon").toLatin1()); // clazy:exclude=qstring-allocations
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());
}

void TestPassword::password2b()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/Gday garçon - owner.pdf"));
    QVERIFY(doc);
    QVERIFY(!doc->isLocked());
    QVERIFY(!doc->unlock(QString::fromUtf8("garçon").toLatin1(), "")); // clazy:exclude=qstring-allocations
    QVERIFY(!doc->isLocked());
}

void TestPassword::password3()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/PasswordEncrypted.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "password"));
    QVERIFY(!doc->isLocked());
}

// issue 690
void TestPassword::password4()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/encrypted-256.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("owner-secret", ""));
    QVERIFY(!doc->isLocked());
}

// issue 690
void TestPassword::password4b()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/encrypted-256.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "user-secret"));
    QVERIFY(!doc->isLocked());
}

void TestPassword::password5()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromUtf8(TESTDATADIR "/unittestcases/PasswordEncryptedReconstructed.pdf"));
    QVERIFY(doc);
    QVERIFY(doc->isLocked());
    QVERIFY(!doc->unlock("", "test"));
    QVERIFY(!doc->isLocked());
}

QTEST_GUILESS_MAIN(TestPassword)
#include "check_password.moc"
