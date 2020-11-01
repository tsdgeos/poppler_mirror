#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QGuiApplication>
#include <QImage>

#include <poppler-qt5.h>

int main(int argc, char **argv)
{
    QGuiApplication a(argc, argv); // QApplication required!

    if (argc < 2 || (argc == 3 && strcmp(argv[2], "-qpainter") != 0) || argc > 3) {
        // use argument as file name
        qWarning() << "usage: test-render-to-file-qt5 filename [-qpainter]";
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

    if (doc->numPages() <= 0) {
        delete doc;
        qDebug() << "Doc has no pages";
        return 0;
    }

    QString backendString;
    if (argc == 3 && strcmp(argv[2], "-qpainter") == 0) {
        backendString = QStringLiteral("QPainter");
        doc->setRenderBackend(Poppler::Document::QPainterBackend);
    } else {
        backendString = QStringLiteral("Splash");
        doc->setRenderBackend(Poppler::Document::SplashBackend);
    }
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);

    for (int i = 0; i < doc->numPages(); ++i) {
        Poppler::Page *page = doc->page(i);
        if (page) {
            qDebug() << "Rendering page using" << backendString << "backend: " << i;
            QTime t = QTime::currentTime();
            QImage image = page->renderToImage();
            qDebug() << "Rendering took" << t.msecsTo(QTime::currentTime()) << "msecs";
            image.save(QStringLiteral("test-render-to-file%1.png").arg(i));
            delete page;
        }
    }

    return 0;
}
