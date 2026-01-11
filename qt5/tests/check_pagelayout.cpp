#include <QtTest/QTest>

#include <poppler-qt5.h>

class TestPageLayout : public QObject
{
    Q_OBJECT
public:
    explicit TestPageLayout(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkNone();
    static void checkSingle();
    static void checkFacing();
};

void TestPageLayout::checkNone()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/UseNone.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::NoLayout);

    delete doc;
}

void TestPageLayout::checkSingle()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/FullScreen.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::SinglePage);

    delete doc;
}

void TestPageLayout::checkFacing()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/doublepage.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::TwoPageRight);

    delete doc;
}

QTEST_GUILESS_MAIN(TestPageLayout)
#include "check_pagelayout.moc"
