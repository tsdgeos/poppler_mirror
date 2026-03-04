#include <cstdint>
#include <vector>

#include <poppler-qt6.h>
#include <poppler-converter.h>
#include <poppler-form.h>

#include <QtCore/QBuffer>
#include <QtCore/QTemporaryFile>

#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>

static void dummy_error_function(const QString &, const QVariant &) { }

static void initialize_nss_dir()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir_path = dirname(exe_path);
        const QString nssDir = QString::fromLocal8Bit(dir_path) + QStringLiteral("/unittestcases/signing_nss/db_complete");
        Poppler::setNSSDir(nssDir);
    }

    initialized = true;
}

static std::vector<uint8_t> createTestImage(const uint8_t *data, size_t size, uint8_t mode)
{
    std::vector<uint8_t> result;

    switch (mode % 2) {
    case 0: {
        const uint8_t png_header[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
        result.insert(result.end(), png_header, png_header + sizeof(png_header));
        break;
    }
    case 1: {
        const uint8_t jpeg_header[] = { 0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46 };
        result.insert(result.end(), jpeg_header, jpeg_header + sizeof(jpeg_header));
        break;
    }
    }

    result.insert(result.end(), data, data + size);
    return result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 10) {
        return 0;
    }

    Poppler::setDebugErrorFunction(dummy_error_function, QVariant());
    initialize_nss_dir();

    uint8_t mode = data[0];
    std::vector<uint8_t> imageData = createTestImage(data + 1, size - 1, mode);

    QTemporaryFile imageFile;
    if (!imageFile.open()) {
        return 0;
    }
    imageFile.write(reinterpret_cast<const char *>(imageData.data()), imageData.size());
    imageFile.flush();
    imageFile.close();

    const char *minimalPdf =
            "%PDF-1.4\n1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]>>endobj\nxref\n0 4\ntrailer<</Root 1 0 R/Size 4>>\n%%EOF";

    QByteArray pdfBytes(minimalPdf);
    std::unique_ptr<Poppler::Document> doc = Poppler::Document::loadFromData(pdfBytes);
    if (!doc || doc->isLocked() || doc->numPages() == 0) {
        return 0;
    }

    QTemporaryFile outputFile;
    if (!outputFile.open()) {
        return 0;
    }

    std::unique_ptr<Poppler::PDFConverter> converter = doc->pdfConverter();
    if (!converter) {
        return 0;
    }

    converter->setOutputFileName(outputFile.fileName());

    Poppler::PDFConverter::NewSignatureData sigData;
    sigData.setCertNickname(QStringLiteral("RSA test key 01"));
    sigData.setPassword(QStringLiteral(""));
    sigData.setPage(0);
    sigData.setBoundingRectangle(QRectF(100, 100, 200, 100));
    sigData.setSignatureText(QStringLiteral("Fuzz Test"));
    sigData.setImagePath(imageFile.fileName());

    converter->sign(sigData);

    return 0;
}
