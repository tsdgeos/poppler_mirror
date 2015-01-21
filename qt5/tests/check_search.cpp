#include <QtTest/QtTest>

#include <poppler-qt5.h>

class TestSearch: public QObject
{
    Q_OBJECT
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

    QCOMPARE( page->search(QString("non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );

    QCOMPARE( page->search(QString("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), false );
    QCOMPARE( page->search(QString("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseInsensitive), true );

    QCOMPARE( page->search(QString("latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), false );

    QCOMPARE( page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QCOMPARE( page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QCOMPARE( page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QCOMPARE( page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QCOMPARE( page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QCOMPARE( page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), false );

    QCOMPARE( page->search(QString("non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );

    QCOMPARE( page->search(QString("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
    QCOMPARE( page->search(QString("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true );

    QCOMPARE( page->search(QString("latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QCOMPARE( page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
}

void TestSearch::testNextAndPrevious()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/xr01.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult, Poppler::Page::CaseSensitive), false );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult, Poppler::Page::CaseSensitive), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult, Poppler::Page::CaseSensitive), false );

    rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QString("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false );
}

void TestSearch::testWholeWordsOnly()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = 0;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode3 = Poppler::Page::IgnoreCase | Poppler::Page::WholeWords;

    double left, top, right, bottom;

    QCOMPARE( page->search(QLatin1String("brown"), left, top, right, bottom, direction, mode0), true );
    QCOMPARE( page->search(QLatin1String("brOwn"), left, top, right, bottom, direction, mode0), false );

    QCOMPARE( page->search(QLatin1String("brOwn"), left, top, right, bottom, direction, mode1), true );
    QCOMPARE( page->search(QLatin1String("brawn"), left, top, right, bottom, direction, mode1), false );

    QCOMPARE( page->search(QLatin1String("brown"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QLatin1String("own"), left, top, right, bottom, direction, mode2), false );

    QCOMPARE( page->search(QLatin1String("brOwn"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QLatin1String("Own"), left, top, right, bottom, direction, mode3), false );
}

QTEST_MAIN(TestSearch)
#include "check_search.moc"

