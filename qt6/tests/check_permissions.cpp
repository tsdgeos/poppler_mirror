#include <QtTest/QTest>

#include <poppler-qt6.h>

class TestPermissions : public QObject
{
    Q_OBJECT
public:
    explicit TestPermissions(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void permissions1();
};

void TestPermissions::permissions1()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    // we are allowed to print
    QVERIFY(doc->okToPrint());

    // we are not allowed to change
    QVERIFY(!(doc->okToChange()));

    // we are not allowed to copy or extract content
    QVERIFY(!(doc->okToCopy()));

    // we are not allowed to print at high resolution
    QVERIFY(!(doc->okToPrintHighRes()));

    // we are not allowed to fill forms
    QVERIFY(!(doc->okToFillForm()));

    // we are allowed to extract content for accessibility
    QVERIFY(doc->okToExtractForAccessibility());

    // we are allowed to assemble this document
    QVERIFY(doc->okToAssemble());
}

QTEST_GUILESS_MAIN(TestPermissions)
#include "check_permissions.moc"
