#include <QtTest/QTest>

#include <poppler-qt6.h>

class EndOfLines : public QObject
{
    Q_OBJECT
public:
    explicit EndOfLines(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkEndOfLines();
};

void EndOfLines::checkEndOfLines()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/fonts.pdf"));
    QVERIFY(doc);

    auto page = doc->page(0);

    QVERIFY(page);

    auto text = page->text(QRect {});

    QCOMPARE(text, QStringLiteral("Hello World!\n\nHello World!\n\nHello World!"));
}

QTEST_GUILESS_MAIN(EndOfLines)
#include "check_endoflines.moc"
