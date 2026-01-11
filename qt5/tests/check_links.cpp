#include <QtTest/QTest>

#include <poppler-qt5.h>

#include <memory>

class TestLinks : public QObject
{
    Q_OBJECT
public:
    explicit TestLinks(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkDocumentWithNoDests();
    static void checkDests_xr01();
    static void checkDests_xr02();
    static void checkDocumentURILink();
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
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/WithAttachments.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::LinkDestination> dest;
    dest.reset(doc->linkDestination(QStringLiteral("no.dests.in.this.document")));
    QVERIFY(!isDestinationValid_pageNumber(dest.get(), doc));
    QVERIFY(isDestinationValid_name(dest.get()));

    delete doc;
}

void TestLinks::checkDests_xr01()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/xr01.pdf"));
    QVERIFY(doc);

    Poppler::Page *page = doc->page(0);
    QVERIFY(page);

    QList<Poppler::Link *> links = page->links();
    QCOMPARE(links.count(), 2);

    {
        QCOMPARE(links.at(0)->linkType(), Poppler::Link::Goto);
        auto *link = static_cast<Poppler::LinkGoto *>(links.at(0));
        const Poppler::LinkDestination dest = link->destination();
        QVERIFY(!isDestinationValid_pageNumber(&dest, doc));
        QVERIFY(isDestinationValid_name(&dest));
        QCOMPARE(dest.destinationName(), QLatin1String("section.1"));
    }

    {
        QCOMPARE(links.at(1)->linkType(), Poppler::Link::Goto);
        auto *link = static_cast<Poppler::LinkGoto *>(links.at(1));
        const Poppler::LinkDestination dest = link->destination();
        QVERIFY(!isDestinationValid_pageNumber(&dest, doc));
        QVERIFY(isDestinationValid_name(&dest));
        QCOMPARE(dest.destinationName(), QLatin1String("section.2"));
    }

    qDeleteAll(links);
    delete page;
    delete doc;
}

void TestLinks::checkDests_xr02()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/xr02.pdf"));
    QVERIFY(doc);

    std::unique_ptr<Poppler::LinkDestination> dest;
    dest.reset(doc->linkDestination(QStringLiteral("section.1")));
    QVERIFY(isDestinationValid_pageNumber(dest.get(), doc));
    QVERIFY(!isDestinationValid_name(dest.get()));
    dest.reset(doc->linkDestination(QStringLiteral("section.2")));
    QVERIFY(isDestinationValid_pageNumber(dest.get(), doc));
    QVERIFY(!isDestinationValid_name(dest.get()));
    dest.reset(doc->linkDestination(QStringLiteral("section.3")));
    QVERIFY(!isDestinationValid_pageNumber(dest.get(), doc));
    QVERIFY(isDestinationValid_name(dest.get()));

    delete doc;
}

void TestLinks::checkDocumentURILink()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/checkbox_issue_159.pdf"));
    QVERIFY(doc);

    Poppler::Page *page = doc->page(0);
    QVERIFY(page);

    QList<Poppler::Link *> links = page->links();
    QCOMPARE(links.count(), 1);

    QCOMPARE(links.at(0)->linkType(), Poppler::Link::Browse);
    auto *link = static_cast<Poppler::LinkBrowse *>(links.at(0));
    QCOMPARE(link->url(), QLatin1String("http://www.tcpdf.org"));

    qDeleteAll(links);
    delete page;
    delete doc;
}

QTEST_GUILESS_MAIN(TestLinks)

#include "check_links.moc"
