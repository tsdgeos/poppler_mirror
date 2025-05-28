#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>
#include <QtGui/QImage>

#include <iostream>

#include <poppler-qt6.h>

int main(int argc, char **argv)
{
    QApplication a(argc, argv); // QApplication required!

    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QElapsedTimer t;
    t.start();
    QDir dbDir(QStringLiteral("./pdfdb"));
    if (!dbDir.exists()) {
        qWarning() << "Database directory does not exist";
    }

    QStringList excludeSubDirs;
    excludeSubDirs << QStringLiteral("000048") << QStringLiteral("000607");

    const QStringList dirs = dbDir.entryList(QStringList() << QStringLiteral("0000*"), QDir::Dirs);
    Q_FOREACH (const QString &subdir, dirs) {
        if (excludeSubDirs.contains(subdir)) {
            // then skip it
        } else {
            QString path = QStringLiteral("./pdfdb/") + subdir + QStringLiteral("/data.pdf");
            std::cout << "Doing " << path.toLatin1().data() << " :";
            std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(path);
            if (!doc) {
                qWarning() << "doc not loaded";
            } else {
                auto pdfVersion = doc->getPdfVersion();
                Q_UNUSED(pdfVersion);
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
                    std::unique_ptr<Poppler::Page> page = doc->page(index);
                    page->renderToImage();
                    page->pageSize();
                    page->orientation();
                    std::cout << ".";
                    std::cout.flush();
                }
                std::cout << std::endl;
            }
        }
    }

    std::cout << "Elapsed time: " << (t.elapsed() / 1000) << std::endl;
}
