#ifndef ANNOT_STAMP_H
#define ANNOT_STAMP_H
//========================================================================
//
// annot_stamp.h
//
// Copyright (C) 2021 Mahmoud Ahmed Khalil <mahmoudkhalil11@gmail.com>
// Copyright (C) 2021 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// Licensed under GPLv2 or later
//
//========================================================================

#include <PDFDoc.h>

static inline std::unique_ptr<Dict> getStampExtGStateDict(PDFDoc *doc, double CA, double ca)
{
    auto a0Dict = std::make_unique<Dict>(doc->getXRef());
    a0Dict->add("CA", Object(CA));
    a0Dict->add("ca", Object(ca));

    auto a1Dict = std::make_unique<Dict>(doc->getXRef());
    a1Dict->add("CA", Object(1));
    a1Dict->add("ca", Object(1));

    auto extGStateDict = std::make_unique<Dict>(doc->getXRef());
    extGStateDict->add("a0", Object(std::move(a0Dict)));
    extGStateDict->add("a1", Object(std::move(a1Dict)));

    return extGStateDict;
}
#endif
