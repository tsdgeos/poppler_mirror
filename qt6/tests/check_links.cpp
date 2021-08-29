#include <QtTest/QtTest>

#include <poppler-qt6.h>

#include <memory>

class TestLinks : public QObject
{
    Q_OBJECT
public:
    explicit TestLinks(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void checkDocumentWithNoDests();
    void checkDests_xr01();
    void checkDests_xr02();
    void checkDocumentURILink();
};

static bool isDestinationValid_pageNumber(const Poppler::LinkDestination *dest, const Poppler::Document *doc)
{
    return dest->pageNumber() > 0 && dest->pageNumber() <= doc->numPages();
}

static bool isDestinationValid_name(const Poppler::LinkDestination *dest)
{
    return !dest->destinationName().isEmpty();
}

void TestLinks::checkDocumentWithNoDests()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/WithAttachments.pdf");
    QVERIFY(doc);

    std::unique_ptr<Poppler::LinkDestination> dest = doc->linkDestination(QStringLiteral("no.dests.in.this.document"));
    QVERIFY(!isDestinationValid_pageNumber(dest.get(), doc.get()));
    QVERIFY(isDestinationValid_name(dest.get()));
}

void TestLinks::checkDests_xr01()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/xr01.pdf");
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(0);
    QVERIFY(page);

    std::vector<std::unique_ptr<Poppler::Link>> links = page->links();
    QCOMPARE(links.size(), 2);

    {
        QCOMPARE(links.at(0)->linkType(), Poppler::Link::Goto);
        Poppler::LinkGoto *link = static_cast<Poppler::LinkGoto *>(links.at(0).get());
        const Poppler::LinkDestination dest = link->destination();
        QVERIFY(!isDestinationValid_pageNumber(&dest, doc.get()));
        QVERIFY(isDestinationValid_name(&dest));
        QCOMPARE(dest.destinationName(), QLatin1String("section.1"));
    }

    {
        QCOMPARE(links.at(1)->linkType(), Poppler::Link::Goto);
        Poppler::LinkGoto *link = static_cast<Poppler::LinkGoto *>(links.at(1).get());
        const Poppler::LinkDestination dest = link->destination();
        QVERIFY(!isDestinationValid_pageNumber(&dest, doc.get()));
        QVERIFY(isDestinationValid_name(&dest));
        QCOMPARE(dest.destinationName(), QLatin1String("section.2"));
    }
}

void TestLinks::checkDests_xr02()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/xr02.pdf");
    QVERIFY(doc);

    std::unique_ptr<Poppler::LinkDestination> dest = doc->linkDestination(QStringLiteral("section.1"));
    QVERIFY(isDestinationValid_pageNumber(dest.get(), doc.get()));
    QVERIFY(!isDestinationValid_name(dest.get()));
    dest = doc->linkDestination(QStringLiteral("section.2"));
    QVERIFY(isDestinationValid_pageNumber(dest.get(), doc.get()));
    QVERIFY(!isDestinationValid_name(dest.get()));
    dest = doc->linkDestination(QStringLiteral("section.3"));
    QVERIFY(!isDestinationValid_pageNumber(dest.get(), doc.get()));
    QVERIFY(isDestinationValid_name(dest.get()));
}

void TestLinks::checkDocumentURILink()
{
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(TESTDATADIR "/unittestcases/checkbox_issue_159.pdf");
    QVERIFY(doc);

    std::unique_ptr<Poppler::Page> page = doc->page(0);
    QVERIFY(page);

    std::vector<std::unique_ptr<Poppler::Link>> links = page->links();
    QCOMPARE(links.size(), 1);

    QCOMPARE(links.at(0)->linkType(), Poppler::Link::Browse);
    Poppler::LinkBrowse *link = static_cast<Poppler::LinkBrowse *>(links.at(0).get());
    QCOMPARE(link->url(), QLatin1String("http://www.tcpdf.org"));
}

QTEST_GUILESS_MAIN(TestLinks)

#include "check_links.moc"
