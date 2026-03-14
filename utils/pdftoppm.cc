//========================================================================
//
// pdftoppm.cc
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2007 Ilmari Heikkinen <ilmari.heikkinen@gmail.com>
// Copyright (C) 2008 Richard Airlie <richard.airlie@maglabs.net>
// Copyright (C) 2009 Michael K. Johnson <a1237@danlj.org>
// Copyright (C) 2009 Shen Liang <shenzhuxi@gmail.com>
// Copyright (C) 2009 Stefan Thomas <thomas@eload24.com>
// Copyright (C) 2009-2011, 2015, 2018-2022, 2025, 2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010, 2012, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Jonathan Liu <net147@gmail.com>
// Copyright (C) 2010 William Bader <williambader@hotmail.com>
// Copyright (C) 2011-2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013, 2015, 2018 Adam Reichold <adamreichold@myopera.com>
// Copyright (C) 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2015 William Bader <williambader@hotmail.com>
// Copyright (C) 2018 Martin Packman <gzlist@googlemail.com>
// Copyright (C) 2019 Yves-Gaël Chény <gitlab@r0b0t.fr>
// Copyright (C) 2019-2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2019 <corentinf@free.fr>
// Copyright (C) 2019 Kris Jurka <jurka@ejurka.com>
// Copyright (C) 2019 Sébastien Berthier <s.berthier@bee-buzziness.com>
// Copyright (C) 2020 Stéfan van der Walt <sjvdwalt@gmail.com>
// Copyright (C) 2020 Philipp Knechtges <philipp-dev@knechtges.com>
// Copyright (C) 2021 Diogo Kollross <diogoko@gmail.com>
// Copyright (C) 2021 Peter Williams <peter@newton.cx>
// Copyright (C) 2022 James Cloos <cloos@jhcloos.com>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#    include <fcntl.h> // for O_BINARY
#    include <io.h> // for _setmode
#endif
#include <cstdio>
#include <cmath>
#include "parseargs.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"
#include "splash/SplashErrorCodes.h"
#include "SplashOutputDev.h"
#include "Win32Console.h"
#include "numberofcharacters.h"
#include "sanitychecks.h"

// Uncomment to build pdftoppm with pthreads
// You may also have to change the buildsystem to
// link pdftoppm to pthread library
// This is here for developer testing not user ready
// #define UTILS_USE_PTHREADS 1

#ifdef UTILS_USE_PTHREADS
#    include <cerrno>
#    include <pthread.h>
#    include <deque>
#endif // UTILS_USE_PTHREADS

#if USE_CMS
#    include <lcms2.h>
#endif

static int firstPage = 1;
static int lastPage = 0;
static bool printOnlyOdd = false;
static bool printOnlyEven = false;
static bool singleFile = false;
static bool scaleDimensionBeforeRotation = false;
static double resolution = 0.0;
static double x_resolution = 150.0;
static double y_resolution = 150.0;
static int scaleTo = 0;
static int x_scaleTo = 0;
static int y_scaleTo = 0;
static int param_x = 0;
static int param_y = 0;
static int param_w = 0;
static int param_h = 0;
static int sz = 0;
static bool hideAnnotations = false;
static bool useCropBox = false;
static bool mono = false;
static bool gray = false;
#if USE_CMS
static GooString displayprofilename;
static GfxLCMSProfilePtr displayprofile;
static GooString defaultgrayprofilename;
static GfxLCMSProfilePtr defaultgrayprofile;
static GooString defaultrgbprofilename;
static GfxLCMSProfilePtr defaultrgbprofile;
static GooString defaultcmykprofilename;
static GfxLCMSProfilePtr defaultcmykprofile;
#endif
static char sep[2] = "-";
static bool forceNum = false;
static bool png = false;
static bool jpeg = false;
static bool jpegcmyk = false;
static bool tiff = false;
static GooString jpegOpt;
static int jpegQuality = -1;
static bool jpegProgressive = false;
static bool jpegOptimize = false;
static bool overprint = false;
static bool splashOverprintPreview = false;
static char enableFreeTypeStr[16] = "";
static bool enableFreeType = true;
static char antialiasStr[16] = "";
static char vectorAntialiasStr[16] = "";
static bool fontAntialias = true;
static bool vectorAntialias = true;
static char ownerPassword[33] = "";
static char userPassword[33] = "";
static char TiffCompressionStr[16] = "";
static char thinLineModeStr[8] = "";
static SplashThinLineMode thinLineMode = splashThinLineDefault;
#ifdef UTILS_USE_PTHREADS
static int numberOfJobs = 1;
#endif // UTILS_USE_PTHREADS
static bool quiet = false;
static bool progress = false;
static bool printVersion = false;
static bool printHelp = false;

static const ArgDesc argDesc[] = { { .arg = "-f", .kind = argInt, .val = &firstPage, .size = 0, .usage = "first page to print" },
                                   { .arg = "-l", .kind = argInt, .val = &lastPage, .size = 0, .usage = "last page to print" },
                                   { .arg = "-o", .kind = argFlag, .val = &printOnlyOdd, .size = 0, .usage = "print only odd pages" },
                                   { .arg = "-e", .kind = argFlag, .val = &printOnlyEven, .size = 0, .usage = "print only even pages" },
                                   { .arg = "-singlefile", .kind = argFlag, .val = &singleFile, .size = 0, .usage = "write only the first page and do not add digits" },
                                   { .arg = "-scale-dimension-before-rotation", .kind = argFlag, .val = &scaleDimensionBeforeRotation, .size = 0, .usage = "for rotated pdf, resize dimensions before the rotation" },

                                   { .arg = "-r", .kind = argFP, .val = &resolution, .size = 0, .usage = "resolution, in DPI (default is 150)" },
                                   { .arg = "-rx", .kind = argFP, .val = &x_resolution, .size = 0, .usage = "X resolution, in DPI (default is 150)" },
                                   { .arg = "-ry", .kind = argFP, .val = &y_resolution, .size = 0, .usage = "Y resolution, in DPI (default is 150)" },
                                   { .arg = "-scale-to", .kind = argInt, .val = &scaleTo, .size = 0, .usage = "scales each page to fit within scale-to*scale-to pixel box" },
                                   { .arg = "-scale-to-x", .kind = argInt, .val = &x_scaleTo, .size = 0, .usage = "scales each page horizontally to fit in scale-to-x pixels" },
                                   { .arg = "-scale-to-y", .kind = argInt, .val = &y_scaleTo, .size = 0, .usage = "scales each page vertically to fit in scale-to-y pixels" },

                                   { .arg = "-x", .kind = argInt, .val = &param_x, .size = 0, .usage = "x-coordinate of the crop area top left corner" },
                                   { .arg = "-y", .kind = argInt, .val = &param_y, .size = 0, .usage = "y-coordinate of the crop area top left corner" },
                                   { .arg = "-W", .kind = argInt, .val = &param_w, .size = 0, .usage = "width of crop area in pixels (default is 0)" },
                                   { .arg = "-H", .kind = argInt, .val = &param_h, .size = 0, .usage = "height of crop area in pixels (default is 0)" },
                                   { .arg = "-sz", .kind = argInt, .val = &sz, .size = 0, .usage = "size of crop square in pixels (sets W and H)" },
                                   { .arg = "-cropbox", .kind = argFlag, .val = &useCropBox, .size = 0, .usage = "use the crop box rather than media box" },
                                   { .arg = "-hide-annotations", .kind = argFlag, .val = &hideAnnotations, .size = 0, .usage = "do not show annotations" },

                                   { .arg = "-mono", .kind = argFlag, .val = &mono, .size = 0, .usage = "generate a monochrome PBM file" },
                                   { .arg = "-gray", .kind = argFlag, .val = &gray, .size = 0, .usage = "generate a grayscale PGM file" },
#if USE_CMS
                                   { .arg = "-displayprofile", .kind = argGooString, .val = &displayprofilename, .size = 0, .usage = "ICC color profile to use as the display profile" },
                                   { .arg = "-defaultgrayprofile", .kind = argGooString, .val = &defaultgrayprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultGray color space" },
                                   { .arg = "-defaultrgbprofile", .kind = argGooString, .val = &defaultrgbprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultRGB color space" },
                                   { .arg = "-defaultcmykprofile", .kind = argGooString, .val = &defaultcmykprofilename, .size = 0, .usage = "ICC color profile to use as the DefaultCMYK color space" },
#endif
                                   { .arg = "-sep", .kind = argString, .val = sep, .size = sizeof(sep), .usage = "single character separator between name and page number, default - " },
                                   { .arg = "-forcenum", .kind = argFlag, .val = &forceNum, .size = 0, .usage = "force page number even if there is only one page " },
#if ENABLE_LIBPNG
                                   { .arg = "-png", .kind = argFlag, .val = &png, .size = 0, .usage = "generate a PNG file" },
#endif
#if ENABLE_LIBJPEG
                                   { .arg = "-jpeg", .kind = argFlag, .val = &jpeg, .size = 0, .usage = "generate a JPEG file" },
                                   { .arg = "-jpegcmyk", .kind = argFlag, .val = &jpegcmyk, .size = 0, .usage = "generate a CMYK JPEG file" },
                                   { .arg = "-jpegopt", .kind = argGooString, .val = &jpegOpt, .size = 0, .usage = "jpeg options, with format <opt1>=<val1>[,<optN>=<valN>]*" },
#endif
                                   { .arg = "-overprint", .kind = argFlag, .val = &overprint, .size = 0, .usage = "enable overprint" },
#if ENABLE_LIBTIFF
                                   { .arg = "-tiff", .kind = argFlag, .val = &tiff, .size = 0, .usage = "generate a TIFF file" },
                                   { .arg = "-tiffcompression", .kind = argString, .val = TiffCompressionStr, .size = sizeof(TiffCompressionStr), .usage = "set TIFF compression: none, packbits, jpeg, lzw, deflate" },
#endif
                                   { .arg = "-freetype", .kind = argString, .val = enableFreeTypeStr, .size = sizeof(enableFreeTypeStr), .usage = "enable FreeType font rasterizer: yes, no" },
                                   { .arg = "-thinlinemode", .kind = argString, .val = thinLineModeStr, .size = sizeof(thinLineModeStr), .usage = "set thin line mode: none, solid, shape. Default: none" },

                                   { .arg = "-aa", .kind = argString, .val = antialiasStr, .size = sizeof(antialiasStr), .usage = "enable font anti-aliasing: yes, no" },
                                   { .arg = "-aaVector", .kind = argString, .val = vectorAntialiasStr, .size = sizeof(vectorAntialiasStr), .usage = "enable vector anti-aliasing: yes, no" },

                                   { .arg = "-opw", .kind = argString, .val = ownerPassword, .size = sizeof(ownerPassword), .usage = "owner password (for encrypted files)" },
                                   { .arg = "-upw", .kind = argString, .val = userPassword, .size = sizeof(userPassword), .usage = "user password (for encrypted files)" },

#ifdef UTILS_USE_PTHREADS
                                   { "-j", argInt, &numberOfJobs, 0, "number of jobs to run concurrently" },
#endif // UTILS_USE_PTHREADS

                                   { .arg = "-q", .kind = argFlag, .val = &quiet, .size = 0, .usage = "don't print any messages or errors" },
                                   { .arg = "-progress", .kind = argFlag, .val = &progress, .size = 0, .usage = "print progress info" },
                                   { .arg = "-v", .kind = argFlag, .val = &printVersion, .size = 0, .usage = "print copyright and version info" },
                                   { .arg = "-h", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "-help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "--help", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   { .arg = "-?", .kind = argFlag, .val = &printHelp, .size = 0, .usage = "print usage information" },
                                   {} };

static constexpr int kOtherError = 99;

static bool needToRotate(int angle)
{
    return (angle == 90) || (angle == 270);
}

static bool parseJpegOptions()
{
    // jpegOpt format is: <opt1>=<val1>,<opt2>=<val2>,...
    const char *nextOpt = jpegOpt.c_str();
    while (nextOpt && *nextOpt) {
        const char *comma = strchr(nextOpt, ',');
        GooString opt;
        if (comma) {
            opt.assign(nextOpt, static_cast<int>(comma - nextOpt));
            nextOpt = comma + 1;
        } else {
            opt.assign(nextOpt);
            nextOpt = nullptr;
        }
        // here opt is "<optN>=<valN> "
        const char *equal = strchr(opt.c_str(), '=');
        if (!equal) {
            fprintf(stderr, "Unknown jpeg option \"%s\"\n", opt.c_str());
            return false;
        }
        const int iequal = static_cast<int>(equal - opt.c_str());
        GooString value(&opt, iequal + 1, opt.size() - iequal - 1);
        opt.erase(iequal, opt.size() - iequal);
        // here opt is "<optN>" and value is "<valN>"

        if (opt.compare("quality") == 0) {
            if (!isInt(value.c_str())) {
                fprintf(stderr, "Invalid jpeg quality\n");
                return false;
            }
            jpegQuality = atoi(value.c_str());
            if (jpegQuality < 0 || jpegQuality > 100) {
                fprintf(stderr, "jpeg quality must be between 0 and 100\n");
                return false;
            }
        } else if (opt.compare("progressive") == 0) {
            jpegProgressive = false;
            if (value.compare("y") == 0) {
                jpegProgressive = true;
            } else if (value.compare("n") != 0) {
                fprintf(stderr, "jpeg progressive option must be \"y\" or \"n\"\n");
                return false;
            }
        } else if (opt.compare("optimize") == 0 || opt.compare("optimise") == 0) {
            jpegOptimize = false;
            if (value.compare("y") == 0) {
                jpegOptimize = true;
            } else if (value.compare("n") != 0) {
                fprintf(stderr, "jpeg optimize option must be \"y\" or \"n\"\n");
                return false;
            }
        } else {
            fprintf(stderr, "Unknown jpeg option \"%s\"\n", opt.c_str());
            return false;
        }
    }
    return true;
}

static auto annotDisplayDecideCbk = [](Annot * /*annot*/, void * /*user_data*/) { return !hideAnnotations; };

static void savePageSlice(PDFDoc *doc, SplashOutputDev *splashOut, int pg, int x, int y, int w, int h, double pg_w, double pg_h, char *ppmFile)
{
    if (w == 0) {
        w = (int)ceil(pg_w);
    }
    if (h == 0) {
        h = (int)ceil(pg_h);
    }
    w = (x + w > pg_w ? (int)ceil(pg_w - x) : w);
    h = (y + h > pg_h ? (int)ceil(pg_h - y) : h);
    doc->displayPageSlice(splashOut, pg, x_resolution, y_resolution, 0, !useCropBox, false, false, x, y, w, h, nullptr, nullptr, annotDisplayDecideCbk, nullptr);

    SplashBitmap *bitmap = splashOut->getBitmap();

    SplashBitmap::WriteImgParams params;
    params.jpegQuality = jpegQuality;
    params.jpegProgressive = jpegProgressive;
    params.jpegOptimize = jpegOptimize;
    params.tiffCompression = TiffCompressionStr;

    if (ppmFile != nullptr) {
        SplashError e;

        if (png) {
            e = bitmap->writeImgFile(splashFormatPng, ppmFile, x_resolution, y_resolution);
        } else if (jpeg) {
            e = bitmap->writeImgFile(splashFormatJpeg, ppmFile, x_resolution, y_resolution, &params);
        } else if (jpegcmyk) {
            e = bitmap->writeImgFile(splashFormatJpegCMYK, ppmFile, x_resolution, y_resolution, &params);
        } else if (tiff) {
            e = bitmap->writeImgFile(splashFormatTiff, ppmFile, x_resolution, y_resolution, &params);
        } else {
            e = bitmap->writePNMFile(ppmFile);
        }
        if (e != SplashError::NoError) {
            fprintf(stderr, "Could not write image to %s; exiting\n", ppmFile);
            exit(EXIT_FAILURE);
        }
    } else {
#if defined(_WIN32) || defined(__CYGWIN__)
        _setmode(fileno(stdout), O_BINARY);
#endif

        if (png) {
            bitmap->writeImgFile(splashFormatPng, stdout, x_resolution, y_resolution);
        } else if (jpeg) {
            bitmap->writeImgFile(splashFormatJpeg, stdout, x_resolution, y_resolution, &params);
        } else if (tiff) {
            bitmap->writeImgFile(splashFormatTiff, stdout, x_resolution, y_resolution, &params);
        } else {
            bitmap->writePNMFile(stdout);
        }
    }

    if (progress) {
        fprintf(stderr, "%d %d %s\n", pg, lastPage, ppmFile != nullptr ? ppmFile : "");
    }
}

#ifdef UTILS_USE_PTHREADS

struct PageJob
{
    PDFDoc *doc;
    int pg;

    double pg_w, pg_h;
    SplashColor *paperColor;

    char *ppmFile;
};

static std::deque<PageJob> pageJobQueue;
static pthread_mutex_t pageJobMutex = PTHREAD_MUTEX_INITIALIZER;

static void processPageJobs()
{
    while (true) {
        // pop the next job or exit if queue is empty
        pthread_mutex_lock(&pageJobMutex);

        if (pageJobQueue.empty()) {
            pthread_mutex_unlock(&pageJobMutex);
            return;
        }

        PageJob pageJob = pageJobQueue.front();
        pageJobQueue.pop_front();

        pthread_mutex_unlock(&pageJobMutex);

        // process the job
        SplashOutputDev *splashOut = new SplashOutputDev(mono                              ? splashModeMono1
                                                                 : gray                    ? splashModeMono8
                                                                 : (jpegcmyk || overprint) ? splashModeDeviceN8
                                                                                           : splashModeRGB8,
                                                         4, false, *pageJob.paperColor, true, thinLineMode, splashOverprintPreview);
        splashOut->setFontAntialias(fontAntialias);
        splashOut->setVectorAntialias(vectorAntialias);
        splashOut->setEnableFreeType(enableFreeType);
#    if USE_CMS
        splashOut->setDisplayProfile(displayprofile);
        splashOut->setDefaultGrayProfile(defaultgrayprofile);
        splashOut->setDefaultRGBProfile(defaultrgbprofile);
        splashOut->setDefaultCMYKProfile(defaultcmykprofile);
#    endif
        splashOut->startDoc(pageJob.doc);

        savePageSlice(pageJob.doc, splashOut, pageJob.pg, param_x, param_y, param_w, param_h, pageJob.pg_w, pageJob.pg_h, pageJob.ppmFile);

        delete splashOut;
        delete[] pageJob.ppmFile;
    }
}

#endif // UTILS_USE_PTHREADS

int main(int argc, char *argv[])
{
    GooString *fileName = nullptr;
    char *ppmRoot = nullptr;
    char *ppmFile;
    std::optional<GooString> ownerPW, userPW;
    SplashColor paperColor;
#ifndef UTILS_USE_PTHREADS
    SplashOutputDev *splashOut;
#else
    pthread_t *jobs;
#endif // UTILS_USE_PTHREADS
    bool ok;
    int pg, pg_num_len;
    double pg_w, pg_h;
#if USE_CMS
    cmsColorSpaceSignature profilecolorspace;
#endif

    Win32Console win32Console(&argc, &argv);

    // parse args
    ok = parseArgs(argDesc, &argc, argv);
    if (mono && gray) {
        ok = false;
    }
    if (resolution != 0.0 && (x_resolution == 150.0 || y_resolution == 150.0)) {
        x_resolution = resolution;
        y_resolution = resolution;
    }
    if (!ok || argc > 3 || printVersion || printHelp) {
        fprintf(stderr, "pdftoppm version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdftoppm", "[PDF-file [PPM-file-prefix]]", argDesc);
        }
        if (printVersion || printHelp) {
            return 0;
        }
        return kOtherError;
    }
    if (argc > 1) {
        fileName = new GooString(argv[1]);
    }
    if (argc == 3) {
        ppmRoot = argv[2];
    }

    if (antialiasStr[0]) {
        if (!GlobalParams::parseYesNo2(antialiasStr, &fontAntialias)) {
            fprintf(stderr, "Bad '-aa' value on command line\n");
        }
    }
    if (vectorAntialiasStr[0]) {
        if (!GlobalParams::parseYesNo2(vectorAntialiasStr, &vectorAntialias)) {
            fprintf(stderr, "Bad '-aaVector' value on command line\n");
        }
    }

    if (!jpegOpt.empty()) {
        if (!jpeg) {
            fprintf(stderr, "Warning: -jpegopt only valid with jpeg output.\n");
        }
        parseJpegOptions();
    }

    // read config file
    globalParams = std::make_unique<GlobalParams>();
    if (enableFreeTypeStr[0]) {
        if (!GlobalParams::parseYesNo2(enableFreeTypeStr, &enableFreeType)) {
            fprintf(stderr, "Bad '-freetype' value on command line\n");
        }
    }
    if (thinLineModeStr[0]) {
        if (strcmp(thinLineModeStr, "solid") == 0) {
            thinLineMode = splashThinLineSolid;
        } else if (strcmp(thinLineModeStr, "shape") == 0) {
            thinLineMode = splashThinLineShape;
        } else if (strcmp(thinLineModeStr, "none") != 0) {
            fprintf(stderr, "Bad '-thinlinemode' value on command line\n");
        }
    }
    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    // open PDF file
    if (ownerPassword[0]) {
        ownerPW = GooString(ownerPassword);
    }
    if (userPassword[0]) {
        userPW = GooString(userPassword);
    }

    if (fileName == nullptr) {
        fileName = new GooString("fd://0");
    }
    if (fileName->compare("-") == 0) {
        delete fileName;
        fileName = new GooString("fd://0");
    }
    std::unique_ptr<PDFDoc> doc(PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW));
    delete fileName;
    if (!doc->isOk()) {
        return 1;
    }

    // get page range
    if (firstPage < 1) {
        firstPage = 1;
    }
    if (singleFile && lastPage < 1) {
        lastPage = firstPage;
    }
    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }
    if (lastPage < firstPage) {
        fprintf(stderr, "Wrong page range given: the first page (%d) can not be after the last page (%d).\n", firstPage, lastPage);
        return kOtherError;
    }

    // If our page range selection and document size indicate we're only
    // outputting a single page, ensure that even/odd page selection doesn't
    // filter out that single page.
    if (firstPage == lastPage && ((printOnlyEven && firstPage % 2 == 1) || (printOnlyOdd && firstPage % 2 == 0))) {
        fprintf(stderr, "Invalid even/odd page selection, no pages match criteria.\n");
        return kOtherError;
    }

    if (singleFile && firstPage < lastPage) {
        if (!quiet) {
            fprintf(stderr, "Warning: Single file will write only the first of the %d pages.\n", lastPage + 1 - firstPage);
        }
        lastPage = firstPage;
    }

    // write PPM files
    if (jpegcmyk || overprint) {
        splashOverprintPreview = true;
        splashClearColor(paperColor);
    } else {
        paperColor[0] = 255;
        paperColor[1] = 255;
        paperColor[2] = 255;
    }

#if USE_CMS
    if (!displayprofilename.toStr().empty()) {
        displayprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(displayprofilename.c_str(), "r"));
        if (!displayprofile) {
            fprintf(stderr, "Could not open the ICC profile \"%s\".\n", displayprofilename.c_str());
            return kOtherError;
        }
        if (!cmsIsIntentSupported(displayprofile.get(), INTENT_RELATIVE_COLORIMETRIC, LCMS_USED_AS_OUTPUT) && !cmsIsIntentSupported(displayprofile.get(), INTENT_ABSOLUTE_COLORIMETRIC, LCMS_USED_AS_OUTPUT)
            && !cmsIsIntentSupported(displayprofile.get(), INTENT_SATURATION, LCMS_USED_AS_OUTPUT) && !cmsIsIntentSupported(displayprofile.get(), INTENT_PERCEPTUAL, LCMS_USED_AS_OUTPUT)) {
            fprintf(stderr, "ICC profile \"%s\" is not an output profile.\n", displayprofilename.c_str());
            return kOtherError;
        }
        profilecolorspace = cmsGetColorSpace(displayprofile.get());
        // Note: In contrast to pdftops we do not fail if a non-matching ICC profile is supplied.
        //       Doing so would be pretentious, since SplashOutputDev by default assumes sRGB, even for
        //       the CMYK and Mono cases.
        if (jpegcmyk || overprint) {
            if (profilecolorspace != cmsSigCmykData) {
                fprintf(stderr, "Warning: Supplied ICC profile \"%s\" is not a CMYK profile.\n", displayprofilename.c_str());
            }
        } else if (mono || gray) {
            if (profilecolorspace != cmsSigGrayData) {
                fprintf(stderr, "Warning: Supplied ICC profile \"%s\" is not a monochrome profile.\n", displayprofilename.c_str());
            }
        } else {
            if (profilecolorspace != cmsSigRgbData) {
                fprintf(stderr, "Warning: Supplied ICC profile \"%s\" is not a RGB profile.\n", displayprofilename.c_str());
            }
        }
    }
    if (!defaultgrayprofilename.toStr().empty()) {
        defaultgrayprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultgrayprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultgrayprofile, defaultgrayprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigGrayData)) {
            return kOtherError;
        }
    }
    if (!defaultrgbprofilename.toStr().empty()) {
        defaultrgbprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultrgbprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultrgbprofile, defaultrgbprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigRgbData)) {
            return kOtherError;
        }
    }
    if (!defaultcmykprofilename.toStr().empty()) {
        defaultcmykprofile = make_GfxLCMSProfilePtr(cmsOpenProfileFromFile(defaultcmykprofilename.c_str(), "r"));
        if (!checkICCProfile(defaultcmykprofile, defaultcmykprofilename.c_str(), LCMS_USED_AS_INPUT, cmsSigCmykData)) {
            return kOtherError;
        }
    }
#endif

#ifndef UTILS_USE_PTHREADS

    splashOut = new SplashOutputDev(mono ? splashModeMono1 : gray ? splashModeMono8 : (jpegcmyk || overprint) ? splashModeDeviceN8 : splashModeRGB8, 4, paperColor, true, thinLineMode, splashOverprintPreview);

    splashOut->setFontAntialias(fontAntialias);
    splashOut->setVectorAntialias(vectorAntialias);
    splashOut->setEnableFreeType(enableFreeType);
#    if USE_CMS
    splashOut->setDisplayProfile(displayprofile);
    splashOut->setDefaultGrayProfile(defaultgrayprofile);
    splashOut->setDefaultRGBProfile(defaultrgbprofile);
    splashOut->setDefaultCMYKProfile(defaultcmykprofile);
#    endif
    splashOut->startDoc(doc.get());

#endif // UTILS_USE_PTHREADS

    if (sz != 0) {
        param_w = param_h = sz;
    }
    pg_num_len = numberOfCharacters(doc->getNumPages());
    for (pg = firstPage; pg <= lastPage; ++pg) {
        if (printOnlyEven && pg % 2 == 1) {
            continue;
        }
        if (printOnlyOdd && pg % 2 == 0) {
            continue;
        }
        if (useCropBox) {
            pg_w = doc->getPageCropWidth(pg);
            pg_h = doc->getPageCropHeight(pg);
        } else {
            pg_w = doc->getPageMediaWidth(pg);
            pg_h = doc->getPageMediaHeight(pg);
        }

        if (scaleDimensionBeforeRotation && needToRotate(doc->getPageRotate(pg))) {
            std::swap(pg_w, pg_h);
        }

        // Handle requests for specific image size
        if (scaleTo != 0) {
            if (pg_w > pg_h) {
                resolution = (72.0 * scaleTo) / pg_w;
                pg_w = scaleTo;
                pg_h = pg_h * (resolution / 72.0);
            } else {
                resolution = (72.0 * scaleTo) / pg_h;
                pg_h = scaleTo;
                pg_w = pg_w * (resolution / 72.0);
            }
            x_resolution = y_resolution = resolution;
        } else {
            if (x_scaleTo > 0) {
                x_resolution = (72.0 * x_scaleTo) / pg_w;
                pg_w = x_scaleTo;
                if (y_scaleTo == -1) {
                    y_resolution = x_resolution;
                }
            }

            if (y_scaleTo > 0) {
                y_resolution = (72.0 * y_scaleTo) / pg_h;
                pg_h = y_scaleTo;
                if (x_scaleTo == -1) {
                    x_resolution = y_resolution;
                }
            }

            // No specific image size requested---compute the size from the resolution
            if (x_scaleTo <= 0) {
                pg_w = pg_w * x_resolution / 72.0;
            }
            if (y_scaleTo <= 0) {
                pg_h = pg_h * y_resolution / 72.0;
            }
        }

        if (!scaleDimensionBeforeRotation && needToRotate(doc->getPageRotate(pg))) {
            std::swap(pg_w, pg_h);
        }

        if (ppmRoot != nullptr) {
            const char *ext = png ? "png" : (jpeg || jpegcmyk) ? "jpg" : tiff ? "tif" : mono ? "pbm" : gray ? "pgm" : "ppm";
            if (singleFile && !forceNum) {
                ppmFile = new char[strlen(ppmRoot) + 1 + strlen(ext) + 1];
                sprintf(ppmFile, "%s.%s", ppmRoot, ext);
            } else {
                ppmFile = new char[strlen(ppmRoot) + 1 + pg_num_len + 1 + strlen(ext) + 1];
                sprintf(ppmFile, "%s%s%0*d.%s", ppmRoot, sep, pg_num_len, pg, ext);
            }
        } else {
            ppmFile = nullptr;
        }
#ifndef UTILS_USE_PTHREADS
        // process job in main thread
        savePageSlice(doc.get(), splashOut, pg, param_x, param_y, param_w, param_h, pg_w, pg_h, ppmFile);

        delete[] ppmFile;
#else

        // queue job for worker threads
        PageJob pageJob = { .doc = doc.get(),
                            .pg = pg,

                            .pg_w = pg_w,
                            .pg_h = pg_h,

                            .paperColor = &paperColor,

                            .ppmFile = ppmFile };

        pageJobQueue.push_back(pageJob);

#endif // UTILS_USE_PTHREADS
    }
#ifndef UTILS_USE_PTHREADS
    delete splashOut;
#else

    // spawn worker threads and wait on them
    jobs = (pthread_t *)malloc(numberOfJobs * sizeof(pthread_t));

    for (int i = 0; i < numberOfJobs; ++i) {
        if (pthread_create(&jobs[i], NULL, (void *(*)(void *))processPageJobs, NULL) != 0) {
            fprintf(stderr, "pthread_create() failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfJobs; ++i) {
        if (pthread_join(jobs[i], NULL) != 0) {
            fprintf(stderr, "pthread_join() failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    free(jobs);

#endif // UTILS_USE_PTHREADS

    return 0;
}
