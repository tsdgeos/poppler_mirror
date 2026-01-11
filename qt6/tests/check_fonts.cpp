#include <QtTest/QTest>

#include <poppler-qt6.h>

#include <memory>

class TestFontsData : public QObject
{
    Q_OBJECT
public:
    explicit TestFontsData(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkNoFonts();
    static void checkType1();
    static void checkType3();
    static void checkTrueType();
    static void checkFontIterator();
    static void checkSecondDocumentQuery();
    static void checkMultipleIterations();
    static void checkIteratorFonts();
};

static QList<Poppler::FontInfo> loadFontsViaIterator(Poppler::Document *doc, int from = 0, int count = -1)
{
    int num = count == -1 ? doc->numPages() - from : count;
    QList<Poppler::FontInfo> list;
    std::unique_ptr<Poppler::FontIterator> it(doc->newFontIterator(from));
    while (it->hasNext() && num) {
        list += it->next();
        --num;
    }
    return list;
}

namespace Poppler {
static bool operator==(const FontInfo &f1, const FontInfo &f2)
{
    if (f1.name() != f2.name()) {
        return false;
    }
    if (f1.file() != f2.file()) {
        return false;
    }
    if (f1.isEmbedded() != f2.isEmbedded()) {
        return false;
    }
    if (f1.isSubset() != f2.isSubset()) {
        return false;
    }
    if (f1.type() != f2.type()) {
        return false;
    }
    if (f1.typeName() != f2.typeName()) {
        return false;
    }
    return true;
}
}

void TestFontsData::checkNoFonts()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/image.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 0);
}

void TestFontsData::checkType1()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/text.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 1);
    QCOMPARE(listOfFonts.at(0).name(), QLatin1String("Helvetica"));
    QCOMPARE(listOfFonts.at(0).type(), Poppler::FontInfo::Type1);
    QCOMPARE(listOfFonts.at(0).typeName(), QLatin1String("Type 1"));

    QCOMPARE(listOfFonts.at(0).isEmbedded(), false);
    QCOMPARE(listOfFonts.at(0).isSubset(), false);
}

void TestFontsData::checkType3()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/type3.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 2);
    QCOMPARE(listOfFonts.at(0).name(), QLatin1String("Helvetica"));
    QCOMPARE(listOfFonts.at(0).type(), Poppler::FontInfo::Type1);
    QCOMPARE(listOfFonts.at(0).typeName(), QLatin1String("Type 1"));

    QCOMPARE(listOfFonts.at(0).isEmbedded(), false);
    QCOMPARE(listOfFonts.at(0).isSubset(), false);

    QCOMPARE(listOfFonts.at(1).name(), QString());
    QCOMPARE(listOfFonts.at(1).type(), Poppler::FontInfo::Type3);
    QCOMPARE(listOfFonts.at(1).typeName(), QLatin1String("Type 3"));

    QCOMPARE(listOfFonts.at(1).isEmbedded(), true);
    QCOMPARE(listOfFonts.at(1).isSubset(), false);
}

void TestFontsData::checkTrueType()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 2);
    QCOMPARE(listOfFonts.at(0).name(), QLatin1String("Arial-BoldMT"));
    QCOMPARE(listOfFonts.at(0).type(), Poppler::FontInfo::TrueType);
    QCOMPARE(listOfFonts.at(0).typeName(), QLatin1String("TrueType"));

    QCOMPARE(listOfFonts.at(0).isEmbedded(), false);
    QCOMPARE(listOfFonts.at(0).isSubset(), false);

    QCOMPARE(listOfFonts.at(1).name(), QLatin1String("ArialMT"));
    QCOMPARE(listOfFonts.at(1).type(), Poppler::FontInfo::TrueType);
    QCOMPARE(listOfFonts.at(1).typeName(), QLatin1String("TrueType"));

    QCOMPARE(listOfFonts.at(1).isEmbedded(), false);
    QCOMPARE(listOfFonts.at(1).isSubset(), false);
}

void TestFontsData::checkFontIterator()
{
    // loading a 1-page document
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/type3.pdf"));
    QVERIFY(doc);
    // loading a 6-pages document
    std::unique_ptr<Poppler::Document> doc6 = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/cropbox.pdf"));
    QVERIFY(doc6);

    std::unique_ptr<Poppler::FontIterator> it;

    // some tests with the 1-page document:
    // - check a default iterator
    it = doc->newFontIterator();
    QVERIFY(it->hasNext());
    // - check an iterator for negative pages to behave as 0
    it = doc->newFontIterator(-1);
    QVERIFY(it->hasNext());
    // - check an iterator for pages out of the page limit
    it = doc->newFontIterator(1);
    QVERIFY(!it->hasNext());
    // - check that it reaches the end after 1 iteration
    it = doc->newFontIterator();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(!it->hasNext());

    // some tests with the 6-page document:
    // - check a default iterator
    it = doc6->newFontIterator();
    QVERIFY(it->hasNext());
    // - check an iterator for pages out of the page limit
    it = doc6->newFontIterator(6);
    QVERIFY(!it->hasNext());
    // - check that it reaches the end after 6 iterations
    it = doc6->newFontIterator();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(it->hasNext());
    it->next();
    QVERIFY(!it->hasNext());
}

void TestFontsData::checkSecondDocumentQuery()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/type3.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 2);
    // check we get the very same result when calling fonts() again (#19405)
    QList<Poppler::FontInfo> listOfFonts2 = doc->fonts();
    QCOMPARE(listOfFonts, listOfFonts2);
}

void TestFontsData::checkMultipleIterations()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/type3.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = loadFontsViaIterator(doc.get());
    QCOMPARE(listOfFonts.size(), 2);
    QList<Poppler::FontInfo> listOfFonts2 = loadFontsViaIterator(doc.get());
    QCOMPARE(listOfFonts, listOfFonts2);
}

void TestFontsData::checkIteratorFonts()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/fonts.pdf"));
    QVERIFY(doc);

    QList<Poppler::FontInfo> listOfFonts = doc->fonts();
    QCOMPARE(listOfFonts.size(), 3);

    // check we get the very same result when gatering fonts using the iterator
    QList<Poppler::FontInfo> listOfFonts2 = loadFontsViaIterator(doc.get());
    QCOMPARE(listOfFonts, listOfFonts2);
}

QTEST_GUILESS_MAIN(TestFontsData)
#include "check_fonts.moc"
