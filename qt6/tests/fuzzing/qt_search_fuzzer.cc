#include <cstdint>
#include <poppler-qt6.h>

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
        std::unique_ptr<Poppler::Page> p = doc->page(i);
        if (!p) {
            continue;
        }
        QString text = QString::fromUtf8(in_data);
        p->search(text, Poppler::Page::IgnoreCase, Poppler::Page::Rotate0);
    }

    return 0;
}
