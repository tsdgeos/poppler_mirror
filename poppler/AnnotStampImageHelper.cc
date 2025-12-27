//========================================================================
//
// AnnotStampImageHelper.cc
//
// Copyright (C) 2021 Mahmoud Ahmed Khalil <mahmoudkhalil11@gmail.com>
// Copyright (C) 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// Licensed under GPLv2 or later
//
//========================================================================

#include "AnnotStampImageHelper.h"

#include "PDFDoc.h"
#include "Stream.h"
#include "Dict.h"

AnnotStampImageHelper::AnnotStampImageHelper(PDFDoc *docA, int widthA, int heightA, ColorSpace colorSpace, int bitsPerComponent, char *data, int dataLength)
{
    initialize(docA, widthA, heightA, colorSpace, bitsPerComponent, data, dataLength);
}

AnnotStampImageHelper::AnnotStampImageHelper(PDFDoc *docA, int widthA, int heightA, ColorSpace colorSpace, int bitsPerComponent, char *data, int dataLength, Ref softMaskRef)
{
    initialize(docA, widthA, heightA, colorSpace, bitsPerComponent, data, dataLength);

    sMaskRef = softMaskRef;
    Dict *dict = imgObj.streamGetDict();
    dict->add("SMask", Object(sMaskRef));
}

void AnnotStampImageHelper::initialize(PDFDoc *docA, int widthA, int heightA, ColorSpace colorSpace, int bitsPerComponent, char *data, int dataLength)
{
    doc = docA;
    width = widthA;
    height = heightA;
    sMaskRef = Ref::INVALID();

    Dict *dict = new Dict(docA->getXRef());
    dict->add("Type", Object(objName, "XObject"));
    dict->add("Subtype", Object(objName, "Image"));
    dict->add("Width", Object(width));
    dict->add("Height", Object(height));
    dict->add("ImageMask", Object(false));
    dict->add("BitsPerComponent", Object(bitsPerComponent));
    dict->add("Length", Object(dataLength));

    switch (colorSpace) {
    case ColorSpace::DeviceGray:
        dict->add("ColorSpace", Object(objName, "DeviceGray"));
        break;
    case ColorSpace::DeviceRGB:
        dict->add("ColorSpace", Object(objName, "DeviceRGB"));
        break;
    case ColorSpace::DeviceCMYK:
        dict->add("ColorSpace", Object(objName, "DeviceCMYK"));
        break;
    }

    std::vector<char> dataCopied { data, data + dataLength };

    auto dataStream = std::make_unique<AutoFreeMemStream>(std::move(dataCopied), Object(dict));
    imgObj = Object(std::move(dataStream));
    ref = doc->getXRef()->addIndirectObject(imgObj);
}

void AnnotStampImageHelper::removeAnnotStampImageObject()
{
    if (sMaskRef != Ref::INVALID()) {
        doc->getXRef()->removeIndirectObject(sMaskRef);
    }

    doc->getXRef()->removeIndirectObject(ref);
}
