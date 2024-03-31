#include <QtTest/QTest>

#include <poppler-qt6.h>

#include <memory>

class TestOutline : public QObject
{
    Q_OBJECT
public:
    explicit TestOutline(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkOutline_xr02();
};

void TestOutline::checkOutline_xr02()
{
    std::unique_ptr<Poppler::Document> document { Poppler::Document::load(TESTDATADIR "/unittestcases/xr02.pdf") };
    QVERIFY(document.get());

    const auto outline = document->outline();
    QCOMPARE(outline.size(), 2);

    const auto &foo = outline[0];
    QVERIFY(!foo.isNull());
    QCOMPARE(foo.name(), QStringLiteral("foo"));
    QCOMPARE(foo.isOpen(), false);
    const auto fooDest = foo.destination();
    QVERIFY(!fooDest.isNull());
    QCOMPARE(fooDest->pageNumber(), 1);
    QVERIFY(foo.externalFileName().isEmpty());
    QVERIFY(foo.uri().isEmpty());
    QVERIFY(!foo.hasChildren());
    QVERIFY(foo.children().isEmpty());

    const auto &bar = outline[1];
    QVERIFY(!bar.isNull());
    QCOMPARE(bar.name(), QStringLiteral("bar"));
    QCOMPARE(bar.isOpen(), false);
    const auto barDest = bar.destination();
    QVERIFY(!barDest.isNull());
    QCOMPARE(barDest->pageNumber(), 2);
    QVERIFY(bar.externalFileName().isEmpty());
    QVERIFY(bar.uri().isEmpty());
    QVERIFY(!bar.hasChildren());
    QVERIFY(bar.children().isEmpty());
}

QTEST_GUILESS_MAIN(TestOutline)
#include "check_outline.moc"
