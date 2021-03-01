//========================================================================
//
// LocalPDFDocBuilder.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
//
//========================================================================

#include <config.h>

#include "LocalPDFDocBuilder.h"

//------------------------------------------------------------------------
// LocalPDFDocBuilder
//------------------------------------------------------------------------

std::unique_ptr<PDFDoc> LocalPDFDocBuilder::buildPDFDoc(const GooString &uri, GooString *ownerPassword, GooString *userPassword, void *guiDataA)
{
    if (uri.cmpN("file://", 7) == 0) {
        GooString *fileName = uri.copy();
        fileName->del(0, 7);
        return std::make_unique<PDFDoc>(fileName, ownerPassword, userPassword, guiDataA);
    } else {
        GooString *fileName = uri.copy();
        return std::make_unique<PDFDoc>(fileName, ownerPassword, userPassword, guiDataA);
    }
}

bool LocalPDFDocBuilder::supports(const GooString &uri)
{
    if (uri.cmpN("file://", 7) == 0) {
        return true;
    } else if (!strstr(uri.c_str(), "://")) {
        return true;
    } else {
        return false;
    }
}
