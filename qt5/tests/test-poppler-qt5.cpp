#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtWidgets/QApplication>
#include <QtGui/QImage>
#include <QtWidgets/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QWidget>

#include <poppler-qt5.h>

class PDFDisplay : public QWidget // picture display widget
{
    Q_OBJECT
public:
    PDFDisplay(Poppler::Document *d, bool qpainter, QWidget *parent = nullptr);
    ~PDFDisplay() override;
    void setShowTextRects(bool show);
    void display();

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    int m_currentPage;
    QImage image;
    Poppler::Document *doc;
    QString backendString;
    bool showTextRects;
    QList<Poppler::TextBox *> textRects;
};

PDFDisplay::PDFDisplay(Poppler::Document *d, bool qpainter, QWidget *parent) : QWidget(parent)
{
    showTextRects = false;
    doc = d;
    m_currentPage = 0;
    if (qpainter) {
        backendString = QStringLiteral("QPainter");
        doc->setRenderBackend(Poppler::Document::QPainterBackend);
    } else {
        backendString = QStringLiteral("Splash");
        doc->setRenderBackend(Poppler::Document::SplashBackend);
    }
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
}

void PDFDisplay::setShowTextRects(bool show)
{
    showTextRects = show;
}

void PDFDisplay::display()
{
    if (doc) {
        Poppler::Page *page = doc->page(m_currentPage);
        if (page) {
            qDebug() << "Displaying page using" << backendString << "backend: " << m_currentPage;
            QTime t = QTime::currentTime();
            image = page->renderToImage();
            qDebug() << "Rendering took" << t.msecsTo(QTime::currentTime()) << "msecs";
            qDeleteAll(textRects);
            if (showTextRects) {
                QPainter painter(&image);
                painter.setPen(Qt::red);
                textRects = page->textList();
                foreach (Poppler::TextBox *tb, textRects) {
                    painter.drawRect(tb->boundingBox());
                }
            } else {
                textRects.clear();
            }
            update();
            delete page;
        }
    } else {
        qWarning() << "doc not loaded";
    }
}

PDFDisplay::~PDFDisplay()
{
    qDeleteAll(textRects);
    delete doc;
}

void PDFDisplay::paintEvent(QPaintEvent *e)
{
    QPainter paint(this); // paint widget
    if (!image.isNull()) {
        paint.drawImage(0, 0, image);
    } else {
        qWarning() << "null image";
    }
}

void PDFDisplay::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Down) {
        if (m_currentPage + 1 < doc->numPages()) {
            m_currentPage++;
            display();
        }
    } else if (e->key() == Qt::Key_Up) {
        if (m_currentPage > 0) {
            m_currentPage--;
            display();
        }
    } else if (e->key() == Qt::Key_Q) {
        exit(0);
    }
}

void PDFDisplay::mousePressEvent(QMouseEvent *e)
{
    int i = 0;
    foreach (Poppler::TextBox *tb, textRects) {
        if (tb->boundingBox().contains(e->pos())) {
            const QString tt = QStringLiteral("Text: \"%1\"\nIndex in text list: %2").arg(tb->text()).arg(i);
            QToolTip::showText(e->globalPos(), tt, this);
            break;
        }
        ++i;
    }
}

int main(int argc, char **argv)
{
    QApplication a(argc, argv); // QApplication required!

    if (argc < 2 || (argc == 3 && strcmp(argv[2], "-extract") != 0 && strcmp(argv[2], "-qpainter") != 0 && strcmp(argv[2], "-textRects") != 0) || argc > 3) {
        // use argument as file name
        qWarning() << "usage: test-poppler-qt5 filename [-extract|-qpainter|-textRects]";
        exit(1);
    }

    Poppler::Document *doc = Poppler::Document::load(QFile::decodeName(argv[1]));
    if (!doc) {
        qWarning() << "doc not loaded";
        exit(1);
    }

    if (doc->isLocked()) {
        qWarning() << "document locked (needs password)";
        exit(0);
    }

    // output some meta-data
    Poppler::Document::PdfVersion pdfVersion = doc->getPdfVersion();
    qDebug() << "    PDF Version: " << qPrintable(QStringLiteral("%1.%2").arg(pdfVersion.major).arg(pdfVersion.minor));
    qDebug() << "          Title: " << doc->info(QStringLiteral("Title"));
    qDebug() << "        Subject: " << doc->info(QStringLiteral("Subject"));
    qDebug() << "         Author: " << doc->info(QStringLiteral("Author"));
    qDebug() << "      Key words: " << doc->info(QStringLiteral("Keywords"));
    qDebug() << "        Creator: " << doc->info(QStringLiteral("Creator"));
    qDebug() << "       Producer: " << doc->info(QStringLiteral("Producer"));
    qDebug() << "   Date created: " << doc->date(QStringLiteral("CreationDate")).toString();
    qDebug() << "  Date modified: " << doc->date(QStringLiteral("ModDate")).toString();
    qDebug() << "Number of pages: " << doc->numPages();
    qDebug() << "     Linearised: " << doc->isLinearized();
    qDebug() << "      Encrypted: " << doc->isEncrypted();
    qDebug() << "    OK to print: " << doc->okToPrint();
    qDebug() << "     OK to copy: " << doc->okToCopy();
    qDebug() << "   OK to change: " << doc->okToChange();
    qDebug() << "OK to add notes: " << doc->okToAddNotes();
    qDebug() << "      Page mode: " << doc->pageMode();
    qDebug() << "       Metadata: " << doc->metadata();

    if (doc->hasEmbeddedFiles()) {
        qDebug() << "Embedded files:";
        foreach (Poppler::EmbeddedFile *file, doc->embeddedFiles()) {
            qDebug() << "   " << file->name();
        }
        qDebug();
    } else {
        qDebug() << "No embedded files";
    }

    if (doc->numPages() <= 0) {
        delete doc;
        qDebug() << "Doc has no pages";
        return 0;
    }

    {
        Poppler::Page *page = doc->page(0);
        if (page) {
            qDebug() << "Page 1 size: " << page->pageSize().width() / 72 << "inches x " << page->pageSize().height() / 72 << "inches";
            delete page;
        }
    }

    if (argc == 2 || (argc == 3 && strcmp(argv[2], "-qpainter") == 0) || (argc == 3 && strcmp(argv[2], "-textRects") == 0)) {
        bool useQPainter = (argc == 3 && strcmp(argv[2], "-qpainter") == 0);
        PDFDisplay test(doc, useQPainter); // create picture display
        test.setWindowTitle(QStringLiteral("Poppler-Qt5 Test"));
        test.setShowTextRects(argc == 3 && strcmp(argv[2], "-textRects") == 0);
        test.display();
        test.show(); // show it

        return QApplication::exec(); // start event loop
    } else {
        Poppler::Page *page = doc->page(0);

        QLabel *l = new QLabel(page->text(QRectF()), nullptr);
        l->show();
        delete page;
        delete doc;
        return QApplication::exec();
    }
}

#include "test-poppler-qt5.moc"
