#include <QtTest/QTest>
#include <QtCore/QTimeZone>

#include <poppler-qt6.h>

class TestMetaData : public QObject
{
    Q_OBJECT
public:
    explicit TestMetaData(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkStrings_data();
    static void checkStrings();
    static void checkStrings2_data();
    static void checkStrings2();
    static void checkStringKeys();
    static void checkLinearised();
    static void checkNumPages();
    static void checkDate();
    static void checkPageSize();
    static void checkPortraitOrientation();
    static void checkLandscapeOrientation();
    static void checkUpsideDownOrientation();
    static void checkSeascapeOrientation();
    static void checkVersion();
    static void checkPdfId();
    static void checkNoPdfId();
};

void TestMetaData::checkStrings_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("Author") << "Author"
                            << "Brad Hards";
    QTest::newRow("Title") << "Title"
                           << "Two pages";
    QTest::newRow("Subject") << "Subject"
                             << "A two page layout for poppler testing";
    QTest::newRow("Keywords") << "Keywords"
                              << "Qt4 bindings";
    QTest::newRow("Creator") << "Creator"
                             << "iText: cgpdftops CUPS filter";
    QTest::newRow("Producer") << "Producer"
                              << "Acrobat Distiller 7.0 for Macintosh";
}

void TestMetaData::checkStrings()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/doublepage.pdf"));
    QVERIFY(doc);

    QFETCH(QString, key);
    QFETCH(QString, value);
    QCOMPARE(doc->info(key), value);
}

void TestMetaData::checkStrings2_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("value");

    QTest::newRow("Title") << "Title"
                           << "Malaga hotels";
    QTest::newRow("Author") << "Author"
                            << "Brad Hards";
    QTest::newRow("Creator") << "Creator"
                             << "Safari: cgpdftops CUPS filter";
    QTest::newRow("Producer") << "Producer"
                              << "Acrobat Distiller 7.0 for Macintosh";
    QTest::newRow("Keywords") << "Keywords"
                              << "First\rSecond\rthird";
    QTest::newRow("Custom1") << "Custom1"
                             << "CustomValue1";
    QTest::newRow("Custom2") << "Custom2"
                             << "CustomValue2";
}

void TestMetaData::checkStrings2()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);

    QFETCH(QString, key);
    QFETCH(QString, value);
    QCOMPARE(doc->info(key), value);
}

void TestMetaData::checkStringKeys()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);

    QStringList keyList;
    keyList << QStringLiteral("Title") << QStringLiteral("Author") << QStringLiteral("Creator") << QStringLiteral("Keywords") << QStringLiteral("CreationDate");
    keyList << QStringLiteral("Producer") << QStringLiteral("ModDate") << QStringLiteral("Custom1") << QStringLiteral("Custom2");
    keyList.sort();
    QStringList keysInDoc = doc->infoKeys();
    keysInDoc.sort();
    QCOMPARE(keysInDoc, keyList);
}

void TestMetaData::checkLinearised()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->isLinearized());

    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);
    QCOMPARE(doc->isLinearized(), false);
}

void TestMetaData::checkPortraitOrientation()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(0);
    QCOMPARE(page->orientation(), Poppler::Page::Portrait);
}

void TestMetaData::checkNumPages()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/doublepage.pdf"));
    QVERIFY(doc);
    QCOMPARE(doc->numPages(), 2);

    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);
    QCOMPARE(doc->numPages(), 1);
}

void TestMetaData::checkDate()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);
    QCOMPARE(doc->date(QStringLiteral("ModDate")), QDateTime(QDate(2005, 12, 5), QTime(9, 44, 46), QTimeZone::utc()));
    QCOMPARE(doc->date(QStringLiteral("CreationDate")), QDateTime(QDate(2005, 8, 13), QTime(1, 12, 11), QTimeZone::utc()));
}

void TestMetaData::checkPageSize()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);
    std::unique_ptr<Poppler::Page> page = doc->page(0);
    QCOMPARE(page->pageSize(), QSize(595, 842));
    QCOMPARE(page->pageSizeF(), QSizeF(595.22, 842));
}

void TestMetaData::checkLandscapeOrientation()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(1);
    QCOMPARE(page->orientation(), Poppler::Page::Landscape);
}

void TestMetaData::checkUpsideDownOrientation()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(2);
    QCOMPARE(page->orientation(), Poppler::Page::UpsideDown);
}

void TestMetaData::checkSeascapeOrientation()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(3);
    QCOMPARE(page->orientation(), Poppler::Page::Seascape);
}

void TestMetaData::checkVersion()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/doublepage.pdf"));
    QVERIFY(doc);

    auto pdfVersion = doc->getPdfVersion();
    QCOMPARE(pdfVersion.major, 1);
    QCOMPARE(pdfVersion.minor, 6);
}

void TestMetaData::checkPdfId()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/A6EmbeddedFiles.pdf"));
    QVERIFY(doc);

    const QByteArray referencePermanentId("00C9D5B6D8FB11D7A902003065D630AA");
    const QByteArray referenceUpdateId("39AECAE6D8FB11D7A902003065D630AA");

    {
        // no IDs wanted, just existance check
        QVERIFY(doc->getPdfId(nullptr, nullptr));
    }
    {
        // only permanent ID
        QByteArray permanentId;
        QVERIFY(doc->getPdfId(&permanentId, nullptr));
        QCOMPARE(permanentId.toUpper(), referencePermanentId);
    }
    {
        // only update ID
        QByteArray updateId;
        QVERIFY(doc->getPdfId(nullptr, &updateId));
        QCOMPARE(updateId.toUpper(), referenceUpdateId);
    }
    {
        // both IDs
        QByteArray permanentId;
        QByteArray updateId;
        QVERIFY(doc->getPdfId(&permanentId, &updateId));
        QCOMPARE(permanentId.toUpper(), referencePermanentId);
        QCOMPARE(updateId.toUpper(), referenceUpdateId);
    }
}

void TestMetaData::checkNoPdfId()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/WithActualText.pdf"));
    QVERIFY(doc);

    QVERIFY(!doc->getPdfId(nullptr, nullptr));
}

QTEST_GUILESS_MAIN(TestMetaData)
#include "check_metadata.moc"
