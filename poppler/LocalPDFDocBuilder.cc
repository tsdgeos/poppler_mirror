//========================================================================
//
// LocalPDFDocBuilder.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#include <config.h>

#include "LocalPDFDocBuilder.h"

//------------------------------------------------------------------------
// LocalPDFDocBuilder
//------------------------------------------------------------------------

std::unique_ptr<PDFDoc> LocalPDFDocBuilder::buildPDFDoc(const GooString &uri, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    if (uri.starts_with("file://")) {
        std::unique_ptr<GooString> fileName = uri.copy();
        fileName->erase(0, 7);
        return std::make_unique<PDFDoc>(std::move(fileName), ownerPassword, userPassword);
    }
    return std::make_unique<PDFDoc>(uri.copy(), ownerPassword, userPassword);
}

bool LocalPDFDocBuilder::supports(const GooString &uri)
{
    if (uri.starts_with("file://")) {
        return true;
    }
    if (!strstr(uri.c_str(), "://")) {
        return true;
    }
    return false;
}
