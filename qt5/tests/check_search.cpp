#include <QtTest/QtTest>

#include <poppler-qt5.h>

class TestSearch: public QObject
{
    Q_OBJECT
public:
    TestSearch(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void bug7063();
    void testNextAndPrevious();
    void testWholeWordsOnly();
};

void TestSearch::bug7063()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/bug7063.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral(u"non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );

    QCOMPARE( page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
    QCOMPARE( page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true );

    QCOMPARE( page->search(QStringLiteral(u"latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QStringLiteral(u"é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QStringLiteral(u"non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );

    QCOMPARE( page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
    QCOMPARE( page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true );

    QCOMPARE( page->search(QStringLiteral(u"latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QStringLiteral(u"é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QStringLiteral(u"¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
}

void TestSearch::testNextAndPrevious()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/xr01.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false );

    rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false );
}

void TestSearch::testWholeWordsOnly()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = nullptr;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode3 = Poppler::Page::IgnoreCase | Poppler::Page::WholeWords;

    double left, top, right, bottom;

    QCOMPARE( page->search(QStringLiteral(u"brown"), left, top, right, bottom, direction, mode0), true );
    QCOMPARE( page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode0), false );

    QCOMPARE( page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode1), true );
    QCOMPARE( page->search(QStringLiteral(u"brawn"), left, top, right, bottom, direction, mode1), false );

    QCOMPARE( page->search(QStringLiteral(u"brown"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral(u"own"), left, top, right, bottom, direction, mode2), false );

    QCOMPARE( page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QStringLiteral(u"Own"), left, top, right, bottom, direction, mode3), false );
}

QTEST_GUILESS_MAIN(TestSearch)
#include "check_search.moc"

