//========================================================================
//
// ImageOutputDev.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005, 2007, 2011, 2018, 2019, 2021, 2022, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Rainer Keller <class321@gmx.de>
// Copyright (C) 2008 Timothy Lee <timothy.lee@siriushk.com>
// Copyright (C) 2008 Vasile Gaburici <gaburici@cs.umd.edu>
// Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 William Bader <williambader@hotmail.com>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
// Copyright (C) 2012, 2013, 2017, 2018 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Thomas Fischer <fischer@unix-ag.uni-kl.de>
// Copyright (C) 2013 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2017 Caolán McNamara <caolanm@redhat.com>
// Copyright (C) 2018 Andreas Gruenbacher <agruenba@redhat.com>
// Copyright (C) 2020 mrbax <12640-mrbax@users.noreply.gitlab.freedesktop.org>
// Copyright (C) 2024 Fernando Herrera <fherrera@onirica.com>
// Copyright (C) 2024 Sebastian J. Bronner <waschtl@sbronner.com>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cctype>
#include <cmath>
#include "goo/gmem.h"
#include "goo/NetPBMWriter.h"
#include "goo/PNGWriter.h"
#include "goo/TiffWriter.h"
#include "Error.h"
#include "GfxState.h"
#include "Object.h"
#include "Stream.h"
#include "JBIG2Stream.h"
#include "ImageOutputDev.h"

ImageOutputDev::ImageOutputDev(char *fileRootA, bool pageNamesA, bool listImagesA)
{
    listImages = listImagesA;
    if (!listImages) {
        fileRoot = copyString(fileRootA);
        fileName = (char *)gmalloc(strlen(fileRoot) + 45);
    }
    outputPNG = false;
    outputTiff = false;
    dumpJPEG = false;
    dumpJP2 = false;
    dumpJBIG2 = false;
    dumpCCITT = false;
    pageNames = pageNamesA;
    printFilenames = false;
    imgNum = 0;
    pageNum = 0;
    errorCode = 0;
    if (listImages) {
        printf("page   num  type   width height color comp bpc  enc interp  object ID x-ppi y-ppi size ratio\n");
        printf("--------------------------------------------------------------------------------------------\n");
    }
}

ImageOutputDev::~ImageOutputDev()
{
    if (!listImages) {
        gfree(fileName);
        gfree(fileRoot);
    }
}

void ImageOutputDev::setFilename(const char *fileExt)
{
    if (pageNames) {
        sprintf(fileName, "%s-%03d-%03d.%s", fileRoot, pageNum, imgNum, fileExt);
    } else {
        sprintf(fileName, "%s-%03d.%s", fileRoot, imgNum, fileExt);
    }
}

// Print a floating point number between 0 - 9999 using 4 characters
// eg '1.23', '12.3', ' 123', '1234'
//
// We need to be careful to handle the cases where rounding adds an
// extra digit before the decimal. eg printf("%4.2f", 9.99999)
// outputs "10.00" instead of "9.99".
static void printNumber(double d)
{
    char buf[10];

    if (d < 10.0) {
        sprintf(buf, "%4.2f", d);
        buf[4] = 0;
        printf("%s", buf);
    } else if (d < 100.0) {
        sprintf(buf, "%4.1f", d);
        if (!isdigit(buf[3])) {
            buf[3] = 0;
            printf(" %s", buf);
        } else {
            printf("%s", buf);
        }
    } else {
        printf("%4.0f", d);
    }
}

void ImageOutputDev::listImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, bool inlineImg, ImageType imageType)
{
    const char *type;
    const char *colorspace;
    const char *enc;
    int components, bpc;

    printf("%4d %5d ", pageNum, imgNum);
    type = "";
    switch (imageType) {
    case imgImage:
        type = "image";
        break;
    case imgStencil:
        type = "stencil";
        break;
    case imgMask:
        type = "mask";
        break;
    case imgSmask:
        type = "smask";
        break;
    }
    printf("%-7s %5d %5d  ", type, width, height);

    colorspace = "-";
    /* masks and stencils default to ncomps = 1 and bpc = 1 */
    components = 1;
    bpc = 1;
    if (colorMap && colorMap->isOk()) {
        switch (colorMap->getColorSpace()->getMode()) {
        case csDeviceGray:
        case csCalGray:
            colorspace = "gray";
            break;
        case csDeviceRGB:
        case csCalRGB:
            colorspace = "rgb";
            break;
        case csDeviceCMYK:
            colorspace = "cmyk";
            break;
        case csLab:
            colorspace = "lab";
            break;
        case csICCBased:
            colorspace = "icc";
            break;
        case csIndexed:
            colorspace = "index";
            break;
        case csSeparation:
            colorspace = "sep";
            break;
        case csDeviceN:
            colorspace = "devn";
            break;
        case csPattern:
        default:
            colorspace = "-";
            break;
        }
        components = colorMap->getNumPixelComps();
        bpc = colorMap->getBits();
    }
    printf("%-5s  %2d  %2d  ", colorspace, components, bpc);

    switch (str->getKind()) {
    case strCCITTFax:
        enc = "ccitt";
        break;
    case strDCT:
        enc = "jpeg";
        break;
    case strJPX:
        enc = "jpx";
        break;
    case strJBIG2:
        enc = "jbig2";
        break;
    case strFile:
    case strFlate:
    case strCachedFile:
    case strASCIIHex:
    case strASCII85:
    case strLZW:
    case strRunLength:
    case strWeird:
    default:
        enc = "image";
        break;
    }
    printf("%-5s  ", enc);

    printf("%-3s  ", interpolate ? "yes" : "no");

    if (inlineImg) {
        printf("[inline]   ");
    } else if (ref->isRef()) {
        const Ref imageRef = ref->getRef();
        if (imageRef.gen >= 100000) {
            printf("[none]     ");
        } else {
            printf(" %6d %2d ", imageRef.num, imageRef.gen);
        }
    } else {
        printf("[none]     ");
    }

    const double *mat = state->getCTM();
    double width2 = sqrt(mat[0] * mat[0] + mat[1] * mat[1]);
    double height2 = sqrt(mat[2] * mat[2] + mat[3] * mat[3]);
    double xppi = fabs(width * 72.0 / width2);
    double yppi = fabs(height * 72.0 / height2);
    if (xppi < 1.0) {
        printf("%5.3f ", xppi);
    } else {
        printf("%5.0f ", xppi);
    }
    if (yppi < 1.0) {
        printf("%5.3f ", yppi);
    } else {
        printf("%5.0f ", yppi);
    }

    Goffset embedSize = -1;
    if (inlineImg) {
        embedSize = getInlineImageLength(str, width, height, colorMap);
    } else {
        embedSize = str->getBaseStream()->getLength();
    }

    long long imageSize = 0;
    if (colorMap && colorMap->isOk()) {
        imageSize = ((long long)width * height * colorMap->getNumPixelComps() * colorMap->getBits()) / 8;
    } else {
        imageSize = (long long)width * height / 8; // mask
    }

    double ratio = -1.0;
    if (imageSize > 0) {
        ratio = 100.0 * embedSize / imageSize;
    }

    if (embedSize < 0) {
        printf("   - ");
    } else if (embedSize <= 9999) {
        printf("%4lldB", embedSize);
    } else {
        double d = embedSize / 1024.0;
        if (d <= 9999.0) {
            printNumber(d);
            putchar('K');
        } else {
            d /= 1024.0;
            if (d <= 9999.0) {
                printNumber(d);
                putchar('M');
            } else {
                d /= 1024.0;
                printNumber(d);
                putchar('G');
            }
        }
    }

    if (ratio > 9.9) {
        printf(" %3.0f%%\n", ratio);
    } else if (ratio >= 0.0) {
        printf(" %3.1f%%\n", ratio);
    } else {
        printf("   - \n");
    }

    ++imgNum;
}

long ImageOutputDev::getInlineImageLength(Stream *str, int width, int height, GfxImageColorMap *colorMap)
{
    long len;

    if (colorMap) {
        ImageStream imgStr(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
        if (!imgStr.reset()) {
            imgStr.close();
            return 0;
        }
        for (int y = 0; y < height; y++) {
            imgStr.getLine();
        }

        imgStr.close();
    } else {
        if (!str->reset()) {
            return 0;
        }
        for (int y = 0; y < height; y++) {
            int size = (width + 7) / 8;
            for (int x = 0; x < size; x++) {
                str->getChar();
            }
        }
    }

    EmbedStream *embedStr = (EmbedStream *)(str->getBaseStream());
    embedStr->rewind();
    len = 0;
    while (embedStr->getChar() != EOF) {
        len++;
    }

    embedStr->restore();

    return len;
}

void ImageOutputDev::writeRawImage(Stream *str, const char *ext)
{
    FILE *f;
    int c;

    // open the image file
    setFilename(ext);
    ++imgNum;
    if (!(f = fopen(fileName, "wb"))) {
        error(errIO, -1, "Couldn't open image file '{0:s}'", fileName);
        errorCode = 2;
        return;
    }

    // initialize stream
    str = str->getNextStream();
    if (!str->reset()) {
        fclose(f);
        error(errIO, -1, "Couldn't reset stream");
        errorCode = 2;
        return;
    }

    // copy the stream
    while ((c = str->getChar()) != EOF) {
        fputc(c, f);
    }

    str->close();
    fclose(f);
}

void ImageOutputDev::writeImageFile(ImgWriter *writer, ImageFormat format, const char *ext, Stream *str, int width, int height, GfxImageColorMap *colorMap)
{
    FILE *f = nullptr; /* squelch bogus compiler warning */
    ImageStream *imgStr = nullptr;
    unsigned char *rowp;
    unsigned char *p;
    GfxRGB rgb;
    GfxCMYK cmyk;
    GfxGray gray;
    unsigned char zero[gfxColorMaxComps];
    int invert_bits;

    if (writer) {
        setFilename(ext);
        ++imgNum;
        if (!(f = fopen(fileName, "wb"))) {
            error(errIO, -1, "Couldn't open image file '{0:s}'", fileName);
            errorCode = 2;
            return;
        }

        if (!writer->init(f, width, height, 72, 72)) {
            error(errIO, -1, "Error writing '{0:s}'", fileName);
            errorCode = 2;
            return;
        }
    }

    int pixelSize = sizeof(unsigned int);
    if (format == imgRGB48) {
        pixelSize = 2 * sizeof(unsigned int);
    }

    if (format != imgMonochrome) {
        // initialize stream
        imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
        if (!imgStr->reset()) {
            error(errIO, -1, "Stream reset failed");
            errorCode = 3;
            return;
        }
    } else {
        // initialize stream
        if (!str->reset()) {
            errorCode = 3;
            error(errIO, -1, "Stream reset failed");
            return;
        }
    }

    unsigned char *row = (unsigned char *)gmallocn_checkoverflow(width, pixelSize);
    if (!row) {
        error(errIO, -1, "Image data for '{0:s}' is too big. {1:d} width with {2:d} bytes per pixel", fileName, width, pixelSize);
        errorCode = 99;
        return;
    }

    // PDF masks use 0 = draw current color, 1 = leave unchanged.
    // We invert this to provide the standard interpretation of alpha
    // (0 = transparent, 1 = opaque). If the colorMap already inverts
    // the mask we leave the data unchanged.
    invert_bits = 0xff;
    if (colorMap) {
        memset(zero, 0, sizeof(zero));
        colorMap->getGray(zero, &gray);
        if (colToByte(gray) == 0) {
            invert_bits = 0x00;
        }
    }

    // for each line...
    for (int y = 0; y < height; y++) {
        switch (format) {
        case imgRGB:
            p = imgStr->getLine();
            rowp = row;
            for (int x = 0; x < width; ++x) {
                if (p) {
                    colorMap->getRGB(p, &rgb);
                    *rowp++ = colToByte(rgb.r);
                    *rowp++ = colToByte(rgb.g);
                    *rowp++ = colToByte(rgb.b);
                    p += colorMap->getNumPixelComps();
                } else {
                    *rowp++ = 0;
                    *rowp++ = 0;
                    *rowp++ = 0;
                }
            }
            if (writer) {
                writer->writeRow(&row);
            }
            break;

        case imgRGB48: {
            p = imgStr->getLine();
            unsigned short *rowp16 = reinterpret_cast<unsigned short *>(row);
            for (int x = 0; x < width; ++x) {
                if (p) {
                    colorMap->getRGB(p, &rgb);
                    *rowp16++ = colToShort(rgb.r);
                    *rowp16++ = colToShort(rgb.g);
                    *rowp16++ = colToShort(rgb.b);
                    p += colorMap->getNumPixelComps();
                } else {
                    *rowp16++ = 0;
                    *rowp16++ = 0;
                    *rowp16++ = 0;
                }
            }
            if (writer) {
                writer->writeRow(&row);
            }
            break;
        }

        case imgCMYK:
            p = imgStr->getLine();
            rowp = row;
            for (int x = 0; x < width; ++x) {
                if (p) {
                    colorMap->getCMYK(p, &cmyk);
                    *rowp++ = colToByte(cmyk.c);
                    *rowp++ = colToByte(cmyk.m);
                    *rowp++ = colToByte(cmyk.y);
                    *rowp++ = colToByte(cmyk.k);
                    p += colorMap->getNumPixelComps();
                } else {
                    *rowp++ = 0;
                    *rowp++ = 0;
                    *rowp++ = 0;
                    *rowp++ = 0;
                }
            }
            if (writer) {
                writer->writeRow(&row);
            }
            break;

        case imgGray:
            p = imgStr->getLine();
            rowp = row;
            for (int x = 0; x < width; ++x) {
                if (p) {
                    colorMap->getGray(p, &gray);
                    *rowp++ = colToByte(gray);
                    p += colorMap->getNumPixelComps();
                } else {
                    *rowp++ = 0;
                }
            }
            if (writer) {
                writer->writeRow(&row);
            }
            break;

        case imgMonochrome:
            int size = (width + 7) / 8;
            for (int x = 0; x < size; x++) {
                row[x] = str->getChar() ^ invert_bits;
            }
            if (writer) {
                writer->writeRow(&row);
            }
            break;
        }
    }

    gfree(row);
    if (format != imgMonochrome) {
        imgStr->close();
        delete imgStr;
    }
    str->close();
    if (writer) {
        writer->close();
        fclose(f);
    }
}

void ImageOutputDev::writeImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool inlineImg)
{
    ImageFormat format;
    EmbedStream *embedStr;

    if (inlineImg) {
        embedStr = (EmbedStream *)(str->getBaseStream());
        // Record the stream. This determines the size.
        getInlineImageLength(str, width, height, colorMap);
        // Reading the stream again will return EOF at end of recording.
        embedStr->rewind();
    }

    if (dumpJPEG && str->getKind() == strDCT) {
        // dump JPEG file
        writeRawImage(str, "jpg");

    } else if (dumpJP2 && str->getKind() == strJPX && !inlineImg) {
        // dump JPEG2000 file
        writeRawImage(str, "jp2");

    } else if (dumpJBIG2 && str->getKind() == strJBIG2 && !inlineImg) {
        // dump JBIG2 globals stream if available
        JBIG2Stream *jb2Str = static_cast<JBIG2Stream *>(str);
        Object *globals = jb2Str->getGlobalsStream();
        if (globals->isStream()) {
            FILE *f;
            int c;
            Stream *globalsStr = globals->getStream();

            setFilename("jb2g");
            if (!(f = fopen(fileName, "wb"))) {
                error(errIO, -1, "Couldn't open image file '{0:s}'", fileName);
                errorCode = 2;
                return;
            }
            if (globalsStr->reset()) {
                while ((c = globalsStr->getChar()) != EOF) {
                    fputc(c, f);
                }
                globalsStr->close();
            }
            fclose(f);
        }

        // dump JBIG2 embedded file
        writeRawImage(str, "jb2e");

    } else if (dumpCCITT && str->getKind() == strCCITTFax) {
        // write CCITT parameters
        CCITTFaxStream *ccittStr = static_cast<CCITTFaxStream *>(str);
        FILE *f;
        setFilename("params");
        if (!(f = fopen(fileName, "wb"))) {
            error(errIO, -1, "Couldn't open image file '{0:s}'", fileName);
            errorCode = 2;
            return;
        }
        if (ccittStr->getEncoding() < 0) {
            fprintf(f, "-4 ");
        } else if (ccittStr->getEncoding() == 0) {
            fprintf(f, "-1 ");
        } else {
            fprintf(f, "-2 ");
        }

        if (ccittStr->getEndOfLine()) {
            fprintf(f, "-A ");
        } else {
            fprintf(f, "-P ");
        }

        fprintf(f, "-X %d ", ccittStr->getColumns());

        if (ccittStr->getBlackIs1()) {
            fprintf(f, "-W ");
        } else {
            fprintf(f, "-B ");
        }

        fprintf(f, "-M\n"); // PDF uses MSB first

        fclose(f);

        // dump CCITT file
        writeRawImage(str, "ccitt");

    } else if (outputPNG && !(outputTiff && colorMap && (colorMap->getColorSpace()->getMode() == csDeviceCMYK || (colorMap->getColorSpace()->getMode() == csICCBased && colorMap->getNumPixelComps() == 4)))) {
        // output in PNG format

#ifdef ENABLE_LIBPNG
        ImgWriter *writer;

        if (!colorMap || (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1)) {
            writer = new PNGWriter(PNGWriter::MONOCHROME);
            format = imgMonochrome;
        } else if (colorMap->getColorSpace()->getMode() == csDeviceGray || colorMap->getColorSpace()->getMode() == csCalGray) {
            writer = new PNGWriter(PNGWriter::GRAY);
            format = imgGray;
        } else if ((colorMap->getColorSpace()->getMode() == csDeviceRGB || colorMap->getColorSpace()->getMode() == csCalRGB || (colorMap->getColorSpace()->getMode() == csICCBased && colorMap->getNumPixelComps() == 3))
                   && colorMap->getBits() > 8) {
            writer = new PNGWriter(PNGWriter::RGB48);
            format = imgRGB48;
        } else {
            writer = new PNGWriter(PNGWriter::RGB);
            format = imgRGB;
        }

        writeImageFile(writer, format, "png", str, width, height, colorMap);

        delete writer;
#endif
    } else if (outputTiff) {
        // output in TIFF format

#ifdef ENABLE_LIBTIFF
        ImgWriter *writer;

        if (!colorMap || (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1)) {
            writer = new TiffWriter(TiffWriter::MONOCHROME);
            format = imgMonochrome;
        } else if (colorMap->getColorSpace()->getMode() == csDeviceGray || colorMap->getColorSpace()->getMode() == csCalGray) {
            writer = new TiffWriter(TiffWriter::GRAY);
            format = imgGray;
        } else if (colorMap->getColorSpace()->getMode() == csDeviceCMYK || (colorMap->getColorSpace()->getMode() == csICCBased && colorMap->getNumPixelComps() == 4)) {
            writer = new TiffWriter(TiffWriter::CMYK);
            format = imgCMYK;
        } else if ((colorMap->getColorSpace()->getMode() == csDeviceRGB || colorMap->getColorSpace()->getMode() == csCalRGB || (colorMap->getColorSpace()->getMode() == csICCBased && colorMap->getNumPixelComps() == 3))
                   && colorMap->getBits() > 8) {
            writer = new TiffWriter(TiffWriter::RGB48);
            format = imgRGB48;
        } else {
            writer = new TiffWriter(TiffWriter::RGB);
            format = imgRGB;
        }

        writeImageFile(writer, format, "tif", str, width, height, colorMap);

        delete writer;
#endif
    } else {
        // output in PPM/PBM format
        ImgWriter *writer;

        if (!colorMap || (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1)) {
            writer = new NetPBMWriter(NetPBMWriter::MONOCHROME);
            format = imgMonochrome;
        } else {
            writer = new NetPBMWriter(NetPBMWriter::RGB);
            format = imgRGB;
        }

        writeImageFile(writer, format, format == imgRGB ? "ppm" : "pbm", str, width, height, colorMap);

        delete writer;
    }

    if (inlineImg) {
        embedStr->restore();
    }

    if (printFilenames) {
        printf("%s\n", fileName);
    }
}

bool ImageOutputDev::tilingPatternFill(GfxState *state, Gfx *gfx, Catalog *cat, GfxTilingPattern *tPat, const double *mat, int x0, int y0, int x1, int y1, double xStep, double yStep)
{
    return true;
    // do nothing -- this avoids the potentially slow loop in Gfx.cc
}

void ImageOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool interpolate, bool inlineImg)
{
    if (listImages) {
        listImage(state, ref, str, width, height, nullptr, interpolate, inlineImg, imgStencil);
    } else {
        writeImage(state, ref, str, width, height, nullptr, inlineImg);
    }
}

void ImageOutputDev::drawImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, const int *maskColors, bool inlineImg)
{
    if (listImages) {
        listImage(state, ref, str, width, height, colorMap, interpolate, inlineImg, imgImage);
    } else {
        writeImage(state, ref, str, width, height, colorMap, inlineImg);
    }
}

void ImageOutputDev::drawMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, bool maskInvert, bool maskInterpolate)
{
    if (listImages) {
        listImage(state, ref, str, width, height, colorMap, interpolate, false, imgImage);
        listImage(state, ref, maskStr, maskWidth, maskHeight, nullptr, maskInterpolate, false, imgMask);
    } else {
        writeImage(state, ref, str, width, height, colorMap, false);
        writeImage(state, ref, maskStr, maskWidth, maskHeight, nullptr, false);
    }
}

void ImageOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, GfxImageColorMap *maskColorMap,
                                         bool maskInterpolate)
{
    if (listImages) {
        listImage(state, ref, str, width, height, colorMap, interpolate, false, imgImage);
        listImage(state, ref, maskStr, maskWidth, maskHeight, maskColorMap, maskInterpolate, false, imgSmask);
    } else {
        writeImage(state, ref, str, width, height, colorMap, false);
        writeImage(state, ref, maskStr, maskWidth, maskHeight, maskColorMap, false);
    }
}
