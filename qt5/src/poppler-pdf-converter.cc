/* poppler-pdf-converter.cc: qt interface to poppler
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
 * Copyright (C) 2008, 2009, 2020, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2020, Thorsten Behrens <Thorsten.Behrens@CIB.de>
 * Copyright (C) 2020, Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "poppler-qt5.h"

#include "poppler-annotation-helper.h"
#include "poppler-annotation-private.h"
#include "poppler-private.h"
#include "poppler-converter-private.h"
#include "poppler-qiodeviceoutstream-private.h"

#include <QFile>
#include <QUuid>

#include "Array.h"
#include "Form.h"
#include <ErrorCodes.h>

namespace Poppler {

class PDFConverterPrivate : public BaseConverterPrivate
{
public:
    PDFConverterPrivate();
    ~PDFConverterPrivate() override;

    PDFConverter::PDFOptions opts;
};

PDFConverterPrivate::PDFConverterPrivate() : BaseConverterPrivate(), opts(nullptr) { }

PDFConverterPrivate::~PDFConverterPrivate() = default;

PDFConverter::PDFConverter(DocumentData *document) : BaseConverter(*new PDFConverterPrivate())
{
    Q_D(PDFConverter);
    d->document = document;
}

PDFConverter::~PDFConverter() { }

void PDFConverter::setPDFOptions(PDFConverter::PDFOptions options)
{
    Q_D(PDFConverter);
    d->opts = options;
}

PDFConverter::PDFOptions PDFConverter::pdfOptions() const
{
    Q_D(const PDFConverter);
    return d->opts;
}

bool PDFConverter::convert()
{
    Q_D(PDFConverter);
    d->lastError = NoError;

    if (d->document->locked) {
        d->lastError = FileLockedError;
        return false;
    }

    QIODevice *dev = d->openDevice();
    if (!dev) {
        d->lastError = OpenOutputError;
        return false;
    }

    bool deleteFile = false;
    if (QFile *file = qobject_cast<QFile *>(dev))
        deleteFile = !file->exists();

    int errorCode = errNone;
    QIODeviceOutStream stream(dev);
    if (d->opts & WithChanges) {
        errorCode = d->document->doc->saveAs(&stream);
    } else {
        errorCode = d->document->doc->saveWithoutChangesAs(&stream);
    }
    d->closeDevice();
    if (errorCode != errNone) {
        if (deleteFile) {
            qobject_cast<QFile *>(dev)->remove();
        }
        if (errorCode == errOpenFile)
            d->lastError = OpenOutputError;
        else
            d->lastError = NotSupportedInputFileError;
    }

    return (errorCode == errNone);
}

bool PDFConverter::sign(const NewSignatureData &data)
{
    Q_D(PDFConverter);
    d->lastError = NoError;

    if (d->document->locked) {
        d->lastError = FileLockedError;
        return false;
    }

    if (data.signatureText().isEmpty()) {
        qWarning() << "No signature text given";
        return false;
    }

    ::PDFDoc *doc = d->document->doc;
    ::Page *destPage = doc->getPage(data.page() + 1);

    const DefaultAppearance da { { objName, "SigFont" }, data.fontSize(), std::unique_ptr<AnnotColor> { convertQColor(data.fontColor()) } };
    const PDFRectangle rect = boundaryToPdfRectangle(destPage, data.boundingRectangle(), 0 /* no flags */);

    Object annotObj = Object(new Dict(doc->getXRef()));
    annotObj.dictSet("Type", Object(objName, "Annot"));
    annotObj.dictSet("Subtype", Object(objName, "Widget"));
    annotObj.dictSet("FT", Object(objName, "Sig"));
    annotObj.dictSet("T", Object(QStringToGooString(data.fieldPartialName())));
    Array *rectArray = new Array(doc->getXRef());
    rectArray->add(Object(rect.x1));
    rectArray->add(Object(rect.y1));
    rectArray->add(Object(rect.x2));
    rectArray->add(Object(rect.y2));
    annotObj.dictSet("Rect", Object(rectArray));

    GooString *daStr = da.toAppearanceString();
    annotObj.dictSet("DA", Object(daStr));

    const Ref ref = doc->getXRef()->addIndirectObject(&annotObj);
    Catalog *catalog = doc->getCatalog();
    catalog->addFormToAcroForm(ref);

    ::FormFieldSignature *field = new ::FormFieldSignature(doc, Object(annotObj.getDict()), ref, nullptr, nullptr);

    std::unique_ptr<GooString> gSignatureText = std::unique_ptr<GooString>(QStringToUnicodeGooString(data.signatureText()));
    field->setCustomAppearanceContent(*gSignatureText);

    Object refObj(ref);
    AnnotWidget *signatureAnnot = new AnnotWidget(doc, &annotObj, &refObj, field);
    signatureAnnot->setFlags(signatureAnnot->getFlags() | Annot::flagPrint | Annot::flagLocked);
    Dict dummy(doc->getXRef());
    auto appearCharacs = std::make_unique<AnnotAppearanceCharacs>(&dummy);
    appearCharacs->setBorderColor(std::unique_ptr<AnnotColor> { convertQColor(data.borderColor()) });
    appearCharacs->setBackColor(std::unique_ptr<AnnotColor> { convertQColor(data.backgroundColor()) });
    signatureAnnot->setAppearCharacs(std::move(appearCharacs));

    bool dummyAddDingbatsResource = false; // This is only update so if we didn't need to add
                                           // the dingbats resource we should not need it now
    signatureAnnot->generateFieldAppearance(&dummyAddDingbatsResource);
    signatureAnnot->updateAppearanceStream();

    FormWidget *formWidget = field->getWidget(field->getNumWidgets() - 1);
    formWidget->setWidgetAnnotation(signatureAnnot);

    destPage->addAnnot(signatureAnnot);

    std::unique_ptr<AnnotBorder> border(new AnnotBorderArray());
    border->setWidth(1.5);
    signatureAnnot->setBorder(std::move(border));

    FormWidgetSignature *fws = dynamic_cast<FormWidgetSignature *>(formWidget);
    if (fws)
        return fws->signDocument(d->outputFileName.toUtf8().constData(), data.certNickname().toUtf8().constData(), "SHA256", data.password().toUtf8().constData());

    return false;
}

struct PDFConverter::NewSignatureData::NewSignatureDataPrivate
{
    NewSignatureDataPrivate() = default;

    QString certNickname;
    QString password;
    int page;
    QRectF boundingRectangle;
    QString signatureText;
    double fontSize = 10.0;
    QColor fontColor = Qt::red;
    QColor borderColor = Qt::red;
    QColor backgroundColor = QColor(240, 240, 240);

    QString partialName = QUuid::createUuid().toString();
};

PDFConverter::NewSignatureData::NewSignatureData() : d(new NewSignatureDataPrivate()) { }

PDFConverter::NewSignatureData::~NewSignatureData()
{
    delete d;
}

QString PDFConverter::NewSignatureData::certNickname() const
{
    return d->certNickname;
}

void PDFConverter::NewSignatureData::setCertNickname(const QString &certNickname)
{
    d->certNickname = certNickname;
}

QString PDFConverter::NewSignatureData::password() const
{
    return d->password;
}

void PDFConverter::NewSignatureData::setPassword(const QString &password)
{
    d->password = password;
}

int PDFConverter::NewSignatureData::page() const
{
    return d->page;
}

void PDFConverter::NewSignatureData::setPage(int page)
{
    d->page = page;
}

QRectF PDFConverter::NewSignatureData::boundingRectangle() const
{
    return d->boundingRectangle;
}

void PDFConverter::NewSignatureData::setBoundingRectangle(const QRectF &rect)
{
    d->boundingRectangle = rect;
}

QString PDFConverter::NewSignatureData::signatureText() const
{
    return d->signatureText;
}

void PDFConverter::NewSignatureData::setSignatureText(const QString &text)
{
    d->signatureText = text;
}

double PDFConverter::NewSignatureData::fontSize() const
{
    return d->fontSize;
}

void PDFConverter::NewSignatureData::setFontSize(double fontSize)
{
    d->fontSize = fontSize;
}

QColor PDFConverter::NewSignatureData::fontColor() const
{
    return d->fontColor;
}

void PDFConverter::NewSignatureData::setFontColor(const QColor &color)
{
    d->fontColor = color;
}

QColor PDFConverter::NewSignatureData::borderColor() const
{
    return d->borderColor;
}

void PDFConverter::NewSignatureData::setBorderColor(const QColor &color)
{
    d->borderColor = color;
}

QColor PDFConverter::NewSignatureData::backgroundColor() const
{
    return d->backgroundColor;
}

void PDFConverter::NewSignatureData::setBackgroundColor(const QColor &color)
{
    d->backgroundColor = color;
}

QString PDFConverter::NewSignatureData::fieldPartialName() const
{
    return d->partialName;
}

void PDFConverter::NewSignatureData::setFieldPartialName(const QString &name)
{
    d->partialName = name;
}
}
