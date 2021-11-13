//========================================================================
//
// StdinPDFDocBuilder.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2017 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
//
//========================================================================

#include <config.h>

#include <cstdio>

#include "StdinPDFDocBuilder.h"
#include "FILECacheLoader.h"
#include "CachedFile.h"

//------------------------------------------------------------------------
// StdinPDFDocBuilder
//------------------------------------------------------------------------

int StdinPDFDocBuilder::parseFdFromUri(const GooString &uri)
{
    int fd = -1;
    char c;
    if (sscanf(uri.c_str(), "fd://%d%c", &fd, &c) != 1)
        return -1;

    return fd;
}

std::unique_ptr<PDFDoc> StdinPDFDocBuilder::buildPDFDoc(const GooString &uri, GooString *ownerPassword, GooString *userPassword, void *guiDataA)
{
    const auto fd = parseFdFromUri(uri);
    if (fd == -1)
        return {};

    FILE *file;
    if (fd == STDIN_FILENO)
        file = stdin;
    else
        file = fdopen(fd, "rb");
    if (!file)
        return {};

    CachedFile *cachedFile = new CachedFile(new FILECacheLoader(file), nullptr);
    return std::make_unique<PDFDoc>(new CachedFileStream(cachedFile, 0, false, cachedFile->getLength(), Object(objNull)), ownerPassword, userPassword);
}

bool StdinPDFDocBuilder::supports(const GooString &uri)
{
    return parseFdFromUri(uri) != -1;
}
