#include <QtTest/QTest>

#include <poppler-qt6.h>

class TestPageLayout : public QObject
{
    Q_OBJECT
public:
    explicit TestPageLayout(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkNone();
    void checkSingle();
    void checkFacing();
};

void TestPageLayout::checkNone()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/UseNone.pdf");
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::NoLayout);
}

void TestPageLayout::checkSingle()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/FullScreen.pdf");
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::SinglePage);
}

void TestPageLayout::checkFacing()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/doublepage.pdf");
    QVERIFY(doc);

    QCOMPARE(doc->pageLayout(), Poppler::Document::TwoPageRight);
}

QTEST_GUILESS_MAIN(TestPageLayout)
#include "check_pagelayout.moc"
