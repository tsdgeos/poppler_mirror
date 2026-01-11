#include <QtTest/QTest>

#include <poppler-qt5.h>

// clazy:excludeall=qstring-allocations

class TestSearch : public QObject
{
    Q_OBJECT
public:
    explicit TestSearch(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void testAcrossLinesSearch(); // leave it first
    static void testAcrossLinesSearchDoubleColumn();
    static void bug7063();
    static void testNextAndPrevious();
    static void testWholeWordsOnly();
    static void testIgnoreDiacritics();
    static void testRussianSearch(); // Issue #743
    static void testDeseretSearch(); // Issue #853
};

void TestSearch::bug7063()
{
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/bug7063.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE(page->search(QStringLiteral(u"non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);

    QCOMPARE(page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);
    QCOMPARE(page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true);

    QCOMPARE(page->search(QStringLiteral(u"latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);

    QCOMPARE(page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);

    QCOMPARE(page->search(QStringLiteral(u"non-ascii:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);

    QCOMPARE(page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);
    QCOMPARE(page->search(QStringLiteral(u"Ascii"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true);

    QCOMPARE(page->search(QStringLiteral(u"latin1:"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);

    QCOMPARE(page->search(QString::fromUtf8("é"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("à"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("ç"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("search \"é\", \"à\" or \"ç\""), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("¥µ©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QCOMPARE(page->search(QString::fromUtf8("¥©"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), false);
}

void TestSearch::testNextAndPrevious()
{
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/xr01.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    double rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false);

    rectLeft = 0.0, rectTop = 0.0, rectRight = page->pageSizeF().width(), rectBottom = page->pageSizeF().height();

    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::FromTop), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::NextResult), false);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 139.81) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 171.46) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), true);
    QVERIFY(qAbs(rectLeft - 161.44) < 0.01);
    QVERIFY(qAbs(rectTop - 127.85) < 0.01);
    QVERIFY(qAbs(rectRight - rectLeft - 6.70) < 0.01);
    QVERIFY(qAbs(rectBottom - rectTop - 8.85) < 0.01);
    QCOMPARE(page->search(QStringLiteral(u"is"), rectLeft, rectTop, rectRight, rectBottom, Poppler::Page::PreviousResult), false);
}

void TestSearch::testWholeWordsOnly()
{
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/WithActualText.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = nullptr;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode3 = Poppler::Page::IgnoreCase | Poppler::Page::WholeWords;

    double left, top, right, bottom;

    QCOMPARE(page->search(QStringLiteral(u"brown"), left, top, right, bottom, direction, mode0), true);
    QCOMPARE(page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode0), false);

    QCOMPARE(page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode1), true);
    QCOMPARE(page->search(QStringLiteral(u"brawn"), left, top, right, bottom, direction, mode1), false);

    QCOMPARE(page->search(QStringLiteral(u"brown"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral(u"own"), left, top, right, bottom, direction, mode2), false);

    QCOMPARE(page->search(QStringLiteral(u"brOwn"), left, top, right, bottom, direction, mode3), true);
    QCOMPARE(page->search(QStringLiteral(u"Own"), left, top, right, bottom, direction, mode3), false);
}

void TestSearch::testIgnoreDiacritics()
{
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/Issue637.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

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

    QCOMPARE(page->search(QString(), left, top, right, bottom, direction, mode0), false);
    QCOMPARE(page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode0), false);
    QCOMPARE(page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode1), false);
    QCOMPARE(page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode1), true);
    QCOMPARE(page->search(QString::fromUtf8("cigüeña"), left, top, right, bottom, direction, mode1), true);
    QCOMPARE(page->search(QString::fromUtf8("cigüena"), left, top, right, bottom, direction, mode1), false);
    QCOMPARE(page->search(QString::fromUtf8("Cigüeña"), left, top, right, bottom, direction, mode1), false);
    QCOMPARE(page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode3), true);
    QCOMPARE(page->search(QStringLiteral("ciguena"), left, top, right, bottom, direction, mode3), true);

    QCOMPARE(page->search(QString::fromUtf8("cigüeña"), left, top, right, bottom, direction, mode4), true);
    QCOMPARE(page->search(QString::fromUtf8("Cigüeña"), left, top, right, bottom, direction, mode4), true);
    QCOMPARE(page->search(QString::fromUtf8("cigüena"), left, top, right, bottom, direction, mode4), false);
    QCOMPARE(page->search(QStringLiteral("Ciguena"), left, top, right, bottom, direction, mode4), false);

    QCOMPARE(page->search(QStringLiteral("kopfe"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("kopfe"), left, top, right, bottom, direction, mode3), true);
    QCOMPARE(page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode0), false);
    QCOMPARE(page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode1), true);
    QCOMPARE(page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("uber"), left, top, right, bottom, direction, mode3), true);

    QCOMPARE(page->search(QStringLiteral("vole"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("vole"), left, top, right, bottom, direction, mode3), false);
    QCOMPARE(page->search(QStringLiteral("survole"), left, top, right, bottom, direction, mode3), true);
    QCOMPARE(page->search(QStringLiteral("tete"), left, top, right, bottom, direction, mode3), false);
    QCOMPARE(page->search(QStringLiteral("tete"), left, top, right, bottom, direction, mode2), true);

    QCOMPARE(page->search(QStringLiteral("La Ciguena Volo"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("Survole Nos Tetes"), left, top, right, bottom, direction, mode2), true);
    QCOMPARE(page->search(QStringLiteral("Uber Unsere Kopfe"), left, top, right, bottom, direction, mode2), true);
}

void TestSearch::testRussianSearch()
{
    // Test for issue #743
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/russian.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags mode0 = Poppler::Page::NoSearchFlags;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::IgnoreDiacritics;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode0W = mode0 | Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode1W = mode1 | Poppler::Page::WholeWords;
    const Poppler::Page::SearchFlags mode2W = mode2 | Poppler::Page::WholeWords;

    double l, t, r, b; // left, top, right, bottom

    // In the searched page 5, these two words do exist: простой and Простой
    const QString str = QString::fromUtf8("простой");
    QCOMPARE(page->search(str, l, t, r, b, direction, mode0), true);
    QCOMPARE(page->search(str, l, t, r, b, direction, mode1), true);
    QCOMPARE(page->search(str, l, t, r, b, direction, mode2), true);
    QCOMPARE(page->search(str, l, t, r, b, direction, mode0W), true);
    QCOMPARE(page->search(str, l, t, r, b, direction, mode1W), true);
    QCOMPARE(page->search(str, l, t, r, b, direction, mode2W), true);
}

void TestSearch::testDeseretSearch()
{
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/deseret.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    double l, t, r, b; // left, top, right, bottom

    const QString str = QString::fromUtf8("𐐐𐐯𐑊𐐬");
    QCOMPARE(page->search(str, l, t, r, b, Poppler::Page::FromTop, Poppler::Page::NoSearchFlags), true);

    const QString str2 = QString::fromUtf8("𐐸𐐯𐑊𐐬");
    QCOMPARE(page->search(str2, l, t, r, b, Poppler::Page::FromTop, Poppler::Page::IgnoreCase), true);
}

void TestSearch::testAcrossLinesSearch()
{
    // Test for searching across lines with new flag Poppler::Page::AcrossLines
    // and its automatic features like ignoring hyphen at end of line or allowing
    // whitespace in the search term to match on newline character.
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/searchAcrossLines.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(1));
    QVERIFY(page);

    const Poppler::Page::SearchDirection direction = Poppler::Page::FromTop;

    const Poppler::Page::SearchFlags empty = Poppler::Page::NoSearchFlags;
    const Poppler::Page::SearchFlags mode0 = Poppler::Page::AcrossLines;
    const Poppler::Page::SearchFlags mode1 = Poppler::Page::AcrossLines | Poppler::Page::IgnoreDiacritics;
    const Poppler::Page::SearchFlags mode2 = Poppler::Page::AcrossLines | Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase;
    const Poppler::Page::SearchFlags mode2W = mode2 | Poppler::Page::WholeWords;

    double l, t, r, b; // left, top, right, bottom

    // In the searched page, each of "re-conocimiento" "PRUE-BA" "imáge-nes" happen split across lines
    const QString str1 = QString::fromUtf8("reconocimiento");
    const QString str2 = QString::fromUtf8("IMagenes");
    // Test it cannot be found with empty search flags
    QCOMPARE(page->search(str1, l, t, r, b, direction, empty), false);
    // Test it is found with AcrossLines option
    QCOMPARE(page->search(str1, l, t, r, b, direction, mode0), true);
    // Test AcrossLines with IgnoreDiacritics and IgnoreCase options
    QCOMPARE(page->search(str2, l, t, r, b, direction, mode0), false);
    QCOMPARE(page->search(str2, l, t, r, b, direction, mode1), false);
    QCOMPARE(page->search(str2, l, t, r, b, direction, mode2), true);
    // Test with WholeWords too
    QCOMPARE(page->search(str2, l, t, r, b, direction, mode2W), true);

    // Now test that AcrossLines also allows whitespace in the search term to match on newline char.
    // In the searched page, "podrá" ends a line and "acordar" starts the next line, so we
    // now test we match it with "podrá acordar"
    const QString str3 = QString::fromUtf8("podrá acordar,");
    QCOMPARE(page->search(str3, l, t, r, b, direction, mode0), true);
    QCOMPARE(page->search(str3, l, t, r, b, direction, mode1), true);
    QCOMPARE(page->search(str3, l, t, r, b, direction, mode2), true);
    QCOMPARE(page->search(str3, l, t, r, b, direction, mode2W), true);
    // now test it also works with IgnoreDiacritics and IgnoreCase
    const QString str4 = QString::fromUtf8("PODRA acordar");
    QCOMPARE(page->search(str4, l, t, r, b, direction, mode0), false);
    QCOMPARE(page->search(str4, l, t, r, b, direction, mode1), false);
    QCOMPARE(page->search(str4, l, t, r, b, direction, mode2), true);
    QCOMPARE(page->search(str4, l, t, r, b, direction, mode2W), false); // false as it lacks ending comma

    // Now test that when a hyphen char in the search term matches a hyphen at end of line,
    // then we don't automatically ignore it, but treat it as a normal char.
    // In the searched page, "CC BY-NC-SA 4.0" is split across two lines on the second hyphen
    const QString str5 = QString::fromUtf8("CC BY-NC-SA 4.0");
    QScopedPointer<Poppler::Page> page0(document->page(0));
    QVERIFY(page0);
    QCOMPARE(page0->search(str5, l, t, r, b, direction, mode0), true);
    QCOMPARE(page0->search(str5, l, t, r, b, direction, mode1), true);
    QCOMPARE(page0->search(str5, l, t, r, b, direction, mode2), true);
    QCOMPARE(page0->search(str5, l, t, r, b, direction, mode2W), true);
    QCOMPARE(page0->search(QString::fromUtf8("NC-SA"), l, t, r, b, direction, mode2W), false);
    // Searching for "CC BY-NCSA 4.0" should also match, because hyphen is now ignored at end of line
    const QString str6 = QString::fromUtf8("CC BY-NCSA 4.0");
    QCOMPARE(page0->search(str6, l, t, r, b, direction, mode0), true);
    QCOMPARE(page0->search(str6, l, t, r, b, direction, mode1), true);
    QCOMPARE(page0->search(str6, l, t, r, b, direction, mode2), true);
    QCOMPARE(page0->search(str6, l, t, r, b, direction, mode2W), true);
    // Check for the case when next line falls in next paragraph. Issue #1475
    const QString across_block = QString::fromUtf8("emacs jose"); // clazy:exclude=qstring-allocations
    QCOMPARE(page0->search(across_block, l, t, r, b, direction, empty), false);
    QCOMPARE(page0->search(across_block, l, t, r, b, direction, mode0), false);
    QCOMPARE(page0->search(across_block, l, t, r, b, direction, mode1), false);
    QCOMPARE(page0->search(across_block, l, t, r, b, direction, mode2), true);
    QCOMPARE(page0->search(across_block, l, t, r, b, direction, mode2W), true);

    // Now for completeness, we will match the full text of two lines
    const QString full2lines = QString::fromUtf8("Las pruebas se practicarán en vista pública, si bien, excepcionalmente, el Tribunal podrá acordar, mediante providencia, que determinadas pruebas se celebren fuera del acto de juicio");
    QCOMPARE(page->search(full2lines, l, t, r, b, direction, mode0), true);
    QCOMPARE(page->search(full2lines, l, t, r, b, direction, mode1), true);
    QCOMPARE(page->search(full2lines, l, t, r, b, direction, mode2), true);
    QCOMPARE(page->search(full2lines, l, t, r, b, direction, mode2W), true);
    // And now the full text of two lines split by a hyphenated word
    const QString full2linesHyphenated = QString::fromUtf8("Consiste básicamente en información digitalizada, codificados y alojados en un elemento contenedor digital (equipos, dispositivos periféricos, unidades de memoria, unidades "
                                                           "virtualizadas, tramas");
    QCOMPARE(page->search(full2linesHyphenated, l, t, r, b, direction, mode0), true);
    QCOMPARE(page->search(full2linesHyphenated, l, t, r, b, direction, mode1), true);
    QCOMPARE(page->search(full2linesHyphenated, l, t, r, b, direction, mode2), true);
    QCOMPARE(page->search(full2linesHyphenated, l, t, r, b, direction, mode2W), true);

    // BUG about false positives at start of a line.
    const QString bug_str = QString::fromUtf8("nes y"); // clazy:exclude=qstring-allocations
    // there's only 1 match, check for that
    QCOMPARE(page->search(bug_str, mode2).size(), 1);
}

void TestSearch::testAcrossLinesSearchDoubleColumn()
{
    // Test for searching across lines with new flag Poppler::Page::AcrossLines
    // in a document with two columns of text.
    QScopedPointer<Poppler::Document> document(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/searchAcrossLinesDoubleColumn.pdf")));
    QVERIFY(document);

    QScopedPointer<Poppler::Page> page(document->page(0));
    QVERIFY(page);

    const Poppler::Page::SearchFlags mode = Poppler::Page::AcrossLines | Poppler::Page::IgnoreDiacritics | Poppler::Page::IgnoreCase;

    // Test for a bug in double column documents where single line matches are
    // wrongly returned as being multiline matches.
    const QString bug_str = QString::fromUtf8("betw"); // clazy:exclude=qstring-allocations

    // there's only 3 matches for 'betw' in document, where only the last
    // one is a multiline match, so that's a total of 4 rects returned
    QCOMPARE(page->search(bug_str, mode).size(), 4);
}

QTEST_GUILESS_MAIN(TestSearch)
#include "check_search.moc"
