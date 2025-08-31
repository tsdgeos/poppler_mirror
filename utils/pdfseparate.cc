//========================================================================
//
// pdfseparate.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2011, 2012, 2015 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2012-2014, 2017, 2018, 2021, 2022, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013, 2016 Pino Toscano <pino@kde.org>
// Copyright (C) 2013 Daniel Kahn Gillmor <dkg@fifthhorseman.net>
// Copyright (C) 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2017 Léonard Michelet <leonard.michelet@smile.fr>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 Oliver Sander <oliver.sander@tu-dresden.de>
//
//========================================================================
#include "config.h"
#include <poppler-config.h>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include "parseargs.h"
#include "goo/GooString.h"
#include "PDFDoc.h"
#include "ErrorCodes.h"
#include "GlobalParams.h"
#include "Win32Console.h"
#include <cctype>

static int firstPage = 0;
static int lastPage = 0;
static bool printVersion = false;
static bool printHelp = false;

static const ArgDesc argDesc[] = { { "-f", argInt, &firstPage, 0, "first page to extract" },
                                   { "-l", argInt, &lastPage, 0, "last page to extract" },
                                   { "-v", argFlag, &printVersion, 0, "print copyright and version info" },
                                   { "-h", argFlag, &printHelp, 0, "print usage information" },
                                   { "-help", argFlag, &printHelp, 0, "print usage information" },
                                   { "--help", argFlag, &printHelp, 0, "print usage information" },
                                   { "-?", argFlag, &printHelp, 0, "print usage information" },
                                   {} };

static bool extractPages(const char *srcFileName, const char *destFileName)
{
    char pathName[4096];
    PDFDoc *doc = new PDFDoc(std::make_unique<GooString>(srcFileName));

    if (!doc->isOk()) {
        error(errSyntaxError, -1, "Could not extract page(s) from damaged file ('{0:s}')", srcFileName);
        delete doc;
        return false;
    }

    // destFileName can have multiple %% and one %d
    // We use auxDestFileName to replace all the valid % appearances
    // by 'A' (random char that is not %), if at the end of replacing
    // any of the valid appearances there is still any % around, the
    // pattern is wrong
    if (firstPage == 0 && lastPage == 0) {
        firstPage = 1;
        lastPage = doc->getNumPages();
    }
    if (lastPage == 0) {
        lastPage = doc->getNumPages();
    }
    if (firstPage == 0) {
        firstPage = 1;
    }
    if (lastPage < firstPage) {
        error(errCommandLine, -1, "Wrong page range given: the first page ({0:d}) can not be after the last page ({1:d}).", firstPage, lastPage);
        delete doc;
        return false;
    }
    bool foundmatch = false;
    char *auxDestFileName = strdup(destFileName);
    char *p = strstr(auxDestFileName, "%d");
    if (p != nullptr) {
        foundmatch = true;
        *p = 'A';
    } else {
        char pattern[6];
        for (int i = 2; i < 10; i++) {
            sprintf(pattern, "%%0%dd", i);
            p = strstr(auxDestFileName, pattern);
            if (p != nullptr) {
                foundmatch = true;
                *p = 'A';
                break;
            }
        }
    }
    if (!foundmatch && firstPage != lastPage) {
        error(errSyntaxError, -1, "'{0:s}' must contain '%d' (or any variant respecting printf format) if more than one page should be extracted, in order to print the page number", destFileName);
        free(auxDestFileName);
        delete doc;
        return false;
    }

    // at this point auxDestFileName can only contain %%
    p = strstr(auxDestFileName, "%%");
    while (p != nullptr) {
        *p = 'A';
        *(p + 1) = 'A';
        p = strstr(p, "%%");
    }

    // at this point any other % is wrong
    p = strstr(auxDestFileName, "%");
    if (p != nullptr) {
        error(errSyntaxError, -1, "'{0:s}' can only contain one '%d' pattern", destFileName);
        free(auxDestFileName);
        delete doc;
        return false;
    }
    free(auxDestFileName);

    for (int pageNo = firstPage; pageNo <= lastPage; pageNo++) {
        snprintf(pathName, sizeof(pathName) - 1, destFileName, pageNo);
        PDFDoc *pagedoc = new PDFDoc(std::make_unique<GooString>(srcFileName));
        int errCode = pagedoc->savePageAs(pathName, pageNo);
        if (errCode != errNone) {
            delete doc;
            delete pagedoc;
            return false;
        }
        delete pagedoc;
    }
    delete doc;
    return true;
}

static constexpr int kOtherError = 99;

int main(int argc, char *argv[])
{
    // parse args
    Win32Console win32console(&argc, &argv);
    const bool parseOK = parseArgs(argDesc, &argc, argv);
    if (!parseOK || argc != 3 || printVersion || printHelp) {
        fprintf(stderr, "pdfseparate version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdfseparate", "<PDF-sourcefile> <PDF-pattern-destfile>", argDesc);
        }
        if (printVersion || printHelp) {
            return 0;
        } else {
            return kOtherError;
        }
    }
    globalParams = std::make_unique<GlobalParams>();
    const bool extractOK = extractPages(argv[1], argv[2]);
    return extractOK ? 0 : kOtherError;
}
