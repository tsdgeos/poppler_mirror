#include <cmath>
#include <memory>
#include <sstream>

#include <QtTest/QtTest>
#include <QTemporaryFile>

#include <poppler-qt5.h>

#include "goo/GooString.h"
#include "goo/gstrtod.h"

class TestAnnotations : public QObject
{
  Q_OBJECT
private slots:
  void checkQColorPrecision();
  void checkFontSizeAndColor();
};

/* Is .5f sufficient for 16 bit color channel roundtrip trough save and load on all architectures? */
void TestAnnotations::checkQColorPrecision() {
  bool precisionOk = true;
  for (int i = std::numeric_limits<uint16_t>::min(); i <= std::numeric_limits<uint16_t>::max(); i++) {
    double normalized = static_cast<uint16_t>(i) / static_cast<double>(std::numeric_limits<uint16_t>::max());
    GooString* serialized = GooString::format("{0:.5f}", normalized);
    double deserialized = gatof( serialized->getCString() );
    uint16_t denormalized = std::round(deserialized * std::numeric_limits<uint16_t>::max());
    if (static_cast<uint16_t>(i) != denormalized) {
      precisionOk = false;
      break;
    }
  }
  QVERIFY(precisionOk);
}

void TestAnnotations::checkFontSizeAndColor()
{
  const QString contents{"foobar"};
  const std::vector<QColor> testColors{QColor::fromRgb(0xAB, 0xCD, 0xEF),
                                       QColor::fromCmyk(0xAB, 0xBC, 0xCD, 0xDE)};
  const QFont testFont("Helvetica", 20);

  QTemporaryFile tempFile;
  QVERIFY(tempFile.open());
  tempFile.close();

  {
    std::unique_ptr<Poppler::Document> doc{
      Poppler::Document::load(TESTDATADIR "/unittestcases/UseNone.pdf")
    };
    QVERIFY(doc.get());

    std::unique_ptr<Poppler::Page> page{
      doc->page(0)
    };
    QVERIFY(page.get());

    for (const auto& color : testColors) {
      auto annot = std::make_unique<Poppler::TextAnnotation>(Poppler::TextAnnotation::InPlace);
      annot->setBoundary(QRectF(0.0, 0.0, 1.0, 1.0));
      annot->setContents(contents);
      annot->setTextFont(testFont);
      annot->setTextColor(color);
      page->addAnnotation(annot.get());
    }

    std::unique_ptr<Poppler::PDFConverter> conv(doc->pdfConverter());
    QVERIFY(conv.get());
    conv->setOutputFileName(tempFile.fileName());
    conv->setPDFOptions(Poppler::PDFConverter::WithChanges);
    QVERIFY(conv->convert());
  }

  {
    std::unique_ptr<Poppler::Document> doc{
      Poppler::Document::load(tempFile.fileName())
    };
    QVERIFY(doc.get());

    std::unique_ptr<Poppler::Page> page{
      doc->page(0)
    };
    QVERIFY(page.get());

    auto annots = page->annotations();
    QCOMPARE(annots.size(), static_cast<int>(testColors.size()));

    auto &&annot = annots.constBegin();
    for (const auto& color : testColors) {
      QCOMPARE((*annot)->subType(), Poppler::Annotation::AText);
      auto textAnnot = static_cast<Poppler::TextAnnotation*>(*annot);
      QCOMPARE(textAnnot->contents(), contents);
      QCOMPARE(textAnnot->textFont().pointSize(), testFont.pointSize());
      QCOMPARE(static_cast<int>(textAnnot->textColor().spec()), static_cast<int>(color.spec()));
      QCOMPARE(textAnnot->textColor(), color);
      if (annot != annots.end())
          ++annot;
    }
  }
}

QTEST_GUILESS_MAIN(TestAnnotations)

#include "check_annotations.moc"
