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
    void testIgnoreDiacritics();
    void testRussianSearch(); // Issue #743
    void testDeseretSearch(); // Issue #853
};

void TestSearch::bug7063()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/bug7063.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral("non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );

    QCOMPARE( page->search(QStringLiteral("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
    QCOMPARE( page->search(QStringLiteral("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true );

    QCOMPARE( page->search(QStringLiteral("latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false ); //clazy:exclude=qstring-allocations

    QCOMPARE( page->search(QStringLiteral("non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );

    QCOMPARE( page->search(QStringLiteral("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );
    QCOMPARE( page->search(QStringLiteral("Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true );

    QCOMPARE( page->search(QStringLiteral("latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false );

    QCOMPARE( page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false ); //clazy:exclude=qstring-allocations
}

void TestSearch::testNextAndPrevious()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/xr01.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false );

    rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 139.81) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 171.46) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true );
    QVERIFY( qAbs(rectLeft - 161.44) < 0.01 );
    QVERIFY( qAbs(rectTop - 127.85) < 0.01 );
    QVERIFY( qAbs(rectRight - rectLeft - 6.70) < 0.01 );
    QVERIFY( qAbs(rectBottom - rectTop - 8.85) < 0.01 );
    QCOMPARE( page->search(QStringLiteral("is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false );
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

    QCOMPARE( page->search(QStringLiteral("brown"), left, top, right, bottom, direction, mode0), true );
    QCOMPARE( page->search(QStringLiteral("brOwn"), left, top, right, bottom, direction, mode0), false );

    QCOMPARE( page->search(QStringLiteral("brOwn"), left, top, right, bottom, direction, mode1), true );
    QCOMPARE( page->search(QStringLiteral("brawn"), left, top, right, bottom, direction, mode1), false );

    QCOMPARE( page->search(QStringLiteral("brown"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("own"), left, top, right, bottom, direction, mode2), false );

    QCOMPARE( page->search(QStringLiteral("brOwn"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QStringLiteral("Own"), left, top, right, bottom, direction, mode3), false );
}

void TestSearch::testIgnoreDiacritics()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/Issue637.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = nullptr;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreDiacritics;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode3 = Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase | Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode4 = Poppler::Page::IgnoreCase | Poppler::Page::WholeWords;

    double left, top, right, bottom;

    // Test pdf (Issue637.pdf) just contains the following three lines:
    // La cigüeña voló sobre nuestras cabezas.
    // La cigogne a survolé nos têtes.
    // Der Storch flog über unsere Köpfe hinweg.

    QCOMPARE( page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode0), false );
    QCOMPARE( page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode1), false );
    QCOMPARE( page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode1), true );
    QCOMPARE( page->search(QString::fromUtf8("cigüeña"), left, top, right, bottom, direction, mode1), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("cigüena"), left, top, right, bottom, direction, mode1), false ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("Cigüeña"), left, top, right, bottom, direction, mode1), false ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode3), true );

    QCOMPARE( page->search(QString::fromUtf8("cigüeña"), left, top, right, bottom, direction, mode4), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("Cigüeña"), left, top, right, bottom, direction, mode4), true ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QString::fromUtf8("cigüena"), left, top, right, bottom, direction, mode4), false ); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode4), false );

    QCOMPARE( page->search(QStringLiteral("kopfe"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("kopfe"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode0), false );
    QCOMPARE( page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode1), true );
    QCOMPARE( page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode3), true );

    QCOMPARE( page->search(QStringLiteral("vole"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("vole"), left, top, right, bottom, direction, mode3), false );
    QCOMPARE( page->search(QStringLiteral("survole"), left, top, right, bottom, direction, mode3), true );
    QCOMPARE( page->search(QStringLiteral("tete"), left, top, right, bottom, direction, mode3), false );
    QCOMPARE( page->search(QStringLiteral("tete"), left, top, right, bottom, direction, mode2), true );

    QCOMPARE( page->search(QStringLiteral("La Ciguena Volo"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("Survole Nos Tetes"), left, top, right, bottom, direction, mode2), true );
    QCOMPARE( page->search(QStringLiteral("Uber Unsere Kopfe"), left, top, right, bottom, direction, mode2), true );
}

void TestSearch::testRussianSearch()
{
    // Test for issue #743
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/russian.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = Poppler::Page::NoSearchFlags;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreDiacritics;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode0W = mode0 | Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode1W = mode1 | Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode2W = mode2 | Poppler::Page::WholeWords;

    double l, t, r, b; //left, top, right, bottom

    // In the searched page 5, these two words do exist: простой and Простой
    const QString str = QString::fromUtf8("простой"); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(str, l, t, r, b, direction, mode0), true );
    QCOMPARE( page->search(str, l, t, r, b, direction, mode1), true );
    QCOMPARE( page->search(str, l, t, r, b, direction, mode2), true );
    QCOMPARE( page->search(str, l, t, r, b, direction, mode0W), true );
    QCOMPARE( page->search(str, l, t, r, b, direction, mode1W), true );
    QCOMPARE( page->search(str, l, t, r, b, direction, mode2W), true );
}

void TestSearch::testDeseretSearch()
{
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/deseret.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    double l, t, r, b; //left, top, right, bottom

    const QString str = QString::fromUtf8("𐐐𐐯𐑊𐐬"); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(str, l, t, r, b, Poppler::Page::FromTop, Poppler::Page::NoSearchFlags), true );

    const QString str2 = QString::fromUtf8("𐐸𐐯𐑊𐐬"); //clazy:exclude=qstring-allocations
    QCOMPARE( page->search(str2, l, t, r, b, Poppler::Page::FromTop,  Poppler::Page::IgnoreCase), true );
}

QTEST_GUILESS_MAIN(TestSearch)
#include "check_search.moc"

