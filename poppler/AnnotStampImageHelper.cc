//========================================================================
//
// AnnotStampImageHelper.cc
//
// Copyright (C) 2021 Mahmoud Ahmed Khalil <mahmoudkhalil11@gmail.com>
// Copyright (C) 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2026 Malika Asman <asmanodeny@gmail.com>
//
// Licensed under GPLv2 or later
//
//========================================================================

#include "AnnotStampImageHelper.h"

#include "PDFDoc.h"
#include "Stream.h"
#include "Dict.h"

AnnotStampImageHelper::AnnotStampImageHelper(PDFDoc *docA, int widthA, int heightA, ColorSpace colorSpace, int bitsPerComponent, std::vector<char> &&data)
    : AnnotStampImageHelper(docA, widthA, heightA, colorSpace, bitsPerComponent, std::move(data), Ref::INVALID())
{
}

AnnotStampImageHelper::AnnotStampImageHelper(PDFDoc *docA, int widthA, int heightA, ColorSpace colorSpace, int bitsPerComponent, std::vector<char> &&data, Ref softMaskRef)
{
    doc = docA;
    width = widthA;
    height = heightA;
    sMaskRef = softMaskRef;

    auto dict = std::make_unique<Dict>(docA->getXRef());
    dict->add("Type", Object::name("XObject"));
    dict->add("Subtype", Object::name("Image"));
    dict->add("Width", Object(width));
    dict->add("Height", Object(height));
    dict->add("ImageMask", Object(false));
    dict->add("BitsPerComponent", Object(bitsPerComponent));
    // Note: "Length" is added automatically by addStreamObject

    switch (colorSpace) {
    case ColorSpace::DeviceGray:
        dict->add("ColorSpace", Object::name("DeviceGray"));
        break;
    case ColorSpace::DeviceRGB:
        dict->add("ColorSpace", Object::name("DeviceRGB"));
        break;
    case ColorSpace::DeviceCMYK:
        dict->add("ColorSpace", Object::name("DeviceCMYK"));
        break;
    }

    // Add soft mask reference if provided and must be done before addStreamObject
    if (sMaskRef != Ref::INVALID()) {
        dict->add("SMask", Object(sMaskRef));
    }

    ref = doc->getXRef()->addStreamObject(std::move(dict), std::move(data), StreamCompression::Compress);
}

void AnnotStampImageHelper::removeAnnotStampImageObject()
{
    if (sMaskRef != Ref::INVALID()) {
        doc->getXRef()->removeIndirectObject(sMaskRef);
    }

    doc->getXRef()->removeIndirectObject(ref);
}
