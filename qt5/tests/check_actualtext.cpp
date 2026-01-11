#include <QtTest/QTest>

#include <poppler-qt5.h>

#include <QtCore/QFile>

class TestActualText : public QObject
{
    Q_OBJECT
public:
    explicit TestActualText(QObject *parent = nullptr) : QObject(parent) { }

    static void checkActualText(Poppler::Document *doc, const QRectF &area, const QString &text);

private Q_SLOTS:
    static void checkActualText1();
    static void checkActualText2();
    static void checkActualText2_data();
    static void checkAllOrientations();
    static void checkAllOrientations_data();
    static void checkFakeboldText();
    static void checkFakeboldText_data();
};

void TestActualText::checkActualText(Poppler::Document *doc, const QRectF &area, const QString &text)
{
    Poppler::Page *page = doc->page(0);
    QVERIFY(page);

    QCOMPARE(page->text(area), text);

    delete page;
}

void TestActualText::checkActualText1()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY(doc);

    checkActualText(doc, QRectF {}, QStringLiteral("The slow brown fox jumps over the black dog."));

    delete doc;
}

void TestActualText::checkActualText2()
{
    QFETCH(QRectF, area);
    QFETCH(QString, text);

    QFile file(QStringLiteral(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    Poppler::Document *doc;
    doc = Poppler::Document::load(&file);
    QVERIFY(doc);

    checkActualText(doc, area, text);

    delete doc;
}

void TestActualText::checkActualText2_data()
{
    QTest::addColumn<QRectF>("area");
    QTest::addColumn<QString>("text");

    // Line bounding box is [100.000 90.720 331.012110 102.350]

    QTest::newRow("full page") << QRectF {} << QStringLiteral("The slow brown fox jumps over the black dog.");
    QTest::newRow("full line") << QRectF { 50.0, 90.0, 290.0, 20.0 } << QStringLiteral("The slow brown fox jumps over the black dog.");
    QTest::newRow("full line [narrow]") << QRectF { 50.0, 95.0, 290.0, 5.0 } << QStringLiteral("The slow brown fox jumps over the black dog.");
    QTest::newRow("above line") << QRectF { 50.0, 85.0, 290.0, 10.0 } << QString {};
    QTest::newRow("above line mid") << QRectF { 50.0, 90.0, 290.0, 5.0 } << QString {};
    QTest::newRow("first two words") << QRectF { 50.0, 90.0, 100.0, 20.0 } << QStringLiteral("The slow");
    QTest::newRow("first two words [narrow]") << QRectF { 50.0, 95.0, 100.0, 5.0 } << QStringLiteral("The slow");
    QTest::newRow("first character") << QRectF { 103.0, 95.0, 1.0, 5.0 } << QStringLiteral("T");
    QTest::newRow("last two words") << QRectF { 285.0, 90.0, 100.0, 20.0 } << QStringLiteral("black dog.");
    QTest::newRow("last character") << QRectF { 320.0, 90.0, 8.0, 20.0 } << QStringLiteral("g");
    QTest::newRow("middle 'fox'") << QRectF { 190.0, 90.0, 15.0, 20.0 } << QStringLiteral("fox");
    QTest::newRow("middle 'x'") << QRectF { 200.0, 90.0, 5.0, 20.0 } << QStringLiteral("x");
}

void TestActualText::checkAllOrientations()
{
    QFETCH(int, pageNr);
    QFETCH(QRectF, area);
    QFETCH(QString, text);

    QString path { QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf") };
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(path) };
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page { doc->page(pageNr) };
    QVERIFY(page);

    QCOMPARE(page->text(area), text);
}

void TestActualText::checkAllOrientations_data()
{
    QTest::addColumn<int>("pageNr");
    QTest::addColumn<QRectF>("area");
    QTest::addColumn<QString>("text");

    QTest::newRow("Portrait") << 0 << QRectF {} << QStringLiteral("Portrait");
    QTest::newRow("Landscape") << 1 << QRectF {} << QStringLiteral("Landscape");
    QTest::newRow("Upside down") << 2 << QRectF {} << QStringLiteral("Upside down");
    QTest::newRow("Seacape") << 3 << QRectF {} << QStringLiteral("Seascape");

    QTest::newRow("Portrait A4 rect") << 0 << QRectF { 0, 0, 595, 842 } << QStringLiteral("Portrait");
    QTest::newRow("Landscape A4 rect") << 1 << QRectF { 0, 0, 842, 595 } << QStringLiteral("Landscape");
    QTest::newRow("Upside down A4 rect") << 2 << QRectF { 0, 0, 595, 842 } << QStringLiteral("Upside down");
    QTest::newRow("Seacape A4 rect") << 3 << QRectF { 0, 0, 842, 595 } << QStringLiteral("Seascape");

    QTest::newRow("Portrait line rect") << 0 << QRectF { 30, 30, 60, 20 } << QStringLiteral("Portrait");
    QTest::newRow("Landscape line rect") << 1 << QRectF { 790, 30, 20, 80 } << QStringLiteral("Landscape");
    QTest::newRow("Upside down line rect") << 2 << QRectF { 485, 790, 75, 20 } << QStringLiteral("Upside down");
    QTest::newRow("Seacape line rect") << 3 << QRectF { 30, 500, 20, 70 } << QStringLiteral("Seascape");

    QTest::newRow("Portrait small rect B") << 0 << QRectF { 30, 35, 10, 10 } << QStringLiteral("P");
    QTest::newRow("Portrait small rect E") << 0 << QRectF { 80, 35, 10, 10 } << QStringLiteral("t");
    QTest::newRow("Landscape small rect B") << 1 << QRectF { 800, 30, 10, 10 } << QStringLiteral("L");
    QTest::newRow("Landscape small rect E") << 1 << QRectF { 800, 90, 10, 10 } << QStringLiteral("e");
    QTest::newRow("Upside down small rect B") << 2 << QRectF { 550, 800, 10, 10 } << QStringLiteral("U");
    QTest::newRow("Upside down small rect E") << 2 << QRectF { 485, 800, 10, 10 } << QStringLiteral("n");
    QTest::newRow("Seacape small rect B") << 3 << QRectF { 40, 550, 10, 10 } << QStringLiteral("S");
    QTest::newRow("Seacape small rect E") << 3 << QRectF { 40, 510, 10, 10 } << QStringLiteral("p");
}

void TestActualText::checkFakeboldText()
{
    QFETCH(int, pageNr);
    QFETCH(QRectF, area);
    QFETCH(QString, text);

    QString path { QStringLiteral(TESTDATADIR "/unittestcases/fakebold.pdf") };
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(path) };
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page { doc->page(pageNr) };
    QVERIFY(page);

    QCOMPARE(page->text(area), text);
}

void TestActualText::checkFakeboldText_data()
{
    QTest::addColumn<int>("pageNr");
    QTest::addColumn<QRectF>("area");
    QTest::addColumn<QString>("text");

    QTest::newRow("Upright line 1") << 0 << QRectF { 0, 0, 595, 80 } << QStringLiteral("1 This is fakebold text.");
    QTest::newRow("Upright line 2") << 0 << QRectF { 0, 80, 595, 80 } << QStringLiteral("2 This is a fakebold word.");
    QTest::newRow("Upright line 3") << 0 << QRectF { 0, 140, 595, 80 } << QStringLiteral("3 The last word is in fakebold.");
    QTest::newRow("Upright line 4") << 0 << QRectF { 0, 220, 595, 80 } << QStringLiteral("4 Hyphenated-fakebold word.");
    QTest::newRow("Upright line 5") << 0 << QRectF { 0, 300, 595, 80 } << QStringLiteral("5 Quoted \"fakebold\" word.");

    QTest::newRow("Rotated 90' line 1") << 1 << QRectF { 510, 0, 80, 842 } << QStringLiteral("1 This is fakebold text.");
    QTest::newRow("Rotated 90' line 2") << 1 << QRectF { 430, 0, 80, 842 } << QStringLiteral("2 This is a fakebold word.");
    QTest::newRow("Rotated 90' line 3") << 1 << QRectF { 350, 0, 80, 842 } << QStringLiteral("3 The last word is in fakebold.");
    QTest::newRow("Rotated 90' line 4") << 1 << QRectF { 270, 0, 80, 842 } << QStringLiteral("4 Hyphenated-fakebold word.");
    QTest::newRow("Rotated 90' line 5") << 1 << QRectF { 190, 0, 80, 842 } << QStringLiteral("5 Quoted \"fakebold\" word.");

    QTest::newRow("Rotated 180' line 1") << 2 << QRectF { 0, 760, 595, 80 } << QStringLiteral("1 This is fakebold text.");
    QTest::newRow("Rotated 180' line 2") << 2 << QRectF { 0, 680, 595, 80 } << QStringLiteral("2 This is a fakebold word.");
    QTest::newRow("Rotated 180' line 3") << 2 << QRectF { 0, 600, 595, 80 } << QStringLiteral("3 The last word is in fakebold.");
    QTest::newRow("Rotated 180' line 4") << 2 << QRectF { 0, 520, 595, 80 } << QStringLiteral("4 Hyphenated-fakebold word.");
    QTest::newRow("Rotated 180' line 5") << 2 << QRectF { 0, 440, 595, 80 } << QStringLiteral("5 Quoted \"fakebold\" word.");

    QTest::newRow("Rotated 270' line 1") << 3 << QRectF { 20, 0, 80, 842 } << QStringLiteral("1 This is fakebold text.");
    QTest::newRow("Rotated 270' line 2") << 3 << QRectF { 100, 0, 80, 842 } << QStringLiteral("2 This is a fakebold word.");
    QTest::newRow("Rotated 270' line 3") << 3 << QRectF { 160, 0, 80, 842 } << QStringLiteral("3 The last word is in fakebold.");
    QTest::newRow("Rotated 270' line 4") << 3 << QRectF { 240, 0, 80, 842 } << QStringLiteral("4 Hyphenated-fakebold word.");
    QTest::newRow("Rotated 270' line 5") << 3 << QRectF { 320, 0, 80, 842 } << QStringLiteral("5 Quoted \"fakebold\" word.");
}

QTEST_GUILESS_MAIN(TestActualText)

#include "check_actualtext.moc"
