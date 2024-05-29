#include <cmath>
#include <memory>
#include <sstream>

#include <QtTest/QTest>
#include <QTemporaryFile>

#include <poppler-qt5.h>

#include "poppler/Annot.h"
#include "goo/GooString.h"
#include "goo/gstrtod.h"

class TestAnnotations : public QObject
{
    Q_OBJECT
public:
    explicit TestAnnotations(QObject *parent = nullptr) : QObject(parent) { }

    void saveAndCheck(const std::unique_ptr<Poppler::Document> &doc, const std::function<void(Poppler::Annotation *a)> &checkFunction);

private slots:
    void checkQColorPrecision();
    void checkFontSizeAndColor();
    void checkHighlightFromAndToQuads();
    void checkUTF16LEAnnot();
    void checkModificationCreationDate();
    void checkNonMarkupAnnotations();
    void checkDefaultAppearance();
};

/* Is .5f sufficient for 16 bit color channel roundtrip trough save and load on all architectures? */
void TestAnnotations::checkQColorPrecision()
{
    bool precisionOk = true;
    for (int i = std::numeric_limits<uint16_t>::min(); i <= std::numeric_limits<uint16_t>::max(); i++) {
        double normalized = static_cast<uint16_t>(i) / static_cast<double>(std::numeric_limits<uint16_t>::max());
        const std::string serialized = GooString::format("{0:.5f}", normalized);
        double deserialized = gatof(serialized.c_str());
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
    const QString contents = QStringLiteral("foobar");
    const std::vector<QColor> testColors { QColor::fromRgb(0xAB, 0xCD, 0xEF), QColor::fromCmyk(0xAB, 0xBC, 0xCD, 0xDE) };
    const QFont testFont(QStringLiteral("Helvetica"), 20);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    {
        std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(TESTDATADIR "/unittestcases/UseNone.pdf") };
        QVERIFY(doc.get());

        std::unique_ptr<Poppler::Page> page { doc->page(0) };
        QVERIFY(page.get());

        for (const auto &color : testColors) {
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
        std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(tempFile.fileName()) };
        QVERIFY(doc.get());

        std::unique_ptr<Poppler::Page> page { doc->page(0) };
        QVERIFY(page.get());

        auto annots = page->annotations();
        QCOMPARE(annots.size(), static_cast<int>(testColors.size()));

        auto &&annot = annots.constBegin();
        for (const auto &color : testColors) {
            QCOMPARE((*annot)->subType(), Poppler::Annotation::AText);
            auto textAnnot = static_cast<Poppler::TextAnnotation *>(*annot);
            QCOMPARE(textAnnot->contents(), contents);
            QCOMPARE(textAnnot->textFont().pointSize(), testFont.pointSize());
            QCOMPARE(static_cast<int>(textAnnot->textColor().spec()), static_cast<int>(color.spec()));
            QCOMPARE(textAnnot->textColor(), color);
            if (annot != annots.constEnd()) {
                ++annot;
            }
        }
        qDeleteAll(annots);
    }
}

namespace Poppler {
static bool operator==(const Poppler::HighlightAnnotation::Quad &a, const Poppler::HighlightAnnotation::Quad &b)
{
    // FIXME We do not compare capStart, capEnd and feather since AnnotQuadrilaterals doesn't contain that info and thus
    //       HighlightAnnotationPrivate::fromQuadrilaterals uses default values
    return a.points[0] == b.points[0] && a.points[1] == b.points[1] && a.points[2] == b.points[2] && a.points[3] == b.points[3];
}
}

void TestAnnotations::checkHighlightFromAndToQuads()
{
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(TESTDATADIR "/unittestcases/UseNone.pdf") };

    std::unique_ptr<Poppler::Page> page { doc->page(0) };

    auto ha = std::make_unique<Poppler::HighlightAnnotation>();
    page->addAnnotation(ha.get());

    const QList<Poppler::HighlightAnnotation::Quad> quads = { { { { 0, 0.1 }, { 0.2, 0.3 }, { 0.4, 0.5 }, { 0.6, 0.7 } }, false, false, 0 }, { { { 0.8, 0.9 }, { 0.1, 0.2 }, { 0.3, 0.4 }, { 0.5, 0.6 } }, true, false, 0.4 } };
    ha->setHighlightQuads(quads);
    QCOMPARE(ha->highlightQuads(), quads);
}

void TestAnnotations::checkUTF16LEAnnot()
{
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(TESTDATADIR "/unittestcases/utf16le-annot.pdf") };
    QVERIFY(doc.get());

    std::unique_ptr<Poppler::Page> page { doc->page(0) };
    QVERIFY(page.get());

    auto annots = page->annotations();
    QCOMPARE(annots.size(), 2);

    auto annot = annots[1];
    QCOMPARE(annot->contents(), QString::fromUtf8("Únîcödé豰")); // clazy:exclude=qstring-allocations

    qDeleteAll(annots);
}

void TestAnnotations::saveAndCheck(const std::unique_ptr<Poppler::Document> &doc, const std::function<void(Poppler::Annotation *a)> &checkFunction)
{
    // also check that saving yields the same output
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    std::unique_ptr<Poppler::PDFConverter> conv(doc->pdfConverter());
    conv->setOutputFileName(tempFile.fileName());
    conv->setPDFOptions(Poppler::PDFConverter::WithChanges);
    conv->convert();

    std::unique_ptr<Poppler::Document> savedDoc { Poppler::Document::load(tempFile.fileName()) };
    std::unique_ptr<Poppler::Page> page { doc->page(0) };
    auto annots = page->annotations();
    checkFunction(annots.at(1));
    qDeleteAll(annots);
}

void TestAnnotations::checkModificationCreationDate()
{
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(TESTDATADIR "/unittestcases/utf16le-annot.pdf") };
    QVERIFY(doc.get());

    std::unique_ptr<Poppler::Page> page { doc->page(0) };

    auto annots = page->annotations();
    auto annot = annots.at(1);
    QCOMPARE(annot->creationDate(), QDateTime());
    QCOMPARE(annot->modificationDate(), QDateTime());

    const QDateTime dt1(QDate(2020, 8, 7), QTime(18, 34, 56));
    annot->setCreationDate(dt1);
    auto checkFunction1 = [dt1](Poppler::Annotation *a) {
        QCOMPARE(a->creationDate(), dt1);
        // setting the creation date updates the modification date
        QVERIFY(std::abs(a->modificationDate().secsTo(QDateTime::currentDateTime())) < 2);
    };
    checkFunction1(annot);
    saveAndCheck(doc, checkFunction1);

    const QDateTime dt2(QDate(2020, 8, 30), QTime(8, 14, 52));
    annot->setModificationDate(dt2);
    auto checkFunction2 = [dt2](Poppler::Annotation *a) { QCOMPARE(a->modificationDate(), dt2); };
    checkFunction2(annot);
    saveAndCheck(doc, checkFunction2);

    // setting the creation date to empty means "use the modification date" and also updates the modification date
    // so both creation date and modification date are the same and are now
    annot->setCreationDate(QDateTime());
    auto checkFunction3 = [](Poppler::Annotation *a) {
        QVERIFY(std::abs(a->creationDate().secsTo(QDateTime::currentDateTime())) < 2);
        QCOMPARE(a->creationDate(), a->modificationDate());
    };
    checkFunction3(annot);
    saveAndCheck(doc, checkFunction3);

    annot->setModificationDate(QDateTime());
    auto checkFunction4 = [](Poppler::Annotation *a) {
        QCOMPARE(a->creationDate(), QDateTime());
        QCOMPARE(a->modificationDate(), QDateTime());
    };
    checkFunction4(annot);
    saveAndCheck(doc, checkFunction4);

    qDeleteAll(annots);
}

void TestAnnotations::checkNonMarkupAnnotations()
{
    std::unique_ptr<Poppler::Document> doc { Poppler::Document::load(TESTDATADIR "/unittestcases/checkbox_issue_159.pdf") };
    QVERIFY(doc.get());

    std::unique_ptr<Poppler::Page> page { doc->page(0) };
    QVERIFY(page.get());

    auto annots = page->annotations();
    QCOMPARE(annots.size(), 17);
    qDeleteAll(annots);
}

void TestAnnotations::checkDefaultAppearance()
{
    std::unique_ptr<GooString> roundtripString;
    {
        GooString daString { "/Helv 10 Tf 0.1 0.2 0.3 rg" };
        const DefaultAppearance da { &daString };
        QCOMPARE(da.getFontPtSize(), 10.);
        QVERIFY(da.getFontName().isName());
        QCOMPARE(da.getFontName().getName(), "Helv");
        const AnnotColor *color = da.getFontColor();
        QVERIFY(color);
        QCOMPARE(color->getSpace(), AnnotColor::colorRGB);
        QCOMPARE(color->getValues()[0], 0.1);
        QCOMPARE(color->getValues()[1], 0.2);
        QCOMPARE(color->getValues()[2], 0.3);
        roundtripString = std::make_unique<GooString>(da.toAppearanceString());
    }
    {
        /* roundtrip through parse/generate/parse shall preserve values */
        const DefaultAppearance da { roundtripString.get() };
        QCOMPARE(da.getFontPtSize(), 10.);
        QVERIFY(da.getFontName().isName());
        QCOMPARE(da.getFontName().getName(), "Helv");
        const AnnotColor *color = da.getFontColor();
        QVERIFY(color);
        QCOMPARE(color->getSpace(), AnnotColor::colorRGB);
        QCOMPARE(color->getValues()[0], 0.1);
        QCOMPARE(color->getValues()[1], 0.2);
        QCOMPARE(color->getValues()[2], 0.3);
    }
    {
        /* parsing bad DA strings must not cause crash */
        GooString daString { "/ % Tf 1 2 rg" };
        const DefaultAppearance da { &daString };
        QVERIFY(!da.getFontName().isName());
    }
}

QTEST_GUILESS_MAIN(TestAnnotations)

#include "check_annotations.moc"
