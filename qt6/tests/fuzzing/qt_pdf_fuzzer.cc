#include <cstdint>
#include <poppler-qt6.h>
#include <poppler-form.h>
#include <QtCore/QBuffer>
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
        std::unique_ptr<Poppler::Page> p = doc->page(i);
        if (!p) {
            continue;
        }
        QImage image = p->renderToImage(72.0, 72.0, -1, -1, -1, -1, Poppler::Page::Rotate0);
    }

    const std::vector<std::unique_ptr<Poppler::FormFieldSignature>> signatures = doc->signatures();
    for (const std::unique_ptr<Poppler::FormFieldSignature> &signature : signatures) {
        const Poppler::SignatureValidationInfo svi = signature->validateAsync(Poppler::FormFieldSignature::ValidateVerifyCertificate).first;
        signature->validateResult();
    }

    doc->outline();

    if (doc->numPages() > 0) {
        QList<int> pageList;
        for (int i = 0; i < doc->numPages(); i++) {
            pageList << (i + 1);
        }

        std::unique_ptr<Poppler::PSConverter> psConverter = doc->psConverter();

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        psConverter->setOutputDevice(&buffer);

        psConverter->setPageList(pageList);
        psConverter->setPaperWidth(595);
        psConverter->setPaperHeight(842);
        psConverter->setTitle(doc->info(QStringLiteral("Title")));
        psConverter->convert();
    }

    return 0;
}
