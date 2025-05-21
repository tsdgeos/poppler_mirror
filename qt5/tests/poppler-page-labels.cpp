#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <iostream>
#include <memory>

#include <poppler-qt5.h>

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv); // QApplication required!

    if (!(argc == 2)) {
        qWarning() << "usage: poppler-page-labels filename";
        exit(1);
    }

    Poppler::Document *doc = Poppler::Document::load(QString::fromLocal8Bit(argv[1]));
    if (!doc || doc->isLocked()) {
        qWarning() << "doc not loaded";
        exit(1);
    }

    for (int i = 0; i < doc->numPages(); i++) {
        int j = 0;
        std::cout << "*** Label of Page " << i << std::endl;
        std::cout << std::flush;

        std::unique_ptr<Poppler::Page> page(doc->page(i));

        if (!page) {
            continue;
        }

        const QByteArray utf8str = page->label().toUtf8();
        for (j = 0; j < utf8str.size(); j++) {
            std::cout << utf8str[j];
        }
        std::cout << std::endl;

        std::unique_ptr<Poppler::Page> pageFromPageLabel(doc->page(page->label()));
        const int indexFromPageLabel = pageFromPageLabel ? pageFromPageLabel->index() : -1;
        if (indexFromPageLabel != i) {
            std::cout << "WARNING: Page label didn't link back to the same page index " << indexFromPageLabel << " " << i << std::endl;
        }
    }
    delete doc;
}
