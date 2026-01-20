//========================================================================
//
// FileDescriptorPDFDocBuilder.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2010, 2017, 2021, 2022, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright 2021 Christian Persch <chpe@src.gnome.org>
// Copyright 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#include <config.h>

#include <cstdio>

#include "FDPDFDocBuilder.h"
#include "FILECacheLoader.h"
#include "CachedFile.h"

//------------------------------------------------------------------------
// FileDescriptorPDFDocBuilder
//------------------------------------------------------------------------

int FileDescriptorPDFDocBuilder::parseFdFromUri(const std::string &uri)
{
    int fd = -1;
    char c;
    if (sscanf(uri.c_str(), "fd://%d%c", &fd, &c) != 1) {
        return -1;
    }

    return fd;
}

std::unique_ptr<PDFDoc> FileDescriptorPDFDocBuilder::buildPDFDoc(const GooString &uri, const std::optional<GooString> &ownerPassword, const std::optional<GooString> &userPassword)
{
    const auto fd = parseFdFromUri(uri.toStr());
    if (fd == -1) {
        return {};
    }

    FILE *file;
    if (fd == fileno(stdin)) {
        file = stdin;
    } else {
        file = fdopen(fd, "rb");
    }
    if (!file) {
        return {};
    }

    auto cachedFile = std::make_shared<CachedFile>(std::make_unique<FILECacheLoader>(file));
    return std::make_unique<PDFDoc>(std::make_unique<CachedFileStream>(cachedFile, 0, false, cachedFile->getLength(), Object::null()), ownerPassword, userPassword);
}

bool FileDescriptorPDFDocBuilder::supports(const GooString &uri)
{
    return parseFdFromUri(uri.toStr()) != -1;
}
