#include <QtCore/QDebug>
#include <QtWidgets/QApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QWidget>

#include <poppler-qt6.h>

class PDFDisplay : public QWidget // picture display widget
{
    Q_OBJECT
public:
    explicit PDFDisplay(std::unique_ptr<Poppler::Document> &&d, QWidget *parent = nullptr);
    ~PDFDisplay() override;

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    void display();
    int m_currentPage;
    QImage image;
    std::unique_ptr<Poppler::Document> doc;
};

PDFDisplay::PDFDisplay(std::unique_ptr<Poppler::Document> &&d, QWidget *parent) : QWidget(parent)
{
    doc = std::move(d);
    m_currentPage = 0;
    display();
}

void PDFDisplay::display()
{
    if (doc) {
        std::unique_ptr<Poppler::Page> page = doc->page(m_currentPage);
        if (page) {
            qDebug() << "Displaying page: " << m_currentPage;
            image = page->renderToImage();
            update();
        }
    } else {
        qWarning() << "doc not loaded";
    }
}

PDFDisplay::~PDFDisplay() { }

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

int main(int argc, char **argv)
{
    QApplication a(argc, argv); // QApplication required!

    if (argc != 3) {
        qWarning() << "usage: test-password-qt6 owner-password filename";
        exit(1);
    }

    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(argv[2], argv[1]);
    if (!doc) {
        qWarning() << "doc not loaded";
        exit(1);
    }

    // output some meta-data
    auto pdfVersion = doc->getPdfVersion();
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
    QStringList fontNameList;
    foreach (const Poppler::FontInfo &font, doc->fonts())
        fontNameList += font.name();
    qDebug() << "          Fonts: " << fontNameList.join(QStringLiteral(", "));

    std::unique_ptr<Poppler::Page> page = doc->page(0);
    qDebug() << "    Page 1 size: " << page->pageSize().width() / 72 << "inches x " << page->pageSize().height() / 72 << "inches";

    PDFDisplay test(std::move(doc)); // create picture display
    test.setWindowTitle(QStringLiteral("Poppler-Qt6 Test"));
    test.show(); // show it

    return a.exec(); // start event loop
}

#include "test-password-qt6.moc"
