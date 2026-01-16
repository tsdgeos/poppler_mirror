#include <cstdint>
#include <poppler-qt6.h>
#include <QtGui/QImage>

static void dummy_error_function(const QString &message, const QVariant &closure)
{
    Q_UNUSED(message)
    Q_UNUSED(closure)
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    Poppler::setDebugErrorFunction(dummy_error_function, QVariant());
    QByteArray in_data = QByteArray::fromRawData((const char *)data, size);
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::loadFromData(in_data);
    if (!doc || doc->isLocked()) {
        return 0;
    }

    for (int i = 0; i < doc->numPages(); i++) {
        QString label = QString::fromUtf8(in_data);
        std::unique_ptr<Poppler::Page> p = doc->page(label);
        if (!p) {
            continue;
        }
        QImage image = p->renderToImage(72.0, 72.0, -1, -1, -1, -1, Poppler::Page::Rotate0);
    }

    return 0;
}
