#include <memory>

#include <QtTest/QTest>
#include <QtCore/QDebug>
#include <QImage>

#include <poppler-qt5.h>

// Unit tests for rendering axial shadings without full opacity
class TestStrokeOpacity : public QObject
{
    Q_OBJECT
public:
    explicit TestStrokeOpacity(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkStrokeOpacity_data();
    static void checkStrokeOpacity();
};

void TestStrokeOpacity::checkStrokeOpacity_data()
{
    QTest::addColumn<int>("backendType");

    QTest::newRow("splash") << (int)Poppler::Document::SplashBackend;
    QTest::newRow("qpainter") << (int)Poppler::Document::QPainterBackend;
}

void TestStrokeOpacity::checkStrokeOpacity()
{
    QFETCH(int, backendType);

    auto doc = std::unique_ptr<Poppler::Document>(Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/stroke-alpha-pattern.pdf")));
    QVERIFY(doc != nullptr);

    doc->setRenderBackend((Poppler::Document::RenderBackend)backendType);

    // BUG: For some reason splash gets the opacity wrong when antialiasing is switched off
    if (backendType == (int)Poppler::Document::SplashBackend) {
        doc->setRenderHint(Poppler::Document::Antialiasing, true);
    }

    const auto page = std::unique_ptr<Poppler::Page>(doc->page(0));
    QVERIFY(page != nullptr);

    // Render (at low resolution and with cropped marging)
    QImage image = page->renderToImage(36, 36, 40, 50, 200, 230);

    // The actual tests start here

    // Allow a tolerance.
    int tolerance;
    auto approximatelyEqual = [&tolerance](QRgb c0, const QColor &c1) {
        return std::abs(qAlpha(c0) - c1.alpha()) <= tolerance && std::abs(qRed(c0) - c1.red()) <= tolerance && std::abs(qGreen(c0) - c1.green()) <= tolerance && std::abs(qBlue(c0) - c1.blue()) <= tolerance;
    };

    // At the lower left of the test document is a square with an axial shading,
    // which should be rendered with opacity 0.25.
    // Check that with a sample pixel
    auto pixel = image.pixel(70, 160);

    // Splash and QPainter backends implement shadings slightly differently,
    // hence we cannot expect to get precisely the same colors.
    tolerance = 2;
    QVERIFY(approximatelyEqual(pixel, QColor(253, 233, 196, 255)));

    // At the upper left of the test document is a stroked square with an axial shading.
    // This is implemented by filling a clip region defined by a stroke outline.
    // Check whether the backend really only renders the stroke, not the region
    // surrounded by the stroke.
    auto pixelUpperLeftInterior = image.pixel(70, 70);

    tolerance = 0;
    QVERIFY(approximatelyEqual(pixelUpperLeftInterior, Qt::white));

    // Now check whether that stroke is semi-transparent.
    // Bug https://gitlab.freedesktop.org/poppler/poppler/-/issues/178
    auto pixelUpperLeftOnStroke = image.pixel(70, 20);

    tolerance = 2;
    QVERIFY(approximatelyEqual(pixelUpperLeftOnStroke, QColor(253, 233, 196, 255)));

    // At the upper right there is a semi-transparent stroked red square
    // a) Make sure that the color is correct.
    auto pixelUpperRightOnStroke = image.pixel(130, 20);

    tolerance = 0;
    QVERIFY(approximatelyEqual(pixelUpperRightOnStroke, QColor(246, 196, 206, 255)));

    // b) Make sure that it is really stroked, not filled
    auto pixelUpperRightInterior = image.pixel(130, 50);
    QVERIFY(approximatelyEqual(pixelUpperRightInterior, Qt::white));
}

QTEST_GUILESS_MAIN(TestStrokeOpacity)

#include "check_stroke_opacity.moc"
