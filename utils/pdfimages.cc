//========================================================================
//
// pdfimages.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
// Modified for Debian by Hamish Moffatt, 22 May 2002.
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2007-2008, 2010, 2018, 2022, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
// Copyright (C) 2012, 2013, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019, 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2019 Hartmut Goebel <h.goebel@crazy-compilers.com>
// Copyright (C) 2024 Fernando Herrera <fherrera@onirica.com>
// Copyright (C) 2024 Sebastian J. Bronner <waschtl@sbronner.com>
// Copyright (C) 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <cstdio>
#include "parseargs.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "ImageOutputDev.h"
#include "Error.h"
#include "Win32Console.h"

static int firstPage = 1;
static int lastPage = 0;
static bool listImages = false;
static bool enablePNG = false;
static bool enableTiff = false;
static bool dumpJPEG = false;
static bool dumpJP2 = false;
static bool dumpJBIG2 = false;
static bool dumpCCITT = false;
static bool allFormats = false;
static bool pageNames = false;
static bool printFilenames = false;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool quiet = false;
static bool printVersion = false;
static bool printHelp = false;

static const ArgDesc argDesc[] = { { .arg = "-f", .kind = argInt, .val = &firstPage, .size = 0, .usage = "first page to convert" },
                                   { .arg = "-l", .kind = argInt, .val = &lastPage, .size = 0, .usage = "last page to convert" },
#if ENABLE_LIBPNG
                                   { .arg = "-png", .kind = argFlag, .val = &enablePNG, .size = 0, .usage = "change the default output format to PNG" },
#endif
#if ENABLE_LIBTIFF
                                   { .arg = "-tiff", .kind = argFlag, .val = &enableTiff, .size = 0, .usage = "change the default output format to TIFF" },
#endif
                                   { .arg = "-j", .kind = argFlag, .val = &dumpJPEG, .size = 0, .usage = "write JPEG images as JPEG files" },
                                   { .arg = "-jp2", .kind = argFlag, .val = &dumpJP2, .size = 0, .usage = "write JPEG2000 images as JP2 files" },
                                   { .arg = "-jbig2", .kind = argFlag, .val = &dumpJBIG2, .size = 0, .usage = "write JBIG2 images as JBIG2 files" },
                                   { .arg = "-ccitt", .kind = argFlag, .val = &dumpCCITT, .size = 0, .usage = "write CCITT images as CCITT files" },
                                   { .arg = "-all", .kind = argFlag, .val = &allFormats, .size = 0, .usage = "equivalent to -png -tiff -j -jp2 -jbig2 -ccitt" },
                                   { .arg = "-list", .kind = argFlag, .val = &listImages, .size = 0, .usage = "print list of images instead of saving" },
                                   { .arg = "-opw", .kind = argString, .val = ownerPassword, .size = sizeof(ownerPassword), .usage = "owner password (for encrypted files)" },
                                   { .arg = "-upw", .kind = argString, .val = userPassword, .size = sizeof(userPassword), .usage = "user password (for encrypted files)" },
                                   { .arg = "-p", .kind = argFlag, .val = &pageNames, .size = 0, .usage = "include page numbers in output file names" },
                                   { .arg = "-print-filenames", .kind = argFlag, .val = &printFilenames, .size = 0, .usage = "print image filenames to stdout" },
                                   { .arg = "-q", .kind = argFlag, .val = &quiet, .size = 0, .usage = "don't print any messages or errors" },
                                   { .arg = "-v", .kind = argFlag, .val = &printVersion, .size = 0, .usage = "print copyright and version info" },
                                   { .arg = "-h", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "-help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "--help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "-?", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   {} };

int main(int argc, char *argv[])
{
    char *imgRoot = nullptr;
    std::optional<GooString> ownerPW, userPW;

    Win32Console win32Console(&argc, &argv);

    // parse args
    const bool ok = parseArgs(argDesc, &argc, argv);
    if (!ok || (listImages && argc != 2) || (!listImages && argc != 3) || printVersion || printHelp) {
        fprintf(stderr, "pdfimages version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdfimages", "<PDF-file> <image-root>", argDesc);
        }
        if (printVersion || printHelp) {
            return 0;
        }
        return 99;
    }
    auto *fileName = new GooString(argv[1]);
    if (!listImages) {
        imgRoot = argv[2];
    }

    // read config file
    globalParams = std::make_unique<GlobalParams>();
    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    // open PDF file
    if (ownerPassword[0] != '\001') {
        ownerPW = GooString(ownerPassword);
    }
    if (userPassword[0] != '\001') {
        userPW = GooString(userPassword);
    }
    if (fileName->compare("-") == 0) {
        delete fileName;
        fileName = new GooString("fd://0");
    }

    std::unique_ptr<PDFDoc> doc = PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW);
    delete fileName;

    if (!doc->isOk()) {
        return 1;
    }

    // check for copy permission
#ifdef ENFORCE_PERMISSIONS
    if (!doc->okToCopy()) {
        error(errNotAllowed, -1, "Copying of images from this document is not allowed.");
        return 3;
    }
#endif

    // get page range
    if (firstPage < 1) {
        firstPage = 1;
    }
    if (firstPage > doc->getNumPages()) {
        error(errCommandLine, -1, "Wrong page range given: the first page ({0:d}) can not be larger then the number of pages in the document ({1:d}).", firstPage, doc->getNumPages());
        return 99;
    }
    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }
    if (lastPage < firstPage) {
        error(errCommandLine, -1, "Wrong page range given: the first page ({0:d}) can not be after the last page ({1:d}).", firstPage, lastPage);
        return 99;
    }

    // write image files
    auto *imgOut = new ImageOutputDev(imgRoot, pageNames, listImages);
    if (imgOut->isOk()) {
        if (allFormats) {
            imgOut->enablePNG(true);
            imgOut->enableTiff(true);
            imgOut->enableJpeg(true);
            imgOut->enableJpeg2000(true);
            imgOut->enableJBig2(true);
            imgOut->enableCCITT(true);
        } else {
            imgOut->enablePNG(enablePNG);
            imgOut->enableTiff(enableTiff);
            imgOut->enableJpeg(dumpJPEG);
            imgOut->enableJpeg2000(dumpJP2);
            imgOut->enableJBig2(dumpJBIG2);
            imgOut->enableCCITT(dumpCCITT);
        }
        imgOut->enablePrintFilenames(printFilenames);
        doc->displayPages(imgOut, firstPage, lastPage, 72, 72, 0, true, false, false);
    }
    const int exitCode = imgOut->isOk() ? 0 : imgOut->getErrorCode();
    delete imgOut;
    return exitCode;
}
