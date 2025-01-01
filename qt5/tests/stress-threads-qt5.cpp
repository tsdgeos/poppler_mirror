
#ifndef _WIN32
#    include <unistd.h>
#else
#    include <windows.h>
#    define sleep Sleep
#endif
#include <ctime>

#include <poppler-qt5.h>
#include <poppler-form.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtGui/QImage>

class SillyThread : public QThread
{
    Q_OBJECT
public:
    explicit SillyThread(Poppler::Document *document, QObject *parent = nullptr);

    void run() override;

private:
    Poppler::Document *m_document;
    QVector<Poppler::Page *> m_pages;
};

class CrazyThread : public QThread
{
    Q_OBJECT
public:
    CrazyThread(uint seed, Poppler::Document *document, QMutex *annotationMutex, QObject *parent = nullptr);

    void run() override;

private:
    uint m_seed;
    Poppler::Document *m_document;
    QMutex *m_annotationMutex;
};

static Poppler::Page *loadPage(Poppler::Document *document, int index)
{
    Poppler::Page *page = document->page(index);

    if (page == nullptr) {
        qDebug() << "!Document::page";

        exit(EXIT_FAILURE);
    }

    return page;
}

static Poppler::Page *loadRandomPage(Poppler::Document *document)
{
    return loadPage(document, qrand() % document->numPages());
}

SillyThread::SillyThread(Poppler::Document *document, QObject *parent) : QThread(parent), m_document(document)
{
    m_pages.reserve(m_document->numPages());

    for (int index = 0; index < m_document->numPages(); ++index) {
        m_pages.append(loadPage(m_document, index));
    }
}

void SillyThread::run()
{
    forever {
        foreach (Poppler::Page *page, m_pages) {
            QImage image = page->renderToImage();

            if (image.isNull()) {
                qDebug() << "!Page::renderToImage";

                ::exit(EXIT_FAILURE);
            }
        }
    }
}

CrazyThread::CrazyThread(uint seed, Poppler::Document *document, QMutex *annotationMutex, QObject *parent) : QThread(parent), m_seed(seed), m_document(document), m_annotationMutex(annotationMutex) { }

void CrazyThread::run()
{
    typedef QScopedPointer<Poppler::Page> PagePointer;

    qsrand(m_seed);

    forever {
        if (qrand() % 2 == 0) {
            qDebug() << "search...";

            PagePointer page(loadRandomPage(m_document));

            page->search(QStringLiteral("c"), Poppler::Page::IgnoreCase);
            page->search(QStringLiteral("r"));
            page->search(QStringLiteral("a"), Poppler::Page::IgnoreCase);
            page->search(QStringLiteral("z"));
            page->search(QStringLiteral("y"), Poppler::Page::IgnoreCase);
        }

        if (qrand() % 2 == 0) {
            qDebug() << "links...";

            PagePointer page(loadRandomPage(m_document));

            QList<Poppler::Link *> links = page->links();

            qDeleteAll(links);
        }

        if (qrand() % 2 == 0) {
            qDebug() << "form fields...";

            PagePointer page(loadRandomPage(m_document));

            QList<Poppler::FormField *> formFields = page->formFields();

            qDeleteAll(formFields);
        }

        if (qrand() % 2 == 0) {
            qDebug() << "thumbnail...";

            PagePointer page(loadRandomPage(m_document));

            page->thumbnail();
        }

        if (qrand() % 2 == 0) {
            qDebug() << "text...";

            PagePointer page(loadRandomPage(m_document));

            page->text(QRectF(QPointF(), page->pageSizeF()));
        }

        if (qrand() % 2 == 0) {
            QMutexLocker mutexLocker(m_annotationMutex);

            qDebug() << "add annotation...";

            PagePointer page(loadRandomPage(m_document));

            Poppler::Annotation *annotation = nullptr;

            switch (qrand() % 3) {
            default:
            case 0:
                annotation = new Poppler::TextAnnotation(qrand() % 2 == 0 ? Poppler::TextAnnotation::Linked : Poppler::TextAnnotation::InPlace);
                break;
            case 1:
                annotation = new Poppler::HighlightAnnotation();
                break;
            case 2:
                annotation = new Poppler::InkAnnotation();
                break;
            }

            annotation->setBoundary(QRectF(0.0, 0.0, 0.5, 0.5));
            annotation->setContents(QStringLiteral("crazy"));

            page->addAnnotation(annotation);

            delete annotation;
        }

        if (qrand() % 2 == 0) {
            QMutexLocker mutexLocker(m_annotationMutex);

            for (int index = 0; index < m_document->numPages(); ++index) {
                PagePointer page(loadPage(m_document, index));

                QList<Poppler::Annotation *> annotations = page->annotations();

                if (!annotations.isEmpty()) {
                    qDebug() << "modify annotation...";

                    annotations.at(qrand() % annotations.size())->setBoundary(QRectF(0.5, 0.5, 0.25, 0.25));
                    annotations.at(qrand() % annotations.size())->setAuthor(QStringLiteral("foo"));
                    annotations.at(qrand() % annotations.size())->setContents(QStringLiteral("bar"));
                    annotations.at(qrand() % annotations.size())->setCreationDate(QDateTime::currentDateTime());
                    annotations.at(qrand() % annotations.size())->setModificationDate(QDateTime::currentDateTime());
                }

                qDeleteAll(annotations);

                if (!annotations.isEmpty()) {
                    break;
                }
            }
        }

        if (qrand() % 2 == 0) {
            QMutexLocker mutexLocker(m_annotationMutex);

            for (int index = 0; index < m_document->numPages(); ++index) {
                PagePointer page(loadPage(m_document, index));

                QList<Poppler::Annotation *> annotations = page->annotations();

                if (!annotations.isEmpty()) {
                    qDebug() << "remove annotation...";

                    page->removeAnnotation(annotations.takeAt(qrand() % annotations.size()));
                }

                qDeleteAll(annotations);

                if (!annotations.isEmpty()) {
                    break;
                }
            }
        }

        if (qrand() % 2 == 0) {
            qDebug() << "fonts...";

            m_document->fonts();
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        qDebug() << "usage: stress-threads-qt duration sillyCount crazyCount file(s)";

        return EXIT_FAILURE;
    }

    const int duration = atoi(argv[1]);
    const int sillyCount = atoi(argv[2]);
    const int crazyCount = atoi(argv[3]);

    qsrand(time(nullptr));

    for (int argi = 4; argi < argc; ++argi) {
        const QString file = QFile::decodeName(argv[argi]);
        Poppler::Document *document = Poppler::Document::load(file);

        if (document == nullptr) {
            qDebug() << "Could not load" << file;
            continue;
        }

        if (document->isLocked()) {
            qDebug() << file << "is locked";
            continue;
        }

        for (int i = 0; i < sillyCount; ++i) {
            (new SillyThread(document))->start();
        }

        QMutex *annotationMutex = new QMutex();

        for (int i = 0; i < crazyCount; ++i) {
            (new CrazyThread(qrand(), document, annotationMutex))->start();
        }
    }

    sleep(duration);

    return EXIT_SUCCESS;
}

#include "stress-threads-qt5.moc"
