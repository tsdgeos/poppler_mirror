#include <QtTest/QTest>

#include <poppler-qt6.h>

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
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseNone.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseNone);
}

void TestPageMode::checkFullScreen()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/FullScreen.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::FullScreen);
}

void TestPageMode::checkAttachments()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseAttachments.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseAttach);
}

void TestPageMode::checkThumbs()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseThumbs.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseThumbs);
}

void TestPageMode::checkOC()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseOC.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageMode(), Poppler::Document::UseOC);
}

QTEST_GUILESS_MAIN(TestPageMode)
#include "check_pagemode.moc"
