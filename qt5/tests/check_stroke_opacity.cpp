#include <memory>

#include <QtTest/QtTest>
#include <QtCore/QDebug>
#include <QImage>

#include <poppler-qt5.h>

// Unit tests for rendering axial shadings without full opacity
class TestStrokeOpacity : public QObject
{
    Q_OBJECT
public:
    TestStrokeOpacity(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkStrokeOpacity_data();
    void checkStrokeOpacity();
};

void TestStrokeOpacity::checkStrokeOpacity_data()
{
    QTest::addColumn<int>("backendType");

    QTest::newRow("splash")   << (int)Poppler::Document::SplashBackend;
    QTest::newRow("qpainter") << (int)Poppler::Document::ArthurBackend;
}

void TestStrokeOpacity::checkStrokeOpacity()
{
    QFETCH(int, backendType);

    auto doc = std::unique_ptr<Poppler::Document>(Poppler::Document::load(TESTDATADIR "/unittestcases/stroke-alpha-pattern.pdf"));
    QVERIFY(doc!=nullptr);

    doc->setRenderBackend((Poppler::Document::RenderBackend)backendType);

    // BUG: For some reason splash gets the opacity wrong when antialiasing is switched off
    if (backendType== (int)Poppler::Document::SplashBackend) {
        doc->setRenderHint(Poppler::Document::Antialiasing, true);
    }

    const auto page = std::unique_ptr<Poppler::Page>(doc->page(0));
    QVERIFY(page!=nullptr);

    // Render (at low resolution and with cropped marging)
    QImage image = page->renderToImage(36,36,40,50,200,230);

    // The actual tests start here

    // Splash and QPainter backends implement shadings slightly differently,
    // hence we cannot expect to get precisely the same colors.
    // Allow a tolerance up to '3' per channel.
    int tolerance = 3;
    auto approximatelyEqual = [&tolerance](QRgb c0, const QColor& c1)
    {
      return std::abs(qAlpha(c0) - c1.alpha() )  < tolerance
          && std::abs(qRed(c0)   - c1.red() ) < tolerance
          && std::abs(qGreen(c0) - c1.green() ) < tolerance
          && std::abs(qBlue(c0)  - c1.blue() ) < tolerance;
    };

    // At the lower left of the test document is a square with an axial shading,
    // which should be rendered with opacity 0.25.
    // Check that with a sample pixel
    auto pixel = image.pixel(70,160);

    QVERIFY(approximatelyEqual(pixel, QColor(253,233,196,255)));
}

QTEST_GUILESS_MAIN(TestStrokeOpacity)

#include "check_stroke_opacity.moc"

