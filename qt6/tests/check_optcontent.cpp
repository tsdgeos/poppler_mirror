#include <QtTest/QTest>

#include "PDFDoc.h"
#include "GlobalParams.h"

#include <poppler-qt6.h>
#include <poppler-optcontent-private.h>

class TestOptionalContent : public QObject
{
    Q_OBJECT
public:
    explicit TestOptionalContent(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkVisPolicy();
    static void checkNestedLayers();
    static void checkNoOptionalContent();
    static void checkIsVisible();
    static void checkVisibilitySetting();
    static void checkRadioButtons();
};

void TestOptionalContent::checkVisPolicy()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/vis_policy_test.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasOptionalContent());

    Poppler::OptContentModel *optContent = doc->optionalContentModel();
    QModelIndex index;
    index = optContent->index(0, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("A"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Checked);
    index = optContent->index(1, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("B"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Checked);
}

void TestOptionalContent::checkNestedLayers()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/NestedLayers.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasOptionalContent());

    Poppler::OptContentModel *optContent = doc->optionalContentModel();
    QModelIndex index;

    index = optContent->index(0, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("Black Text and Green Snow"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Unchecked);

    index = optContent->index(1, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("Mountains and Image"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Checked);

    // This is a sub-item of "Mountains and Image"
    QModelIndex subindex = optContent->index(0, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("Image"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Checked);

    index = optContent->index(2, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("Starburst"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Checked);

    index = optContent->index(3, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("Watermark"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Unchecked);
}

void TestOptionalContent::checkNoOptionalContent()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/orientation.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->hasOptionalContent(), false);
}

void TestOptionalContent::checkIsVisible()
{
    globalParams = std::make_unique<GlobalParams>();
    auto *doc = new PDFDoc(std::make_unique<GooString>(TESTDATADIR "/unittestcases/vis_policy_test.pdf"));
    QVERIFY(doc);

    const OCGs *ocgs = doc->getOptContentConfig();
    QVERIFY(ocgs);

    XRef *xref = doc->getXRef();

    Object obj;

    // In this test, both Ref(21,0) and Ref(2,0) are set to On

    // AnyOn, one element array:
    // 22 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QVERIFY(ocgs->optContentIsVisible(&obj));

    // Same again, looking for any leaks or dubious free()'s
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QVERIFY(ocgs->optContentIsVisible(&obj));

    // AnyOff, one element array:
    // 29 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOff>>endobj
    obj = xref->fetch(29, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOn, one element array:
    // 36 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOn>>endobj
    obj = xref->fetch(36, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOff, one element array:
    // 43 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOff>>endobj
    obj = xref->fetch(43, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AnyOn, multi-element array:
    // 50 0 obj<</Type/OCMD/OCGs[21 0 R 28 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(50, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOff, multi-element array:
    // 57 0 obj<</Type/OCMD/P/AnyOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(57, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOn, multi-element array:
    // 64 0 obj<</Type/OCMD/P/AllOn/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(64, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOff, multi-element array:
    // 71 0 obj<</Type/OCMD/P/AllOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(71, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    delete doc;
    globalParams.reset();
}

void TestOptionalContent::checkVisibilitySetting()
{
    globalParams = std::make_unique<GlobalParams>();
    auto *doc = new PDFDoc(std::make_unique<GooString>(TESTDATADIR "/unittestcases/vis_policy_test.pdf"));
    QVERIFY(doc);

    const OCGs *ocgs = doc->getOptContentConfig();
    QVERIFY(ocgs);

    XRef *xref = doc->getXRef();

    Object obj;

    // In this test, both Ref(21,0) and Ref(28,0) start On,
    // based on the file settings
    Object ref21obj(Ref { .num = 21, .gen = 0 });
    Ref ref21 = ref21obj.getRef();
    OptionalContentGroup *ocgA = ocgs->findOcgByRef(ref21);
    QVERIFY(ocgA);

    QVERIFY((ocgA->getName()->compare("A")) == 0);
    QCOMPARE(ocgA->getState(), OptionalContentGroup::On);

    Object ref28obj(Ref { .num = 28, .gen = 0 });
    Ref ref28 = ref28obj.getRef();
    OptionalContentGroup *ocgB = ocgs->findOcgByRef(ref28);
    QVERIFY(ocgB);

    QVERIFY((ocgB->getName()->compare("B")) == 0);
    QCOMPARE(ocgB->getState(), OptionalContentGroup::On);

    // turn one Off
    ocgA->setState(OptionalContentGroup::Off);

    // AnyOn, one element array:
    // 22 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // Same again, looking for any leaks or dubious free()'s
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AnyOff, one element array:
    // 29 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOff>>endobj
    obj = xref->fetch(29, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOn, one element array:
    // 36 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOn>>endobj
    obj = xref->fetch(36, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOff, one element array:
    // 43 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOff>>endobj
    obj = xref->fetch(43, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AnyOn, multi-element array:
    // 50 0 obj<</Type/OCMD/OCGs[21 0 R 28 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(50, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOff, multi-element array:
    // 57 0 obj<</Type/OCMD/P/AnyOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(57, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOn, multi-element array:
    // 64 0 obj<</Type/OCMD/P/AllOn/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(64, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOff, multi-element array:
    // 71 0 obj<</Type/OCMD/P/AllOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(71, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // Turn the other one off as well (i.e. both are Off)
    ocgB->setState(OptionalContentGroup::Off);

    // AnyOn, one element array:
    // 22 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // Same again, looking for any leaks or dubious free()'s
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AnyOff, one element array:
    // 29 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOff>>endobj
    obj = xref->fetch(29, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOn, one element array:
    // 36 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOn>>endobj
    obj = xref->fetch(36, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOff, one element array:
    // 43 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOff>>endobj
    obj = xref->fetch(43, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOn, multi-element array:
    // 50 0 obj<</Type/OCMD/OCGs[21 0 R 28 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(50, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AnyOff, multi-element array:
    // 57 0 obj<</Type/OCMD/P/AnyOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(57, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOn, multi-element array:
    // 64 0 obj<</Type/OCMD/P/AllOn/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(64, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOff, multi-element array:
    // 71 0 obj<</Type/OCMD/P/AllOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(71, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // Turn the first one on again (21 is On, 28 is Off)
    ocgA->setState(OptionalContentGroup::On);

    // AnyOn, one element array:
    // 22 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // Same again, looking for any leaks or dubious free()'s
    obj = xref->fetch(22, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOff, one element array:
    // 29 0 obj<</Type/OCMD/OCGs[21 0 R]/P/AnyOff>>endobj
    obj = xref->fetch(29, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOn, one element array:
    // 36 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOn>>endobj
    obj = xref->fetch(36, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOff, one element array:
    // 43 0 obj<</Type/OCMD/OCGs[28 0 R]/P/AllOff>>endobj
    obj = xref->fetch(43, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOn, multi-element array:
    // 50 0 obj<</Type/OCMD/OCGs[21 0 R 28 0 R]/P/AnyOn>>endobj
    obj = xref->fetch(50, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AnyOff, multi-element array:
    // 57 0 obj<</Type/OCMD/P/AnyOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(57, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), true);

    // AllOn, multi-element array:
    // 64 0 obj<</Type/OCMD/P/AllOn/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(64, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    // AllOff, multi-element array:
    // 71 0 obj<</Type/OCMD/P/AllOff/OCGs[21 0 R 28 0 R]>>endobj
    obj = xref->fetch(71, 0);
    QVERIFY(obj.isDict());
    QCOMPARE(ocgs->optContentIsVisible(&obj), false);

    delete doc;
    globalParams.reset();
}

void TestOptionalContent::checkRadioButtons()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/ClarityOCGs.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasOptionalContent());

    Poppler::OptContentModel *optContent = doc->optionalContentModel();
    QModelIndex index;

    index = optContent->index(0, 0, QModelIndex());
    QCOMPARE(optContent->data(index, Qt::DisplayRole).toString(), QLatin1String("Languages"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(index, Qt::CheckStateRole).toInt()), Qt::Unchecked);

    // These are sub-items of the "Languages" label
    QModelIndex subindex = optContent->index(0, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("English"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Checked);

    subindex = optContent->index(1, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("French"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);

    subindex = optContent->index(2, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("Japanese"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);

    // RBGroup of languages, so turning on Japanese should turn off English
    QVERIFY(optContent->setData(subindex, QVariant(true), Qt::CheckStateRole));

    subindex = optContent->index(0, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("English"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    subindex = optContent->index(2, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("Japanese"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Checked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::On);

    subindex = optContent->index(1, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("French"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    // and turning on French should turn off Japanese
    QVERIFY(optContent->setData(subindex, QVariant(true), Qt::CheckStateRole));

    subindex = optContent->index(0, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("English"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    subindex = optContent->index(2, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("Japanese"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    subindex = optContent->index(1, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("French"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Checked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::On);

    // and turning off French should leave them all off
    QVERIFY(optContent->setData(subindex, QVariant(false), Qt::CheckStateRole));

    subindex = optContent->index(0, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("English"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    subindex = optContent->index(2, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("Japanese"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);

    subindex = optContent->index(1, 0, index);
    QCOMPARE(optContent->data(subindex, Qt::DisplayRole).toString(), QLatin1String("French"));
    QCOMPARE(static_cast<Qt::CheckState>(optContent->data(subindex, Qt::CheckStateRole).toInt()), Qt::Unchecked);
    QCOMPARE(static_cast<Poppler::OptContentItem *>(subindex.internalPointer())->group()->getState(), OptionalContentGroup::Off);
}

QTEST_GUILESS_MAIN(TestOptionalContent)

#include "check_optcontent.moc"
