#include <QtTest/QTest>

#include <poppler-qt5.h>

#include <memory>

class TestOverprint : public QObject
{
    Q_OBJECT
public:
    explicit TestOverprint(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkOverprintImageRendering();
};

void TestOverprint::checkOverprintImageRendering()
{
    Poppler::Document *doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/tests/mask-seams.pdf"));
    QVERIFY(doc);

    doc->setRenderHint(Poppler::Document::OverprintPreview, true);

    Poppler::Page *page = doc->page(0);
    QVERIFY(page);

    constexpr int width = 600;
    constexpr int height = 400;

    QImage img = page->renderToImage(300.0, 300.0, 0, 0, width, height);
    QCOMPARE(img.format(), QImage::Format_RGB32);
    QCOMPARE(img.width(), width);
    QCOMPARE(img.height(), height);
    QCOMPARE(img.bytesPerLine(), width * 4);
    QCOMPARE(img.sizeInBytes(), width * height * 4);

    delete page;
    delete doc;
}

QTEST_GUILESS_MAIN(TestOverprint)
#include "check_overprint.moc"
