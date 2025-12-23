#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <iostream>
#include <string>

#include <poppler-qt6.h>

int main(int argc, char **argv)
{
    using namespace std::string_literals;
    QCoreApplication a(argc, argv); // QApplication required!

    if ((argc < 2) || (argc > 3)) {
        qWarning() << "usage: poppler-texts [-r|-p|-f] filename";
        exit(1);
    }
    auto layout = Poppler::Page::RawOrderLayout;
    if (argc == 3) {
        if (argv[1] == "-r"s) {
            layout = Poppler::Page::RawOrderLayout;
        } else if (argv[1] == "-p"s) {
            layout = Poppler::Page::PhysicalLayout;
        } else if (argv[1] == "-f"s) {
            layout = Poppler::Page::ReadingOrder;
        } else {
            qWarning() << "usage: poppler-texts [-r|-p|-f] filename";
            exit(1);
        }
    }

    std::unique_ptr<Poppler::Document> doc = Poppler::Document::load(QString::fromLocal8Bit(argv[argc - 1]));
    if (!doc) {
        qWarning() << "doc not loaded";
        exit(1);
    }

    for (int i = 0; i < doc->numPages(); i++) {
        int j = 0;
        std::cout << "*** Page " << i << std::endl;
        std::cout << std::flush;

        std::unique_ptr<Poppler::Page> page = doc->page(i);
        const QByteArray utf8str = page->text(QRectF(), layout).toUtf8();
        std::cout << std::flush;
        for (j = 0; j < utf8str.size(); j++) {
            std::cout << utf8str[j];
        }
        std::cout << std::endl;
    }
}
