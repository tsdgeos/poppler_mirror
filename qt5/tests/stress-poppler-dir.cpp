#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>
#include <QtGui/QImage>

#include <iostream>

#include <poppler-qt5.h>

int main(int argc, char **argv)
{
    QApplication a(argc, argv); // QApplication required!

    QElapsedTimer t;
    t.start();

    QDir directory(argv[1]);
    foreach (const QString &fileName, directory.entryList()) {
        if (fileName.endsWith(QStringLiteral("pdf"))) {
            qDebug() << "Doing" << fileName.toLatin1().data() << ":";
            Poppler::Document *doc = Poppler::Document::load(directory.canonicalPath() + "/" + fileName);
            if (!doc) {
                qWarning() << "doc not loaded";
            } else if (doc->isLocked()) {
                if (!doc->unlock("", "password")) {
                    qWarning() << "couldn't unlock document";
                    delete doc;
                }
            } else {
                auto pdfVersion = doc->getPdfVersion();
                if (pdfVersion.major != 1) {
                    qWarning() << "pdf major version is not '1'";
                }

                doc->info(QStringLiteral("Title"));
                doc->info(QStringLiteral("Subject"));
                doc->info(QStringLiteral("Author"));
                doc->info(QStringLiteral("Keywords"));
                doc->info(QStringLiteral("Creator"));
                doc->info(QStringLiteral("Producer"));
                doc->date(QStringLiteral("CreationDate")).toString();
                doc->date(QStringLiteral("ModDate")).toString();
                doc->numPages();
                doc->isLinearized();
                doc->isEncrypted();
                doc->okToPrint();
                doc->okToCopy();
                doc->okToChange();
                doc->okToAddNotes();
                doc->pageMode();

                for (int index = 0; index < doc->numPages(); ++index) {
                    Poppler::Page *page = doc->page(index);
                    page->renderToImage();
                    page->pageSize();
                    page->orientation();
                    delete page;
                    std::cout << ".";
                    std::cout.flush();
                }
                std::cout << std::endl;
                delete doc;
            }
        }
    }

    std::cout << "Elapsed time: " << (t.elapsed() / 1000) << "seconds" << std::endl;
}
