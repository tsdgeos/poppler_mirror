//========================================================================
//
// pdftops.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
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
// Copyright (C) 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2007-2008, 2010, 2015, 2017, 2018, 2020-2022, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Till Kamppeter <till.kamppeter@gmail.com>
// Copyright (C) 2009 Sanjoy Mahajan <sanjoy@mit.edu>
// Copyright (C) 2009, 2011, 2012, 2014-2016, 2020 William Bader <williambader@hotmail.com>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2012 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2014, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019, 2021, 2023 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2020 Philipp Knechtges <philipp-dev@knechtges.com>
// Copyright (C) 2021 Hubert Figuiere <hub@figuiere.net>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "parseargs.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "PSOutputDev.h"
#include "Error.h"
#include "Win32Console.h"
#include "sanitychecks.h"

#if USE_CMS
#    include <lcms2.h>
#endif

static bool setPSPaperSize(char *size, int &psPaperWidth, int &psPaperHeight)
{
    if (!strcmp(size, "match")) {
        psPaperWidth = psPaperHeight = -1;
    } else if (!strcmp(size, "letter")) {
        psPaperWidth = 612;
        psPaperHeight = 792;
    } else if (!strcmp(size, "legal")) {
        psPaperWidth = 612;
        psPaperHeight = 1008;
    } else if (!strcmp(size, "A4")) {
        psPaperWidth = 595;
        psPaperHeight = 842;
    } else if (!strcmp(size, "A3")) {
        psPaperWidth = 842;
        psPaperHeight = 1190;
    } else {
        return false;
    }
    return true;
}

static int firstPage = 1;
static int lastPage = 0;
static bool level1 = false;
static bool level1Sep = false;
static bool level2 = false;
static bool level2Sep = false;
static bool level3 = false;
static bool level3Sep = false;
static bool origPageSizes = false;
static bool doEPS = false;
static bool doForm = false;
static bool doOPI = false;
static int splashResolution = 0;
static bool psBinary = false;
static bool noEmbedT1Fonts = false;
static bool noEmbedTTFonts = false;
static bool noEmbedCIDPSFonts = false;
static bool noEmbedCIDTTFonts = false;
static bool fontPassthrough = false;
static bool optimizeColorSpace = false;
static bool passLevel1CustomColor = false;
static char rasterAntialiasStr[16] = "";
static char forceRasterizeStr[16] = "";
static bool preload = false;
static char paperSize[15] = "";
static int paperWidth = -1;
static int paperHeight = -1;
static bool noCrop = false;
static bool expand = false;
static bool noShrink = false;
static bool noCenter = false;
static bool duplex = false;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool quiet = false;
static bool printVersion = false;
static bool printHelp = false;
static bool overprint = false;
static GooString processcolorformatname;
static SplashColorMode processcolorformat;
static bool processcolorformatspecified = false;
#if USE_CMS
static GooString processcolorprofilename;
static GfxLCMSProfilePtr processcolorprofile;
static GooString defaultgrayprofilename;
static GfxLCMSProfilePtr defaultgrayprofile;
static GooString defaultrgbprofilename;
static GfxLCMSProfilePtr defaultrgbprofile;
static GooString defaultcmykprofilename;
static GfxLCMSProfilePtr defaultcmykprofile;
#endif

static const ArgDesc argDesc[] = {
    { .arg = "-f", .kind = argInt, .val = &firstPage, .size = 0, .usage = "first page to print" },
    { .arg = "-l", .kind = argInt, .val = &lastPage, .size = 0, .usage = "last page to print" },
    { .arg = "-level1", .kind = argFlag, .val = &level1, .size = 0, .usage = "generate Level 1 PostScript" },
    { .arg = "-level1sep", .kind = argFlag, .val = &level1Sep, .size = 0, .usage = "generate Level 1 separable PostScript" },
    { .arg = "-level2", .kind = argFlag, .val = &level2, .size = 0, .usage = "generate Level 2 PostScript" },
    { .arg = "-level2sep", .kind = argFlag, .val = &level2Sep, .size = 0, .usage = "generate Level 2 separable PostScript" },
    { .arg = "-level3", .kind = argFlag, .val = &level3, .size = 0, .usage = "generate Level 3 PostScript" },
    { .arg = "-level3sep", .kind = argFlag, .val = &level3Sep, .size = 0, .usage = "generate Level 3 separable PostScript" },
    { .arg = "-origpagesizes", .kind = argFlag, .val = &origPageSizes, .size = 0, .usage = "conserve original page sizes" },
    { .arg = "-eps", .kind = argFlag, .val = &doEPS, .size = 0, .usage = "generate Encapsulated PostScript (EPS)" },
    { .arg = "-form", .kind = argFlag, .val = &doForm, .size = 0, .usage = "generate a PostScript form" },
    { .arg = "-opi", .kind = argFlag, .val = &doOPI, .size = 0, .usage = "generate OPI comments" },
    { .arg = "-r", .kind = argInt, .val = &splashResolution, .size = 0, .usage = "resolution for rasterization, in DPI (default is 300)" },
    { .arg = "-binary", .kind = argFlag, .val = &psBinary, .size = 0, .usage = "write binary data in Level 1 PostScript" },
    { .arg = "-noembt1", .kind = argFlag, .val = &noEmbedT1Fonts, .size = 0, .usage = "don't embed Type 1 fonts" },
    { .arg = "-noembtt", .kind = argFlag, .val = &noEmbedTTFonts, .size = 0, .usage = "don't embed TrueType fonts" },
    { .arg = "-noembcidps", .kind = argFlag, .val = &noEmbedCIDPSFonts, .size = 0, .usage = "don't embed CID PostScript fonts" },
    { .arg = "-noembcidtt", .kind = argFlag, .val = &noEmbedCIDTTFonts, .size = 0, .usage = "don't embed CID TrueType fonts" },
    { .arg = "-passfonts", .kind = argFlag, .val = &fontPassthrough, .size = 0, .usage = "don't substitute missing fonts" },
    { .arg = "-aaRaster", .kind = argString, .val = rasterAntialiasStr, .size = sizeof(rasterAntialiasStr), .usage = "enable anti-aliasing on rasterization: yes, no" },
    { .arg = "-rasterize", .kind = argString, .val = forceRasterizeStr, .size = sizeof(forceRasterizeStr), .usage = "control rasterization: always, never, whenneeded" },
    { .arg = "-processcolorformat", .kind = argGooString, .val = &processcolorformatname, .size = 0, .usage = "color format that is used during rasterization and transparency reduction: MONO8, RGB8, CMYK8" },
#if USE_CMS
    { .arg = "-processcolorprofile", .kind = argGooString, .val = &processcolorprofilename, .size = 0, .usage = "ICC color profile to use as the process color profile during rasterization and transparency reduction" },
    { .arg = "-defaultgrayprofile", .kind = argGooString, .val = &defaultgrayprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultGray color space" },
    { .arg = "-defaultrgbprofile", .kind = argGooString, .val = &defaultrgbprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultRGB color space" },
    { .arg = "-defaultcmykprofile", .kind = argGooString, .val = &defaultcmykprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultCMYK color space" },
#endif
    { .arg = "-optimizecolorspace", .kind = argFlag, .val = &optimizeColorSpace, .size = 0, .usage = "convert gray RGB images to gray color space" },
    { .arg = "-passlevel1customcolor", .kind = argFlag, .val = &passLevel1CustomColor, .size = 0, .usage = "pass custom color in level1sep" },
    { .arg = "-preload", .kind = argFlag, .val = &preload, .size = 0, .usage = "preload images and forms" },
    { .arg = "-paper", .kind = argString, .val = paperSize, .size = sizeof(paperSize), .usage = "paper size (letter, legal, A4, A3, match)" },
    { .arg = "-paperw", .kind = argInt, .val = &paperWidth, .size = 0, .usage = "paper width, in points" },
    { .arg = "-paperh", .kind = argInt, .val = &paperHeight, .size = 0, .usage = "paper height, in points" },
    { .arg = "-nocrop", .kind = argFlag, .val = &noCrop, .size = 0, .usage = "don't crop pages to CropBox" },
    { .arg = "-expand", .kind = argFlag, .val = &expand, .size = 0, .usage = "expand pages smaller than the paper size" },
    { .arg = "-noshrink", .kind = argFlag, .val = &noShrink, .size = 0, .usage = "don't shrink pages larger than the paper size" },
    { .arg = "-nocenter", .kind = argFlag, .val = &noCenter, .size = 0, .usage = "don't center pages smaller than the paper size" },
    { .arg = "-duplex", .kind = argFlag, .val = &duplex, .size = 0, .usage = "enable duplex printing" },
    { .arg = "-opw", .kind = argString, .val = ownerPassword, .size = sizeof(ownerPassword), .usage = "owner password (for encrypted files)" },
    { .arg = "-upw", .kind = argString, .val = userPassword, .size = sizeof(userPassword), .usage = "user password (for encrypted files)" },
    { .arg = "-overprint", .kind = argFlag, .val = &overprint, .size = 0, .usage = "enable overprint emulation during rasterization" },
    { .arg = "-q", .kind = argFlag, .val = &quiet, .size = 0, .usage = "don't print any messages or errors" },
    { .arg = "-v", .kind = argFlag, .val = &printVersion, .size = 0, .usage = "print copyright and version info" },
    { .arg = "-h", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "-help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "--help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    { .arg = "-?", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
    {}
};

int main(int argc, char *argv[])
{
    std::unique_ptr<PDFDoc> doc;
    GooString *fileName;
    std::string psFileName;
    PSLevel level;
    PSOutMode mode;
    std::optional<GooString> ownerPW, userPW;
    PSOutputDev *psOut;
    bool ok;
    int exitCode;
    bool rasterAntialias = false;
    std::vector<int> pages;
#if USE_CMS
    cmsColorSpaceSignature profilecolorspace;
#endif

    Win32Console win32Console(&argc, &argv);
    exitCode = 99;

    // parse args
    ok = parseArgs(argDesc, &argc, argv);
    if (!ok || argc < 2 || argc > 3 || printVersion || printHelp) {
        fprintf(stderr, "pdftops version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdftops", "<PDF-file> [<PS-file>]", argDesc);
        }
        if (printVersion || printHelp) {
            exit(0);
        } else {
            exit(1);
        }
    }
    if ((level1 ? 1 : 0) + (level1Sep ? 1 : 0) + (level2 ? 1 : 0) + (level2Sep ? 1 : 0) + (level3 ? 1 : 0) + (level3Sep ? 1 : 0) > 1) {
        fprintf(stderr, "Error: use only one of the 'level' options.\n");
        exit(1);
    }
    if ((doEPS ? 1 : 0) + (doForm ? 1 : 0) > 1) {
        fprintf(stderr, "Error: use only one of -eps, and -form\n");
        exit(1);
    }
    if (level1) {
        level = psLevel1;
    } else if (level1Sep) {
        level = psLevel1Sep;
    } else if (level2Sep) {
        level = psLevel2Sep;
    } else if (level3) {
        level = psLevel3;
    } else if (level3Sep) {
        level = psLevel3Sep;
    } else {
        level = psLevel2;
    }
    if (doForm && level < psLevel2) {
        fprintf(stderr, "Error: forms are only available with Level 2 output.\n");
        exit(1);
    }
    mode = doEPS ? psModeEPS : doForm ? psModeForm : psModePS;
    fileName = new GooString(argv[1]);

    // read config file
    globalParams = std::make_unique<GlobalParams>();
    if (origPageSizes) {
        paperWidth = paperHeight = -1;
    }
    if (paperSize[0]) {
        if (origPageSizes) {
            fprintf(stderr, "Error: -origpagesizes and -paper may not be used together.\n");
            exit(1);
        }
        if (!setPSPaperSize(paperSize, paperWidth, paperHeight)) {
            fprintf(stderr, "Invalid paper size\n");
            delete fileName;
            goto err0;
        }
    }
    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    if (!processcolorformatname.toStr().empty()) {
        if (processcolorformatname.toStr() == "MONO8") {
            processcolorformat = splashModeMono8;
            processcolorformatspecified = true;
        } else if (processcolorformatname.toStr() == "CMYK8") {
            processcolorformat = splashModeCMYK8;
            processcolorformatspecified = true;
        } else if (processcolorformatname.toStr() == "RGB8") {
            processcolorformat = splashModeRGB8;
            processcolorformatspecified = true;
        } else {
            fprintf(stderr, "Error: Unknown process color format \"%s\".\n", processcolorformatname.c_str());
            goto err1;
        }
    }

#if USE_CMS
    if (!processcolorprofilename.toStr().empty()) {
        processcolorprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(processcolorprofilename.c_str(), "r"));
        if (!processcolorprofile) {
            fprintf(stderr, "Error: Could not open the ICC profile \"%s\".\n", processcolorprofilename.c_str());
            goto err1;
        }
        if (!cmsIsIntentSupported(processcolorprofile.get(), INTENT_RELATIVE_COLORIMETRIC, LCMS_USED_AS_OUTPUT) && !cmsIsIntentSupported(processcolorprofile.get(), INTENT_ABSOLUTE_COLORIMETRIC, LCMS_USED_AS_OUTPUT)
            && !cmsIsIntentSupported(processcolorprofile.get(), INTENT_SATURATION, LCMS_USED_AS_OUTPUT) && !cmsIsIntentSupported(processcolorprofile.get(), INTENT_PERCEPTUAL, LCMS_USED_AS_OUTPUT)) {
            fprintf(stderr, "Error: ICC profile \"%s\" is not an output profile.\n", processcolorprofilename.c_str());
            goto err1;
        }
        profilecolorspace = cmsGetColorSpace(processcolorprofile.get());
        if (profilecolorspace == cmsSigCmykData) {
            if (!processcolorformatspecified) {
                processcolorformat = splashModeCMYK8;
                processcolorformatspecified = true;
            } else if (processcolorformat != splashModeCMYK8) {
                fprintf(stderr, "Error: Supplied ICC profile \"%s\" is a CMYK profile, but process color format is not CMYK8.\n", processcolorprofilename.c_str());
                goto err1;
            }
        } else if (profilecolorspace == cmsSigGrayData) {
            if (!processcolorformatspecified) {
                processcolorformat = splashModeMono8;
                processcolorformatspecified = true;
            } else if (processcolorformat != splashModeMono8) {
                fprintf(stderr, "Error: Supplied ICC profile \"%s\" is a monochrome profile, but process color format is not monochrome.\n", processcolorprofilename.c_str());
                goto err1;
            }
        } else if (profilecolorspace == cmsSigRgbData) {
            if (!processcolorformatspecified) {
                processcolorformat = splashModeRGB8;
                processcolorformatspecified = true;
            } else if (processcolorformat != splashModeRGB8) {
                fprintf(stderr, "Error: Supplied ICC profile \"%s\" is a RGB profile, but process color format is not RGB.\n", processcolorprofilename.c_str());
                goto err1;
            }
        }
    }
#endif

    if (processcolorformatspecified) {
        if (level1 && processcolorformat != splashModeMono8) {
            fprintf(stderr, "Error: Setting -level1 requires -processcolorformat MONO8");
            goto err1;
        } else if ((level1Sep || level2Sep || level3Sep || overprint) && processcolorformat != splashModeCMYK8) {
            fprintf(stderr, "Error: Setting -level1sep/-level2sep/-level3sep/-overprint requires -processcolorformat CMYK8");
            goto err1;
        }
    }

#if USE_CMS
    if (!defaultgrayprofilename.toStr().empty()) {
        defaultgrayprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultgrayprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultgrayprofile, defaultgrayprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigGrayData)) {
            goto err1;
        }
    }
    if (!defaultrgbprofilename.toStr().empty()) {
        defaultrgbprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultrgbprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultrgbprofile, defaultrgbprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigRgbData)) {
            goto err1;
        }
    }
    if (!defaultcmykprofilename.toStr().empty()) {
        defaultcmykprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultcmykprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultcmykprofile, defaultcmykprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigCmykData)) {
            goto err1;
        }
    }
#endif

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

    doc = PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW);

    if (!doc->isOk()) {
        exitCode = 1;
        goto err1;
    }

#ifdef ENFORCE_PERMISSIONS
    // check for print permission
    if (!doc->okToPrint()) {
        error(errNotAllowed, -1, "Printing this document is not allowed.");
        exitCode = 3;
        goto err1;
    }
#endif

    // construct PostScript file name
    if (argc == 3) {
        psFileName = std::string(argv[2]);
    } else if (fileName->compare("fd://0") == 0) {
        error(errCommandLine, -1, "You have to provide an output filename when reading from stdin.");
        goto err1;
    } else {
        const char *p = fileName->c_str() + fileName->size() - 4;
        if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
            psFileName = std::string(fileName->c_str(), fileName->size() - 4);

        } else {
            psFileName = fileName->toStr();
        }
        psFileName += (doEPS ? ".eps" : ".ps");
    }

    // get page range
    if (firstPage < 1) {
        firstPage = 1;
    }
    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }
    if (lastPage < firstPage) {
        error(errCommandLine, -1, "Wrong page range given: the first page ({0:d}) can not be after the last page ({1:d}).", firstPage, lastPage);
        goto err1;
    }

    // check for multi-page EPS or form
    if ((doEPS || doForm) && firstPage != lastPage) {
        error(errCommandLine, -1, "EPS and form files can only contain one page.");
        goto err1;
    }

    for (int i = firstPage; i <= lastPage; ++i) {
        pages.push_back(i);
    }

    // write PostScript file
    psOut = new PSOutputDev(psFileName.c_str(), doc.get(), nullptr, pages, mode, paperWidth, paperHeight, noCrop, duplex, /*imgLLXA*/ 0, /*imgLLYA*/ 0,
                            /*imgURXA*/ 0, /*imgURYA*/ 0, psRasterizeWhenNeeded, /*manualCtrlA*/ false, /*customCodeCbkA*/ nullptr, /*customCodeCbkDataA*/ nullptr, level);
    if (noCenter) {
        psOut->setPSCenter(false);
    }
    if (expand) {
        psOut->setPSExpandSmaller(true);
    }
    if (noShrink) {
        psOut->setPSShrinkLarger(false);
    }
    if (overprint) {
        psOut->setOverprintPreview(true);
    }

    if (rasterAntialiasStr[0]) {
        if (!GlobalParams::parseYesNo2(rasterAntialiasStr, &rasterAntialias)) {
            fprintf(stderr, "Bad '-aaRaster' value on command line\n");
        }
    }

    if (forceRasterizeStr[0]) {
        PSForceRasterize forceRasterize = psRasterizeWhenNeeded;
        if (strcmp(forceRasterizeStr, "whenneeded") == 0) {
            forceRasterize = psRasterizeWhenNeeded;
        } else if (strcmp(forceRasterizeStr, "always") == 0) {
            forceRasterize = psAlwaysRasterize;
        } else if (strcmp(forceRasterizeStr, "never") == 0) {
            forceRasterize = psNeverRasterize;
        } else {
            fprintf(stderr, "Bad '-rasterize' value on command line\n");
        }
        psOut->setForceRasterize(forceRasterize);
    }

    if (splashResolution > 0) {
        psOut->setRasterResolution(splashResolution);
    }
    if (processcolorformatspecified) {
        psOut->setProcessColorFormat(processcolorformat);
    }
#if USE_CMS
    psOut->setDisplayProfile(processcolorprofile);
    psOut->setDefaultGrayProfile(defaultgrayprofile);
    psOut->setDefaultRGBProfile(defaultrgbprofile);
    psOut->setDefaultCMYKProfile(defaultcmykprofile);
#endif
    psOut->setEmbedType1(!noEmbedT1Fonts);
    psOut->setEmbedTrueType(!noEmbedTTFonts);
    psOut->setEmbedCIDPostScript(!noEmbedCIDPSFonts);
    psOut->setEmbedCIDTrueType(!noEmbedCIDTTFonts);
    psOut->setFontPassthrough(fontPassthrough);
    psOut->setPreloadImagesForms(preload);
    psOut->setOptimizeColorSpace(optimizeColorSpace);
    psOut->setPassLevel1CustomColor(passLevel1CustomColor);
    psOut->setGenerateOPI(doOPI);
    psOut->setUseBinary(psBinary);

    psOut->setRasterAntialias(rasterAntialias);
    if (psOut->isOk()) {
        for (int i = firstPage; i <= lastPage; ++i) {
            doc->displayPage(psOut, i, 72, 72, 0, noCrop, !noCrop, true);
        }
    } else {
        delete psOut;
        exitCode = 2;
        goto err1;
    }
    delete psOut;

    exitCode = 0;

    // clean up
err1:
    delete fileName;
err0:

    return exitCode;
}
