#include <QtTest/QtTest>

#include <poppler-qt6.h>

#include <QtCore/QFile>

class TestActualText : public QObject
{
    Q_OBJECT
public:
    explicit TestActualText(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkActualText1();
    void checkActualText2();

private:
    void checkActualText(Poppler::Document &doc);
};

void TestActualText::checkActualText(Poppler::Document &doc)
{
    std::unique_ptr<Poppler::Page> page = doc.page(0);
    QVERIFY(page);

    QCOMPARE(page->text(QRectF()), QLatin1String("The slow brown fox jumps over the black dog."));
}

void TestActualText::checkActualText1()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/WithActualText.pdf");
    QVERIFY(doc);

    checkActualText(*doc);
}

void TestActualText::checkActualText2()
{
    QFile file(TESTDATADIR "/unittestcases/WithActualText.pdf");
    QVERIFY(file.open(QIODevice::ReadOnly));

    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(&file);
    QVERIFY(doc);

    checkActualText(*doc);
}

QTEST_GUILESS_MAIN(TestActualText)

#include "check_actualtext.moc"
