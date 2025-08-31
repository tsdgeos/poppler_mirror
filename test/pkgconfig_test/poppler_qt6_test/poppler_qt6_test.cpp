#include <QCoreApplication>
#include <QDebug>
#include <poppler-qt6.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        qWarning() << "usage:" << argv[0] << "file.pdf";
        return 1;
    }

    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(argv[1]));
    if (!doc) {
        qWarning() << "failed to open" << argv[1];
        return 1;
    }

    int n_pages = doc->numPages();
    qDebug() << argv[1] << "has" << n_pages << "pages";

    return 0;
}
