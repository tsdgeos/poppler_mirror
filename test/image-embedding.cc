//========================================================================
//
// image-embedding.cc
// A test util to check ImageEmbeddingUtils::embed().
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
// Copyright (C) 2022 by Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
//========================================================================

#include <config.h>
#include <cstdio>
#include <string>

#include "utils/parseargs.h"
#include "goo/GooString.h"
#include "Object.h"
#include "Dict.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "ImageEmbeddingUtils.h"

static int depth = 0;
static GooString colorSpace;
static GooString filter;
static bool smask = false;
static bool fail = false;
static bool printHelp = false;

static const ArgDesc argDesc[] = { { "-depth", argInt, &depth, 0, "XObject's property 'BitsPerComponent'" },
                                   { "-colorspace", argGooString, &colorSpace, 0, "XObject's property 'ColorSpace'" },
                                   { "-filter", argGooString, &filter, 0, "XObject's property 'Filter'" },
                                   { "-smask", argFlag, &smask, 0, "SMask should exist" },
                                   { "-fail", argFlag, &fail, 0, "the image embedding API is expected to fail" },
                                   { "-h", argFlag, &printHelp, 0, "print usage information" },
                                   { "-help", argFlag, &printHelp, 0, "print usage information" },
                                   { "--help", argFlag, &printHelp, 0, "print usage information" },
                                   { "-?", argFlag, &printHelp, 0, "print usage information" },
                                   {} };

int main(int argc, char *argv[])
{
    // Parse args.
    const bool ok = parseArgs(argDesc, &argc, argv);
    if (!ok || (argc != 3) || printHelp) {
        printUsage(argv[0], "PDF-FILE IMAGE-FILE", argDesc);
        return (printHelp) ? 0 : 1;
    }
    const GooString docPath(argv[1]);
    const GooString imagePath(argv[2]);

    auto doc = std::unique_ptr<PDFDoc>(PDFDocFactory().createPDFDoc(docPath));
    if (!doc->isOk()) {
        fprintf(stderr, "Error opening input PDF file.\n");
        return 1;
    }

    // Embed an image.
    Ref baseImageRef = ImageEmbeddingUtils::embed(doc->getXRef(), imagePath.toStr());
    if (baseImageRef == Ref::INVALID()) {
        if (fail) {
            return 0;
        }
        fprintf(stderr, "ImageEmbeddingUtils::embed() failed.\n");
        return 1;
    }

    // Save the updated PDF document.
    // const GooString outputPathSuffix(".pdf");
    // const GooString outputPath = GooString(&imagePath, &outputPathSuffix);
    // doc->saveAs(&outputPath, writeForceRewrite);

    // Check the base image.
    Object baseImageObj = Object(baseImageRef).fetch(doc->getXRef());
    Dict *baseImageDict = baseImageObj.streamGetDict();
    if (std::string("XObject") != baseImageDict->lookup("Type").getName()) {
        fprintf(stderr, "A problem with Type.\n");
        return 1;
    }
    if (std::string("Image") != baseImageDict->lookup("Subtype").getName()) {
        fprintf(stderr, "A problem with Subtype.\n");
        return 1;
    }
    if (depth > 0) {
        if (baseImageDict->lookup("BitsPerComponent").getInt() != depth) {
            fprintf(stderr, "A problem with BitsPerComponent.\n");
            return 1;
        }
    }
    if (!colorSpace.toStr().empty()) {
        if (colorSpace.compare(baseImageDict->lookup("ColorSpace").getName()) != 0) {
            fprintf(stderr, "A problem with ColorSpace.\n");
            return 1;
        }
    }
    if (!filter.toStr().empty()) {
        if (filter.compare(baseImageDict->lookup("Filter").getName()) != 0) {
            fprintf(stderr, "A problem with Filter.\n");
            return 1;
        }
    }
    if (smask) {
        Object maskObj = baseImageDict->lookup("SMask");
        if (!maskObj.isStream()) {
            fprintf(stderr, "A problem with SMask.\n");
            return 1;
        }
    }
    return 0;
}
