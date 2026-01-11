#include <QtTest/QTest>

#include <poppler-qt5.h>

#include <QtCore/QFile>

class TestAttachments : public QObject
{
    Q_OBJECT
public:
    explicit TestAttachments(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void checkNoAttachments();
    static void checkAttach1();
    static void checkAttach2();
    static void checkAttach3();
    static void checkAttach4();
};

void TestAttachments::checkNoAttachments()
{
    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/truetype.pdf"));
    QVERIFY(doc);

    QCOMPARE(doc->hasEmbeddedFiles(), false);

    delete doc;
}

void TestAttachments::checkAttach1()
{

    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/WithAttachments.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasEmbeddedFiles());

    QList<Poppler::EmbeddedFile *> fileList = doc->embeddedFiles();
    QCOMPARE(fileList.size(), 2);

    Poppler::EmbeddedFile *embfile = fileList.at(0);
    QCOMPARE(embfile->name(), QLatin1String("kroller.png"));
    QCOMPARE(embfile->description(), QString());
    QCOMPARE(embfile->createDate(), QDateTime(QDate(), QTime()));
    QCOMPARE(embfile->modDate(), QDateTime(QDate(), QTime()));
    QCOMPARE(embfile->mimeType(), QString());

    QFile file(QStringLiteral(TESTDATADIR "/unittestcases/kroller.png"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray krollerData = file.readAll();
    QByteArray embdata = embfile->data();
    QCOMPARE(krollerData, embdata);

    Poppler::EmbeddedFile *embfile2 = fileList.at(1);
    QCOMPARE(embfile2->name(), QLatin1String("gnome-64.gif"));
    QCOMPARE(embfile2->description(), QString());
    QCOMPARE(embfile2->modDate(), QDateTime(QDate(), QTime()));
    QCOMPARE(embfile2->createDate(), QDateTime(QDate(), QTime()));
    QCOMPARE(embfile2->mimeType(), QString());

    QFile file2(QStringLiteral(TESTDATADIR "/unittestcases/gnome-64.gif"));
    QVERIFY(file2.open(QIODevice::ReadOnly));
    QByteArray g64Data = file2.readAll();
    QByteArray emb2data = embfile2->data();
    QCOMPARE(g64Data, emb2data);

    delete doc;
}

void TestAttachments::checkAttach2()
{

    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/A6EmbeddedFiles.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasEmbeddedFiles());

    QList<Poppler::EmbeddedFile *> fileList;
    fileList = doc->embeddedFiles();
    QCOMPARE(fileList.size(), 3);

    Poppler::EmbeddedFile *embfile1 = fileList.at(0);
    QCOMPARE(embfile1->name(), QLatin1String("Acro7 thoughts"));
    QCOMPARE(embfile1->description(), QString());
    QCOMPARE(embfile1->createDate(), QDateTime(QDate(2003, 8, 4), QTime(13, 54, 54), Qt::UTC));
    QCOMPARE(embfile1->modDate(), QDateTime(QDate(2003, 8, 4), QTime(14, 15, 27), Qt::UTC));
    QCOMPARE(embfile1->mimeType(), QLatin1String("text/xml"));

    Poppler::EmbeddedFile *embfile2 = fileList.at(1);
    QCOMPARE(embfile2->name(), QLatin1String("acro transitions 1.xls"));
    QCOMPARE(embfile2->description(), QString());
    QCOMPARE(embfile2->createDate(), QDateTime(QDate(2003, 7, 18), QTime(21, 7, 16), Qt::UTC));
    QCOMPARE(embfile2->modDate(), QDateTime(QDate(2003, 7, 22), QTime(13, 4, 40), Qt::UTC));
    QCOMPARE(embfile2->mimeType(), QLatin1String("application/excel"));

    Poppler::EmbeddedFile *embfile3 = fileList.at(2);
    QCOMPARE(embfile3->name(), QLatin1String("apago_pdfe_wide.gif"));
    QCOMPARE(embfile3->description(), QString());
    QCOMPARE(embfile3->createDate(), QDateTime(QDate(2003, 1, 31), QTime(15, 54, 29), Qt::UTC));
    QCOMPARE(embfile3->modDate(), QDateTime(QDate(2003, 1, 31), QTime(15, 52, 58), Qt::UTC));
    QCOMPARE(embfile3->mimeType(), QString());

    delete doc;
}

void TestAttachments::checkAttach3()
{

    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/shapes+attachments.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasEmbeddedFiles());

    QList<Poppler::EmbeddedFile *> fileList;
    fileList = doc->embeddedFiles();
    QCOMPARE(fileList.size(), 1);

    Poppler::EmbeddedFile *embfile = fileList.at(0);
    QCOMPARE(embfile->name(), QLatin1String("ADEX1.xpdf.pgp"));
    QCOMPARE(embfile->description(), QString());
    QCOMPARE(embfile->createDate(), QDateTime(QDate(2004, 3, 29), QTime(19, 37, 16), Qt::UTC));
    QCOMPARE(embfile->modDate(), QDateTime(QDate(2004, 3, 29), QTime(19, 37, 16), Qt::UTC));
    QCOMPARE(embfile->mimeType(), QString());
    delete doc;
}

void TestAttachments::checkAttach4()
{

    Poppler::Document *doc;
    doc = Poppler::Document::load(QStringLiteral(TESTDATADIR "/unittestcases/imageretrieve+attachment.pdf"));
    QVERIFY(doc);

    QVERIFY(doc->hasEmbeddedFiles());

    QList<Poppler::EmbeddedFile *> fileList;
    fileList = doc->embeddedFiles();
    QCOMPARE(fileList.size(), 1);

    Poppler::EmbeddedFile *embfile = fileList.at(0);
    QCOMPARE(embfile->name(), QLatin1String("export-altona.csv"));
    QCOMPARE(embfile->description(), QLatin1String("Altona Export"));
    QCOMPARE(embfile->createDate(), QDateTime(QDate(2005, 8, 30), QTime(20, 49, 35), Qt::UTC));
    QCOMPARE(embfile->modDate(), QDateTime(QDate(2005, 8, 30), QTime(20, 49, 52), Qt::UTC));
    QCOMPARE(embfile->mimeType(), QLatin1String("application/vnd.ms-excel"));
    delete doc;
}

QTEST_GUILESS_MAIN(TestAttachments)
#include "check_attachments.moc"
