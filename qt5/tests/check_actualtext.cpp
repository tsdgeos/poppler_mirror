#include <QtTest/QtTest>

#include <poppler-qt5.h>

#include <QtCore/QFile>

class TestActualText: public QObject
{
    Q_OBJECT
public:
    TestActualText(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkActualText1();
};

void TestActualText::checkActualText1()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(TESTDATADIR "/unittestcases/WithActualText.pdf");
    QVERIFY( doc );

    Poppler::Page *page = doc->page(0);
    QVERIFY( page );

    QCOMPARE( page->text(QRectF()), QLatin1String("The slow brown fox jumps over the black dog.") );

    delete page;

    delete doc;
}

QTEST_GUILESS_MAIN(TestActualText)

#include "check_actualtext.moc"

