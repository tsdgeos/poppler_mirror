//========================================================================
//
// CurlPDFDocBuilder.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2017, 2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#include <config.h>

#include "CurlPDFDocBuilder.h"

#include "CachedFile.h"
#include "CurlCachedFile.h"
#include "ErrorCodes.h"

//------------------------------------------------------------------------
// CurlPDFDocBuilder
//------------------------------------------------------------------------

std::unique_ptr<PDFDoc> CurlPDFDocBuilder::buildPDFDoc(const GooString &uri, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    auto cachedFile = std::make_shared<CachedFile>(std::make_unique<CurlCachedFileLoader>(uri.toStr()));

    if (cachedFile->getLength() == ((unsigned int)-1)) {
        return PDFDoc::ErrorPDFDoc(errOpenFile, uri.copy());
    }

    auto str = std::make_unique<CachedFileStream>(cachedFile, 0, false, cachedFile->getLength(), Object::null());

    return std::make_unique<PDFDoc>(std::move(str), ownerPassword, userPassword);
}

bool CurlPDFDocBuilder::supports(const GooString &uri)
{
    return uri.starts_with("http://") || uri.starts_with("https://");
}
