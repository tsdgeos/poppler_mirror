#include <cstdint>
#include <poppler-qt6.h>
#include <QtCore/QBuffer>

static void dummy_error_function(const QString &message, const QVariant &closure)
{
    Q_UNUSED(message)
    Q_UNUSED(closure)
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    Poppler::setDebugErrorFunction(dummy_error_function, QVariant());
    const QFont font(QStringLiteral("Helvetica"), 20);
    const QColor color = QColor::fromRgb(0xAB, 0xCD, 0xEF);

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
        Poppler::TextAnnotation *ann = new Poppler::TextAnnotation(Poppler::TextAnnotation::InPlace);
        ann->setTextFont(font);
        ann->setTextColor(color);
        ann->setBoundary(QRectF(0.1, 0.1, 0.2, 0.2));
        ann->setContents(QString::fromUtf8(in_data));
        p->addAnnotation(ann);

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        std::unique_ptr<Poppler::PDFConverter> conv(doc->pdfConverter());
        conv->setOutputDevice(&buffer);
        conv->setPDFOptions(Poppler::PDFConverter::WithChanges);
        conv->convert();
        buffer.close();
        delete ann;
    }

    return 0;
}
