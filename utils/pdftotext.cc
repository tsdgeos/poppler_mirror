//========================================================================
//
// pdftotext.cc
//
// Copyright 1997-2003 Glyph & Cog, LLC
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
// Copyright (C) 2006 Dominic Lachowicz <cinamod@hotmail.com>
// Copyright (C) 2007-2008, 2010, 2011, 2017-2022, 2024 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Jan Jockusch <jan@jockusch.de>
// Copyright (C) 2010, 2013 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Kenneth Berland <ken@hero.com>
// Copyright (C) 2011 Tom Gleason <tom@buildadam.com>
// Copyright (C) 2011 Steven Murdoch <Steven.Murdoch@cl.cam.ac.uk>
// Copyright (C) 2013 Yury G. Kudryashov <urkud.urkud@gmail.com>
// Copyright (C) 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2015 Jeremy Echols <jechols@uoregon.edu>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2018 Sanchit Anand <sanxchit@gmail.com>
// Copyright (C) 2019 Dan Shea <dan.shea@logical-innovations.com>
// Copyright (C) 2019, 2021 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2021 William Bader <williambader@hotmail.com>
// Copyright (C) 2022 kVdNi <kVdNi@waqa.eu>
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
#include <cstring>
#include "parseargs.h"
#include "printencodings.h"
#include "goo/GooString.h"
#include "goo/gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "TextOutputDev.h"
#include "CharTypes.h"
#include "UnicodeMap.h"
#include "PDFDocEncoding.h"
#include "Error.h"
#include <string>
#include <sstream>
#include <iomanip>
#include "Win32Console.h"
#include "DateInfo.h"
#include <cfloat>

static void printInfoString(FILE *f, Dict *infoDict, const char *key, const char *text1, const char *text2, const UnicodeMap *uMap);
static void printInfoDate(FILE *f, Dict *infoDict, const char *key, const char *text1, const char *text2);
void printDocBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last);
void printWordBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last);
void printTSVBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last);

static int firstPage = 1;
static int lastPage = 0;
static double resolution = 72.0;
static int x = 0;
static int y = 0;
static int w = 0;
static int h = 0;
static bool bbox = false;
static bool bboxLayout = false;
static bool physLayout = false;
static bool useCropBox = false;
static double colspacing = TextOutputDev::minColSpacing1_default;
static double fixedPitch = 0;
static bool rawOrder = false;
static bool discardDiag = false;
static bool htmlMeta = false;
static char textEncName[128] = "";
static char textEOLStr[16] = "";
static bool noPageBreaks = false;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool quiet = false;
static bool printVersion = false;
static bool printHelp = false;
static bool printEnc = false;
static bool tsvMode = false;

static const ArgDesc argDesc[] = { { "-f", argInt, &firstPage, 0, "first page to convert" },
                                   { "-l", argInt, &lastPage, 0, "last page to convert" },
                                   { "-r", argFP, &resolution, 0, "resolution, in DPI (default is 72)" },
                                   { "-x", argInt, &x, 0, "x-coordinate of the crop area top left corner" },
                                   { "-y", argInt, &y, 0, "y-coordinate of the crop area top left corner" },
                                   { "-W", argInt, &w, 0, "width of crop area in pixels (default is 0)" },
                                   { "-H", argInt, &h, 0, "height of crop area in pixels (default is 0)" },
                                   { "-layout", argFlag, &physLayout, 0, "maintain original physical layout" },
                                   { "-fixed", argFP, &fixedPitch, 0, "assume fixed-pitch (or tabular) text" },
                                   { "-raw", argFlag, &rawOrder, 0, "keep strings in content stream order" },
                                   { "-nodiag", argFlag, &discardDiag, 0, "discard diagonal text" },
                                   { "-htmlmeta", argFlag, &htmlMeta, 0, "generate a simple HTML file, including the meta information" },
                                   { "-tsv", argFlag, &tsvMode, 0, "generate a simple TSV file, including the meta information for bounding boxes" },
                                   { "-enc", argString, textEncName, sizeof(textEncName), "output text encoding name" },
                                   { "-listenc", argFlag, &printEnc, 0, "list available encodings" },
                                   { "-eol", argString, textEOLStr, sizeof(textEOLStr), "output end-of-line convention (unix, dos, or mac)" },
                                   { "-nopgbrk", argFlag, &noPageBreaks, 0, "don't insert page breaks between pages" },
                                   { "-bbox", argFlag, &bbox, 0, "output bounding box for each word and page size to html. Sets -htmlmeta" },
                                   { "-bbox-layout", argFlag, &bboxLayout, 0, "like -bbox but with extra layout bounding box data.  Sets -htmlmeta" },
                                   { "-cropbox", argFlag, &useCropBox, 0, "use the crop box rather than media box" },
                                   { "-colspacing", argFP, &colspacing, 0,
                                     "how much spacing we allow after a word before considering adjacent text to be a new column, as a fraction of the font size (default is 0.7, old releases had a 0.3 default)" },
                                   { "-opw", argString, ownerPassword, sizeof(ownerPassword), "owner password (for encrypted files)" },
                                   { "-upw", argString, userPassword, sizeof(userPassword), "user password (for encrypted files)" },
                                   { "-q", argFlag, &quiet, 0, "don't print any messages or errors" },
                                   { "-v", argFlag, &printVersion, 0, "print copyright and version info" },
                                   { "-h", argFlag, &printHelp, 0, "print usage information" },
                                   { "-help", argFlag, &printHelp, 0, "print usage information" },
                                   { "--help", argFlag, &printHelp, 0, "print usage information" },
                                   { "-?", argFlag, &printHelp, 0, "print usage information" },
                                   {} };

static std::string myStringReplace(const std::string &inString, const std::string &oldToken, const std::string &newToken)
{
    std::string result = inString;
    size_t foundLoc;
    int advance = 0;
    do {
        foundLoc = result.find(oldToken, advance);
        if (foundLoc != std::string::npos) {
            result.replace(foundLoc, oldToken.length(), newToken);
            advance = foundLoc + newToken.length();
        }
    } while (foundLoc != std::string::npos);
    return result;
}

static std::string myXmlTokenReplace(const char *inString)
{
    std::string myString(inString);
    myString = myStringReplace(myString, "&", "&amp;");
    myString = myStringReplace(myString, "'", "&apos;");
    myString = myStringReplace(myString, "\"", "&quot;");
    myString = myStringReplace(myString, "<", "&lt;");
    myString = myStringReplace(myString, ">", "&gt;");
    return myString;
}

int main(int argc, char *argv[])
{
    std::unique_ptr<PDFDoc> doc;
    std::unique_ptr<GooString> textFileName;
    std::optional<GooString> ownerPW, userPW;
    FILE *f;
    const UnicodeMap *uMap;
    Object info;
    bool ok;
    EndOfLineKind textEOL = TextOutputDev::defaultEndOfLine();

    Win32Console win32Console(&argc, &argv);

    // parse args
    ok = parseArgs(argDesc, &argc, argv);
    if (bboxLayout) {
        bbox = true;
    }
    if (bbox) {
        htmlMeta = true;
    }
    if (colspacing <= 0 || colspacing > 10) {
        error(errCommandLine, -1, "Bogus value provided for -colspacing");
        return 99;
    }
    if (!ok || (argc < 2 && !printEnc) || argc > 3 || printVersion || printHelp) {
        fprintf(stderr, "pdftotext version %s\n", PACKAGE_VERSION);
        fprintf(stderr, "%s\n", popplerCopyright);
        fprintf(stderr, "%s\n", xpdfCopyright);
        if (!printVersion) {
            printUsage("pdftotext", "<PDF-file> [<text-file>]", argDesc);
        }
        if (printVersion || printHelp) {
            return 0;
        }
        return 99;
    }

    // read config file
    globalParams = std::make_unique<GlobalParams>();

    if (printEnc) {
        printEncodings();
        return 0;
    }

    GooString fileName(argv[1]);
    if (fixedPitch) {
        physLayout = true;
    }

    if (textEncName[0]) {
        globalParams->setTextEncoding(textEncName);
    }
    if (textEOLStr[0]) {
        if (!strcmp(textEOLStr, "unix")) {
            textEOL = eolUnix;
        } else if (!strcmp(textEOLStr, "dos")) {
            textEOL = eolDOS;
        } else if (!strcmp(textEOLStr, "mac")) {
            textEOL = eolMac;
        } else {
            fprintf(stderr, "Bad '-eol' value on command line\n");
        }
    }
    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    // get mapping to output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        error(errCommandLine, -1, "Couldn't get text encoding");
        return 99;
    }

    // open PDF file
    if (ownerPassword[0] != '\001') {
        ownerPW = GooString(ownerPassword);
    }
    if (userPassword[0] != '\001') {
        userPW = GooString(userPassword);
    }

    if (fileName.cmp("-") == 0) {
        fileName = GooString("fd://0");
    }

    doc = PDFDocFactory().createPDFDoc(fileName, ownerPW, userPW);

    if (!doc->isOk()) {
        return 1;
    }

#ifdef ENFORCE_PERMISSIONS
    // check for copy permission
    if (!doc->okToCopy()) {
        error(errNotAllowed, -1, "Copying of text from this document is not allowed.");
        return 3;
    }
#endif

    // construct text file name
    if (argc == 3) {
        textFileName = std::make_unique<GooString>(argv[2]);
    } else if (fileName.cmp("fd://0") == 0) {
        error(errCommandLine, -1, "You have to provide an output filename when reading from stdin.");
        return 99;
    } else {
        const char *p = fileName.c_str() + fileName.getLength() - 4;
        if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
            textFileName = std::make_unique<GooString>(fileName.c_str(), fileName.getLength() - 4);
        } else {
            textFileName = fileName.copy();
        }
        textFileName->append(htmlMeta ? ".html" : ".txt");
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
        return 99;
    }

    // write HTML header
    if (htmlMeta) {
        if (!textFileName->cmp("-")) {
            f = stdout;
        } else {
            if (!(f = fopen(textFileName->c_str(), "wb"))) {
                error(errIO, -1, "Couldn't open text file '{0:t}'", textFileName.get());
                return 2;
            }
        }
        fputs(R"(<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">)", f);
        fputs("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", f);
        fputs("<head>\n", f);
        info = doc->getDocInfo();
        if (info.isDict()) {
            Object obj = info.getDict()->lookup("Title");
            if (obj.isString()) {
                printInfoString(f, info.getDict(), "Title", "<title>", "</title>\n", uMap);
            } else {
                fputs("<title></title>\n", f);
            }
            printInfoString(f, info.getDict(), "Subject", R"(<meta name="Subject" content=")", "\"/>\n", uMap);
            printInfoString(f, info.getDict(), "Keywords", R"(<meta name="Keywords" content=")", "\"/>\n", uMap);
            printInfoString(f, info.getDict(), "Author", R"(<meta name="Author" content=")", "\"/>\n", uMap);
            printInfoString(f, info.getDict(), "Creator", R"(<meta name="Creator" content=")", "\"/>\n", uMap);
            printInfoString(f, info.getDict(), "Producer", R"(<meta name="Producer" content=")", "\"/>\n", uMap);
            printInfoDate(f, info.getDict(), "CreationDate", R"(<meta name="CreationDate" content=")", "\"/>\n");
            printInfoDate(f, info.getDict(), "ModDate", R"(<meta name="ModDate" content=")", "\"/>\n");
        }
        fputs("</head>\n", f);
        fputs("<body>\n", f);
        if (!bbox) {
            fputs("<pre>\n", f);
            if (f != stdout) {
                fclose(f);
            }
        }
    }

    // write text file
    if (htmlMeta && bbox) { // htmlMeta && is superfluous but makes gcc happier
        TextOutputDev textOut(nullptr, physLayout, fixedPitch, rawOrder, htmlMeta, discardDiag);

        if (textOut.isOk()) {
            textOut.setTextEOL(textEOL);
            textOut.setMinColSpacing1(colspacing);
            if (noPageBreaks) {
                textOut.setTextPageBreaks(false);
            }
            if (bboxLayout) {
                printDocBBox(f, doc.get(), &textOut, firstPage, lastPage);
            } else {
                printWordBBox(f, doc.get(), &textOut, firstPage, lastPage);
            }
        }
        if (f != stdout) {
            fclose(f);
        }
    } else {

        if (tsvMode) {
            TextOutputDev textOut(nullptr, physLayout, fixedPitch, rawOrder, htmlMeta, discardDiag);
            if (!textFileName->cmp("-")) {
                f = stdout;
            } else {
                if (!(f = fopen(textFileName->c_str(), "wb"))) {
                    error(errIO, -1, "Couldn't open text file '{0:t}'", textFileName.get());
                    return 2;
                }
            }
            printTSVBBox(f, doc.get(), &textOut, firstPage, lastPage);
            if (f != stdout) {
                fclose(f);
            }
        } else {
            TextOutputDev textOut(textFileName->c_str(), physLayout, fixedPitch, rawOrder, htmlMeta, discardDiag);
            if (textOut.isOk()) {
                textOut.setTextEOL(textEOL);
                textOut.setMinColSpacing1(colspacing);
                if (noPageBreaks) {
                    textOut.setTextPageBreaks(false);
                }

                if ((w == 0) && (h == 0) && (x == 0) && (y == 0)) {
                    doc->displayPages(&textOut, firstPage, lastPage, resolution, resolution, 0, true, false, false);
                } else {

                    for (int page = firstPage; page <= lastPage; ++page) {
                        doc->displayPageSlice(&textOut, page, resolution, resolution, 0, true, false, false, x, y, w, h);
                    }
                }

            } else {
                return 2;
            }
        }
    }

    // write end of HTML file
    if (htmlMeta) {
        if (!textFileName->cmp("-")) {
            f = stdout;
        } else {
            if (!(f = fopen(textFileName->c_str(), "ab"))) {
                error(errIO, -1, "Couldn't open text file '{0:t}'", textFileName.get());
                return 2;
            }
        }
        if (!bbox) {
            fputs("</pre>\n", f);
        }
        fputs("</body>\n", f);
        fputs("</html>\n", f);
        if (f != stdout) {
            fclose(f);
        }
    }

    return 0;
}

static void printInfoString(FILE *f, Dict *infoDict, const char *key, const char *text1, const char *text2, const UnicodeMap *uMap)
{
    const GooString *s1;
    bool isUnicode;
    Unicode u;
    char buf[9];
    int i, n;

    Object obj = infoDict->lookup(key);
    if (obj.isString()) {
        fputs(text1, f);
        s1 = obj.getString();
        if ((s1->getChar(0) & 0xff) == 0xfe && (s1->getChar(1) & 0xff) == 0xff) {
            isUnicode = true;
            i = 2;
        } else {
            isUnicode = false;
            i = 0;
        }
        while (i < obj.getString()->getLength()) {
            if (isUnicode) {
                u = ((s1->getChar(i) & 0xff) << 8) | (s1->getChar(i + 1) & 0xff);
                i += 2;
            } else {
                u = pdfDocEncoding[s1->getChar(i) & 0xff];
                ++i;
            }
            n = uMap->mapUnicode(u, buf, sizeof(buf));
            buf[n] = '\0';
            const std::string myString = myXmlTokenReplace(buf);
            fputs(myString.c_str(), f);
        }
        fputs(text2, f);
    }
}

static void printInfoDate(FILE *f, Dict *infoDict, const char *key, const char *text1, const char *text2)
{
    int year, mon, day, hour, min, sec, tz_hour, tz_minute;
    char tz;

    Object obj = infoDict->lookup(key);
    if (obj.isString()) {
        const GooString *s = obj.getString();
        if (parseDateString(s, &year, &mon, &day, &hour, &min, &sec, &tz, &tz_hour, &tz_minute)) {
            fputs(text1, f);
            fprintf(f, "%04d-%02d-%02dT%02d:%02d:%02d", year, mon, day, hour, min, sec);
            if (tz_hour == 0 && tz_minute == 0) {
                fprintf(f, "Z");
            } else {
                fprintf(f, "%c%02d", tz, tz_hour);
                if (tz_minute) {
                    fprintf(f, ":%02d", tz_minute);
                }
            }
            fputs(text2, f);
        }
    }
}

static void printLine(FILE *f, const TextLine *line)
{
    double xMin, yMin, xMax, yMax;
    double lineXMin = 0, lineYMin = 0, lineXMax = 0, lineYMax = 0;
    const TextWord *word;
    std::stringstream wordXML;
    wordXML << std::fixed << std::setprecision(6);

    for (word = line->getWords(); word; word = word->getNext()) {
        word->getBBox(&xMin, &yMin, &xMax, &yMax);

        if (lineXMin == 0 || lineXMin > xMin) {
            lineXMin = xMin;
        }
        if (lineYMin == 0 || lineYMin > yMin) {
            lineYMin = yMin;
        }
        if (lineXMax < xMax) {
            lineXMax = xMax;
        }
        if (lineYMax < yMax) {
            lineYMax = yMax;
        }

        GooString *wordText = word->getText();
        const std::string myString = myXmlTokenReplace(wordText->c_str());
        wordXML << "          <word xMin=\"" << xMin << "\" yMin=\"" << yMin << "\" xMax=\"" << xMax << "\" yMax=\"" << yMax << "\">" << myString << "</word>\n";
        delete wordText;
    }
    fprintf(f, "        <line xMin=\"%f\" yMin=\"%f\" xMax=\"%f\" yMax=\"%f\">\n", lineXMin, lineYMin, lineXMax, lineYMax);
    fputs(wordXML.str().c_str(), f);
    fputs("        </line>\n", f);
}

void printDocBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last)
{
    double xMin, yMin, xMax, yMax;
    const TextFlow *flow;
    const TextBlock *blk;
    const TextLine *line;

    fprintf(f, "<doc>\n");
    for (int page = first; page <= last; ++page) {
        const double wid = useCropBox ? doc->getPageCropWidth(page) : doc->getPageMediaWidth(page);
        const double hgt = useCropBox ? doc->getPageCropHeight(page) : doc->getPageMediaHeight(page);
        fprintf(f, "  <page width=\"%f\" height=\"%f\">\n", wid, hgt);
        doc->displayPage(textOut, page, resolution, resolution, 0, !useCropBox, useCropBox, false);
        for (flow = textOut->getFlows(); flow; flow = flow->getNext()) {
            fprintf(f, "    <flow>\n");
            for (blk = flow->getBlocks(); blk; blk = blk->getNext()) {
                blk->getBBox(&xMin, &yMin, &xMax, &yMax);
                fprintf(f, "      <block xMin=\"%f\" yMin=\"%f\" xMax=\"%f\" yMax=\"%f\">\n", xMin, yMin, xMax, yMax);
                for (line = blk->getLines(); line; line = line->getNext()) {
                    printLine(f, line);
                }
                fprintf(f, "      </block>\n");
            }
            fprintf(f, "    </flow>\n");
        }
        fprintf(f, "  </page>\n");
    }
    fprintf(f, "</doc>\n");
}

void printTSVBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last)
{
    double xMin = 0, yMin = 0, xMax = 0, yMax = 0;
    const TextFlow *flow;
    const TextBlock *blk;
    const TextLine *line;
    const TextWord *word;
    int blockNum = 0;
    int lineNum = 0;
    int flowNum = 0;
    int wordNum = 0;
    const int pageLevel = 1;
    const int blockLevel = 3;
    const int lineLevel = 4;
    const int wordLevel = 5;
    const int metaConf = -1;
    const int wordConf = 100;

    fputs("level\tpage_num\tpar_num\tblock_num\tline_num\tword_num\tleft\ttop\twidth\theight\tconf\ttext\n", f);

    for (int page = first; page <= last; ++page) {
        const double wid = useCropBox ? doc->getPageCropWidth(page) : doc->getPageMediaWidth(page);
        const double hgt = useCropBox ? doc->getPageCropHeight(page) : doc->getPageMediaHeight(page);

        fprintf(f, "%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\t%d\t###PAGE###\n", pageLevel, page, flowNum, blockNum, lineNum, wordNum, xMin, yMin, wid, hgt, metaConf);
        doc->displayPage(textOut, page, resolution, resolution, 0, !useCropBox, useCropBox, false);

        for (flow = textOut->getFlows(); flow; flow = flow->getNext()) {
            // flow->getBBox(&xMin, &yMin, &xMax, &yMax);
            // fprintf(f, "%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\t\n", page,flowNum,blockNum,lineNum,wordNum,xMin,yMin,wid, hgt);

            for (blk = flow->getBlocks(); blk; blk = blk->getNext()) {
                blk->getBBox(&xMin, &yMin, &xMax, &yMax);
                fprintf(f, "%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\t%d\t###FLOW###\n", blockLevel, page, flowNum, blockNum, lineNum, wordNum, xMin, yMin, xMax - xMin, yMax - yMin, metaConf);

                for (line = blk->getLines(); line; line = line->getNext()) {

                    double lxMin = 1E+37, lyMin = 1E+37;
                    double lxMax = 0, lyMax = 0;
                    GooString *lineWordsBuffer = new GooString();

                    for (word = line->getWords(); word; word = word->getNext()) {
                        word->getBBox(&xMin, &yMin, &xMax, &yMax);
                        if (lxMin > xMin) {
                            lxMin = xMin;
                        }
                        if (lxMax < xMax) {
                            lxMax = xMax;
                        }
                        if (lyMin > yMin) {
                            lyMin = yMin;
                        }
                        if (lyMax < yMax) {
                            lyMax = yMax;
                        }

                        lineWordsBuffer->appendf("{0:d}\t{1:d}\t{2:d}\t{3:d}\t{4:d}\t{5:d}\t{6:.2f}\t{7:.2f}\t{8:.2f}\t{9:.2f}\t{10:d}\t{11:t}\n", wordLevel, page, flowNum, blockNum, lineNum, wordNum, xMin, yMin, xMax - xMin, yMax - yMin,
                                                 wordConf, word->getText());
                        wordNum++;
                    }

                    // Print Link Bounding Box info
                    fprintf(f, "%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\t%d\t###LINE###\n", lineLevel, page, flowNum, blockNum, lineNum, 0, lxMin, lyMin, lxMax - lxMin, lyMax - lyMin, metaConf);
                    fprintf(f, "%s", lineWordsBuffer->c_str());
                    delete lineWordsBuffer;
                    wordNum = 0;
                    lineNum++;
                }
                lineNum = 0;
                blockNum++;
            }
            blockNum = 0;
            flowNum++;
        }
        flowNum = 0;
    }
}

void printWordBBox(FILE *f, PDFDoc *doc, TextOutputDev *textOut, int first, int last)
{
    fprintf(f, "<doc>\n");
    for (int page = first; page <= last; ++page) {
        double wid = useCropBox ? doc->getPageCropWidth(page) : doc->getPageMediaWidth(page);
        double hgt = useCropBox ? doc->getPageCropHeight(page) : doc->getPageMediaHeight(page);
        fprintf(f, "  <page width=\"%f\" height=\"%f\">\n", wid, hgt);
        doc->displayPage(textOut, page, resolution, resolution, 0, !useCropBox, useCropBox, false);
        std::unique_ptr<TextWordList> wordlist = textOut->makeWordList();
        const int word_length = wordlist != nullptr ? wordlist->getLength() : 0;
        TextWord *word;
        double xMinA, yMinA, xMaxA, yMaxA;
        if (word_length == 0) {
            fprintf(stderr, "no word list\n");
        }

        for (int i = 0; i < word_length; ++i) {
            word = wordlist->get(i);
            word->getBBox(&xMinA, &yMinA, &xMaxA, &yMaxA);
            const std::string myString = myXmlTokenReplace(word->getText()->c_str());
            fprintf(f, "    <word xMin=\"%f\" yMin=\"%f\" xMax=\"%f\" yMax=\"%f\">%s</word>\n", xMinA, yMinA, xMaxA, yMaxA, myString.c_str());
        }
        fprintf(f, "  </page>\n");
    }
    fprintf(f, "</doc>\n");
}
