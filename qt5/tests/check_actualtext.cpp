#include <QtTest/QTest>

#include <poppler-qt5.h>

#include <QtCore/QFile>

class TestActualText : public QObject
{
    Q_OBJECT
public:
    explicit TestActualText(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkActualText1();
    void checkActualText2();
    void checkActualText2_data();

private:
    void checkActualText(Poppler::Document *doc, const QRectF &area, const QString &text);
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
    doc = Poppler::Document::load(TESTDATADIR "/unittestcases/WithActualText.pdf");
    QVERIFY(doc);

    checkActualText(doc, QRectF {}, QStringLiteral("The slow brown fox jumps over the black dog."));

    delete doc;
}

void TestActualText::checkActualText2()
{
    QFETCH(QRectF, area);
    QFETCH(QString, text);

    QFile file(TESTDATADIR "/unittestcases/WithActualText.pdf");
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

QTEST_GUILESS_MAIN(TestActualText)

#include "check_actualtext.moc"
