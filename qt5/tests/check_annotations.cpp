#include <memory>

#include <QtTest/QtTest>
#include <QTemporaryFile>

#include <poppler-qt5.h>

class TestAnnotations : public QObject
{
  Q_OBJECT
private slots:
  void checkFontColor();
};

void TestAnnotations::checkFontColor()
{
  const QString contents{"foobar"};
  const QColor textColor{0xAB, 0xCD, 0xEF};

  QTemporaryFile tempFile;
  QVERIFY(tempFile.open());
  tempFile.close();

  {
    std::unique_ptr<Poppler::Document> doc{
      Poppler::Document::load(TESTDATADIR "/unittestcases/UseNone.pdf")
    };
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page{
      doc->page(0)
    };
    QVERIFY(page);

    std::unique_ptr<Poppler::TextAnnotation> annot{
      new Poppler::TextAnnotation{Poppler::TextAnnotation::InPlace}
    };

    annot->setBoundary(QRectF(0.0, 0.0, 1.0, 1.0));
    annot->setContents(contents);
    annot->setTextColor(textColor);

    page->addAnnotation(annot.get());

    std::unique_ptr<Poppler::PDFConverter> conv(doc->pdfConverter());
    QVERIFY(conv);
    conv->setOutputFileName(tempFile.fileName());
    conv->setPDFOptions(Poppler::PDFConverter::WithChanges);
    QVERIFY(conv->convert());
  }

  {
    std::unique_ptr<Poppler::Document> doc{
      Poppler::Document::load(tempFile.fileName())
    };
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page{
      doc->page(0)
    };
    QVERIFY(page);

    auto annots = page->annotations();
    QCOMPARE(1, annots.size());
    QCOMPARE(Poppler::Annotation::AText, annots.constFirst()->subType());

    auto annot = static_cast<Poppler::TextAnnotation*>(annots.constFirst());
    QCOMPARE(contents, annot->contents());
    QCOMPARE(textColor, annot->textColor());
  }
}

QTEST_GUILESS_MAIN(TestAnnotations)

#include "check_annotations.moc"
