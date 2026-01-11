#include <QtTest/QTest>

#include <poppler-qt5.h>

class TestPageMode : public QObject
{
    Q_OBJECT
public:
    explicit TestPageMode(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkNone();
    static void checkFullScreen();
    static void checkAttachments();
    static void checkThumbs();
    static void checkOC();
};

void TestPageMode::checkNone()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QString::fromLocal8Bit(TESTDATADIR "/unittestcases/UseNone.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseNone);

    delete doc;
}

void TestPageMode::checkFullScreen()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/FullScreen.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::FullScreen);

    delete doc;
}

void TestPageMode::checkAttachments()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseAttachments.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseAttach);

    delete doc;
}

void TestPageMode::checkThumbs()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseThumbs.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseThumbs);

    delete doc;
}

void TestPageMode::checkOC()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseOC.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseOC);

    delete doc;
}

QTEST_GUILESS_MAIN(TestPageMode)
#include "check_pagemode.moc"
