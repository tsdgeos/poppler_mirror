#include <QtTest/QTest>
#include <QtCore/QTemporaryFile>

#include "Outline.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"

class TestInternalOutline : public QObject
{
    Q_OBJECT
public:
    explicit TestInternalOutline(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void testCreateOutline();
    void testSetOutline();
    void testInsertChild();
    void testRemoveChild();
    void testSetTitleAndSetPageDest();
};

void TestInternalOutline::testCreateOutline()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    const std::string tempFileName = tempFile.fileName().toStdString();
    const GooString gooTempFileName { tempFileName };

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(GooString(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc.get());

    // ensure the file has no existing outline
    Outline *outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    auto *outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);

    // create an empty outline and save the file
    outline->setOutline({});
    outlineItems = outline->getItems();
    // no items will result in a nullptr rather than a 0 length list
    QVERIFY(outlineItems == nullptr);
    doc->saveAs(gooTempFileName);

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName);
    QVERIFY(doc.get());

    // ensure the re-opened file has an outline with no items
    outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);
}

static std::string getTitle(const OutlineItem *item)
{
    std::vector<Unicode> u = item->getTitle();
    std::string s;
    for (auto &c : u) {
        s.append(1, (char)(c));
    }
    return s;
}

void TestInternalOutline::testSetOutline()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    const std::string tempFileName = tempFile.fileName().toStdString();
    const GooString gooTempFileName { tempFileName };

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(GooString(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc.get());

    // ensure the file has no existing outline
    Outline *outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    auto *outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);

    // create an outline and save the file
    outline->setOutline(
            { { "1", 1, { { "1.1", 1, {} }, { "1.2", 2, {} }, { "1.3", 3, { { "1.3.1", 1, {} }, { "1.3.2", 2, {} }, { "1.3.3", 3, {} }, { "1.3.4", 4, {} } } }, { "1.4", 4, {} } } }, { "2", 2, {} }, { "3", 3, {} }, { "4", 4, {} } });
    outlineItems = outline->getItems();
    QVERIFY(outlineItems != nullptr);
    doc->saveAs(gooTempFileName);
    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName);
    QVERIFY(doc.get());

    // ensure the re-opened file has an outline
    outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    outlineItems = outline->getItems();

    QVERIFY(outlineItems != nullptr);
    QVERIFY(outlineItems->size() == 4);

    OutlineItem *item = outlineItems->at(0);
    QVERIFY(item != nullptr);

    // c_str() is used so QCOMPARE prints string correctly on disagree
    QCOMPARE(getTitle(item).c_str(), "1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "3");
    item = outlineItems->at(3);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "4");

    outlineItems = outlineItems->at(0)->getKids();
    QVERIFY(outlineItems != nullptr);
    item = outlineItems->at(0);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3");
    item = outlineItems->at(3);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.4");

    outlineItems = outlineItems->at(2)->getKids();
    QVERIFY(outlineItems != nullptr);

    item = outlineItems->at(0);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.3");
    item = outlineItems->at(3);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.4");
}

void TestInternalOutline::testInsertChild()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();
    QTemporaryFile tempFile2;
    QVERIFY(tempFile2.open());
    tempFile2.close();

    const std::string tempFileName = tempFile.fileName().toStdString();
    const GooString gooTempFileName { tempFileName };
    const std::string tempFileName2 = tempFile2.fileName().toStdString();
    const GooString gooTempFileName2 { tempFileName2 };

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(GooString(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc.get());

    // ensure the file has no existing outline
    Outline *outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    auto *outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);

    // create an outline and save the file
    outline->setOutline({});
    doc->saveAs(gooTempFileName);
    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName);
    QVERIFY(doc.get());

    // ensure the re-opened file has an outline with no items
    outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    // nullptr for 0-length
    QVERIFY(outline->getItems() == nullptr);

    // insert first one to empty
    outline->insertChild("2", 1, 0);
    // insert at the end
    outline->insertChild("3", 1, 1);
    // insert at the start
    outline->insertChild("1", 1, 0);

    // add an item to "2"
    outlineItems = outline->getItems();
    QVERIFY(outlineItems != nullptr);
    QVERIFY(outlineItems->at(1));
    outlineItems->at(1)->insertChild("2.1", 2, 0);
    outlineItems->at(1)->insertChild("2.2", 2, 1);
    outlineItems->at(1)->insertChild("2.4", 2, 2);

    outlineItems->at(1)->insertChild("2.3", 2, 2);

    // save the file
    doc->saveAs(gooTempFileName2);
    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName2);
    QVERIFY(doc.get());

    // ensure the re-opened file has an outline
    outline = doc->getOutline();
    QVERIFY(outline != nullptr);

    outlineItems = outline->getItems();

    QVERIFY(outlineItems != nullptr);
    QVERIFY(outlineItems->size() == 3);

    OutlineItem *item = outlineItems->at(0);
    QVERIFY(item != nullptr);

    // c_str() is used so QCOMPARE prints string correctly on disagree
    QCOMPARE(getTitle(item).c_str(), "1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "3");

    outlineItems = outlineItems->at(1)->getKids();
    item = outlineItems->at(0);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2.1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2.2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2.3");
    item = outlineItems->at(3);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2.4");
}

void TestInternalOutline::testRemoveChild()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    QTemporaryFile tempFile2;
    QVERIFY(tempFile2.open());
    tempFile2.close();

    const std::string tempFileName = tempFile.fileName().toStdString();
    const GooString gooTempFileName { tempFileName };
    const std::string tempFileName2 = tempFile2.fileName().toStdString();
    const GooString gooTempFileName2 { tempFileName2 };

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(GooString(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc.get());

    // ensure the file has no existing outline
    Outline *outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    auto *outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);

    // create an outline and save the file
    outline->setOutline({ { "1", 1, { { "1.1", 1, {} }, { "1.2", 2, {} }, { "1.3", 3, { { "1.3.1", 1, {} }, { "1.3.2", 2, {} }, { "1.3.3", 3, {} }, { "1.3.4", 4, {} } } }, { "1.4", 4, {} } } },
                          { "2", 2, { { "2.1", 1, {} } } },
                          { "3", 3, { { "3.1", 1, {} }, { "3.2", 2, { { "3.2.1", 1, {} } } } } },
                          { "4", 4, {} } });
    outlineItems = outline->getItems();
    QVERIFY(outlineItems != nullptr);
    doc->saveAs(gooTempFileName);
    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName);
    QVERIFY(doc.get());

    outline = doc->getOutline();
    QVERIFY(outline != nullptr);

    // remove "3"
    outline->removeChild(2);
    // remove "1.3.1"
    outline->getItems()->at(0)->getKids()->at(2)->removeChild(0);
    // remove "1.3.4"
    outline->getItems()->at(0)->getKids()->at(2)->removeChild(2);
    // remove "2.1"
    outline->getItems()->at(1)->removeChild(0);

    // save the file
    doc->saveAs(gooTempFileName2);
    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName2);
    QVERIFY(doc.get());

    // ensure the re-opened file has an outline
    outline = doc->getOutline();
    QVERIFY(outline != nullptr);

    outlineItems = outline->getItems();

    QVERIFY(outlineItems != nullptr);
    QVERIFY(outlineItems->size() == 3);

    OutlineItem *item = outlineItems->at(0);
    QVERIFY(item != nullptr);

    // c_str() is used so QCOMPARE prints string correctly on disagree
    QCOMPARE(getTitle(item).c_str(), "1");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "2");
    item = outlineItems->at(2);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "4");

    outlineItems = outlineItems->at(0)->getKids();
    outlineItems = outlineItems->at(2)->getKids();
    item = outlineItems->at(0);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.2");
    item = outlineItems->at(1);
    QVERIFY(item != nullptr);
    QCOMPARE(getTitle(item).c_str(), "1.3.3");

    // verify "2.1" is removed, lst length 0 is returned as a nullptr
    QVERIFY(outline->getItems()->at(1)->getKids() == nullptr);
}

void TestInternalOutline::testSetTitleAndSetPageDest()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    QTemporaryFile tempFile2;
    QVERIFY(tempFile2.open());
    tempFile2.close();

    const std::string tempFileName = tempFile.fileName().toStdString();
    const GooString gooTempFileName { tempFileName };
    const std::string tempFileName2 = tempFile2.fileName().toStdString();
    const GooString gooTempFileName2 { tempFileName2 };

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(GooString(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc.get());

    // ensure the file has no existing outline
    Outline *outline = doc->getOutline();
    QVERIFY(outline != nullptr);
    auto *outlineItems = outline->getItems();
    QVERIFY(outlineItems == nullptr);

    // create an outline and save the file
    outline->setOutline({ { "1", 1, { { "1.1", 1, {} }, { "1.2", 2, {} }, { "1.3", 3, { { "1.3.1", 1, {} }, { "1.3.2", 2, {} }, { "1.3.3", 3, {} }, { "1.3.4", 4, {} } } }, { "1.4", 4, {} } } },
                          { "2", 2, { { "2.1", 1, {} } } },
                          { "3", 3, { { "3.1", 1, {} }, { "3.2", 2, { { "3.2.1", 1, {} } } } } },
                          { "4", 4, {} } });
    outlineItems = outline->getItems();
    QVERIFY(outlineItems != nullptr);
    doc->saveAs(gooTempFileName);

    outline = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName);
    QVERIFY(doc.get());

    outline = doc->getOutline();
    QVERIFY(outline != nullptr);

    // change "1.3.1"
    OutlineItem *item = outline->getItems()->at(0)->getKids()->at(2)->getKids()->at(0);
    QCOMPARE(getTitle(item).c_str(), "1.3.1");

    item->setTitle("Changed to a different title");

    item = outline->getItems()->at(2);
    {
        const LinkAction *action = item->getAction();
        QVERIFY(action->getKind() == actionGoTo);
        const LinkGoTo *gotoAction = dynamic_cast<const LinkGoTo *>(action);
        const LinkDest *dest = gotoAction->getDest();
        QVERIFY(dest->isPageRef() == false);
        QCOMPARE(dest->getPageNum(), 3);

        item->setPageDest(1);
    }

    // save the file
    doc->saveAs(gooTempFileName2);
    outline = nullptr;
    item = nullptr;

    /******************************************************/

    doc = PDFDocFactory().createPDFDoc(gooTempFileName2);
    QVERIFY(doc.get());

    outline = doc->getOutline();
    QVERIFY(outline != nullptr);

    item = outline->getItems()->at(0)->getKids()->at(2)->getKids()->at(0);
    QCOMPARE(getTitle(item).c_str(), "Changed to a different title");
    {
        item = outline->getItems()->at(2);
        const LinkAction *action = item->getAction();
        QVERIFY(action->getKind() == actionGoTo);
        const LinkGoTo *gotoAction = dynamic_cast<const LinkGoTo *>(action);
        const LinkDest *dest = gotoAction->getDest();
        QVERIFY(dest->isPageRef() == false);
        QCOMPARE(dest->getPageNum(), 1);
    }
}

QTEST_GUILESS_MAIN(TestInternalOutline)
#include "check_internal_outline.moc"
