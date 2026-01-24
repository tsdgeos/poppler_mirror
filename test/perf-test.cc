/* Copyright Krzysztof Kowalczyk 2006-2007
   Copyright Hib Eris <hib@hiberis.nl> 2008, 2013
   Copyright 2018, 2020, 2022, 2025, 2026 Albert Astals Cid <aacid@kde.org> 2018
   Copyright 2019 Oliver Sander <oliver.sander@tu-dresden.de>
   Copyright 2020 Adam Reichold <adam.reichold@t-online.de>
   Copyright 2024 Vincent Lefevre <vincent@vinc17.net>
   Copyright 2024, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
   License: GPLv2 */
/*
  A tool to stress-test poppler rendering and measure rendering times for
  very simplistic performance measuring.

  TODO:
   * make it work with cairo output as well
   * print more info about document like e.g. enumerate images,
     streams, compression, encryption, password-protection. Each should have
     a command-line arguments to turn it on/off
   * never over-write file given as -out argument (optionally, provide -force
     option to force writing the -out file). It's way too easy too lose results
     of a previous run.
*/

#ifdef _MSC_VER
// this sucks but I don't know any other way
#    pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include <config.h>

// Define COPY_FILE if you want the file to be copied to a local disk first
// before it's tested. This is desired if a file is on a slow drive.
// Currently copying only works on Windows.
// Not enabled by default.
// #define COPY_FILE 1

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>

#include "Error.h"
#include "ErrorCodes.h"
#include "goo/GooString.h"
#include "goo/GooTimer.h"
#include "GlobalParams.h"
#include "splash/SplashBitmap.h"
#include "Object.h" /* must be included before SplashOutputDev.h because of sloppiness in SplashOutputDev.h */
#include "SplashOutputDev.h"
#include "TextOutputDev.h"
#include "PDFDoc.h"
#include "Link.h"

#define dimof(X) (sizeof(X) / sizeof((X)[0]))

constexpr int INVALID_PAGE_NO = -1;

class PdfEnginePoppler
{
public:
    PdfEnginePoppler();
    ~PdfEnginePoppler();

    PdfEnginePoppler(const PdfEnginePoppler &) = delete;
    PdfEnginePoppler &operator=(const PdfEnginePoppler &) = delete;

    const char *fileName() const { return _fileName; };

    void setFileName(const char *fileName)
    {
        assert(!_fileName);
        _fileName = strdup(fileName);
    }

    int pageCount() const { return _pageCount; }

    bool load(const char *fileName);
    SplashBitmap *renderBitmap(int pageNo, double zoomReal, int rotation);

    SplashOutputDev *outputDevice();

private:
    char *_fileName = nullptr;
    int _pageCount = INVALID_PAGE_NO;

    PDFDoc *_pdfDoc = nullptr;
    SplashOutputDev *_outputDev = nullptr;
};

struct StrList
{
    struct StrList *next;
    char *str;
};

/* List of all command-line arguments that are not switches.
   We assume those are:
     - names of PDF files
     - names of a file with a list of PDF files
     - names of directories with PDF files
*/
static StrList *gArgsListRoot = nullptr;

/* Names of all command-line switches we recognize */
constexpr const char *TIMINGS_ARG = "-timings";
constexpr const char *RESOLUTION_ARG = "-resolution";
constexpr const char *RECURSIVE_ARG = "-recursive";
constexpr const char *OUT_ARG = "-out";
constexpr const char *LOAD_ONLY_ARG = "-loadonly";
constexpr const char *PAGE_ARG = "-page";
constexpr const char *TEXT_ARG = "-text";

/* Should we record timings? True if -timings command-line argument was given. */
static bool gfTimings = false;

/* If true, we use render each page at resolution 'gResolutionX'/'gResolutionY'.
   If false, we render each page at its native resolution.
   True if -resolution NxM command-line argument was given. */
static bool gfForceResolution = false;
static int gResolutionX = 0;
static int gResolutionY = 0;
/* If NULL, we output the log info to stdout. If not NULL, should be a name
   of the file to which we output log info.
   Controlled by -out command-line argument. */
static char *gOutFileName = nullptr;
/* FILE * corresponding to gOutFileName or stdout if gOutFileName is NULL or
   was invalid name */
static FILE *gOutFile = nullptr;
/* FILE * corresponding to gOutFileName or stderr if gOutFileName is NULL or
   was invalid name */
static FILE *gErrFile = nullptr;

/* If True and a directory is given as a command-line argument, we'll process
   pdf files in sub-directories as well.
   Controlled by -recursive command-line argument */
static bool gfRecursive = false;

/* If true, we only dump the text, not render */
static bool gfTextOnly = false;

constexpr int PAGE_NO_NOT_GIVEN = -1;

/* If equals PAGE_NO_NOT_GIVEN, we're in default mode where we render all pages.
   If different, will only render this page */
static int gPageNo = PAGE_NO_NOT_GIVEN;
/* If true, will only load the file, not render any pages. Mostly for
   profiling load time */
static bool gfLoadOnly = false;

constexpr int PDF_FILE_DPI = 72;

static void memzero(void *data, size_t len)
{
    memset(data, 0, len);
}

static void *zmalloc(size_t len)
{
    void *data = malloc(len);
    if (data) {
        memzero(data, len);
    }
    return data;
}

/* Concatenate 4 strings. Any string can be NULL.
   Caller needs to free() memory. */
static char *str_cat4(const char *str1, const char *str2, const char *str3, const char *str4)
{
    char *str;
    char *tmp;
    size_t str1_len = 0;
    size_t str2_len = 0;
    size_t str3_len = 0;
    size_t str4_len = 0;

    if (str1) {
        str1_len = strlen(str1);
    }
    if (str2) {
        str2_len = strlen(str2);
    }
    if (str3) {
        str3_len = strlen(str3);
    }
    if (str4) {
        str4_len = strlen(str4);
    }

    str = (char *)zmalloc(str1_len + str2_len + str3_len + str4_len + 1);
    if (!str) {
        return nullptr;
    }

    tmp = str;
    if (str1) {
        memcpy(tmp, str1, str1_len);
        tmp += str1_len;
    }
    if (str2) {
        memcpy(tmp, str2, str2_len);
        tmp += str2_len;
    }
    if (str3) {
        memcpy(tmp, str3, str3_len);
        tmp += str3_len;
    }
    if (str4) {
        memcpy(tmp, str4, str1_len);
    }
    return str;
}

static char *str_dup(const char *str)
{
    return str_cat4(str, nullptr, nullptr, nullptr);
}

static bool str_eq(const char *str1, const char *str2)
{
    if (!str1 && !str2) {
        return true;
    }
    if (!str1 || !str2) {
        return false;
    }
    if (0 == strcmp(str1, str2)) {
        return true;
    }
    return false;
}

static bool str_ieq(const char *str1, const char *str2)
{
    if (!str1 && !str2) {
        return true;
    }
    if (!str1 || !str2) {
        return false;
    }
    if (0 == strcasecmp(str1, str2)) {
        return true;
    }
    return false;
}

static bool str_endswith(const char *txt, const char *end)
{
    size_t end_len;
    size_t txt_len;

    if (!txt || !end) {
        return false;
    }

    txt_len = strlen(txt);
    end_len = strlen(end);
    if (end_len > txt_len) {
        return false;
    }
    if (str_eq(txt + txt_len - end_len, end)) {
        return true;
    }
    return false;
}

static SplashColorMode gSplashColorMode = splashModeBGR8;

static SplashColor splashColRed;
static SplashColor splashColGreen;
static SplashColor splashColBlue;
static SplashColor splashColWhite;
static SplashColor splashColBlack;

#define SPLASH_COL_RED_PTR ((SplashColorPtr) & (splashColRed[0]))
#define SPLASH_COL_GREEN_PTR ((SplashColorPtr) & (splashColGreen[0]))
#define SPLASH_COL_BLUE_PTR ((SplashColorPtr) & (splashColBlue[0]))
#define SPLASH_COL_WHITE_PTR ((SplashColorPtr) & (splashColWhite[0]))
#define SPLASH_COL_BLACK_PTR ((SplashColorPtr) & (splashColBlack[0]))

static SplashColorPtr gBgColor = SPLASH_COL_WHITE_PTR;

static void splashColorSet(SplashColorPtr col, unsigned char red, unsigned char green, unsigned char blue)
{
    switch (gSplashColorMode) {
    case splashModeBGR8:
        col[0] = blue;
        col[1] = green;
        col[2] = red;
        break;
    case splashModeRGB8:
        col[0] = red;
        col[1] = green;
        col[2] = blue;
        break;
    default:
        assert(0);
        break;
    }
}

static void SplashColorsInit()
{
    splashColorSet(SPLASH_COL_RED_PTR, 0xff, 0, 0);
    splashColorSet(SPLASH_COL_GREEN_PTR, 0, 0xff, 0);
    splashColorSet(SPLASH_COL_BLUE_PTR, 0, 0, 0xff);
    splashColorSet(SPLASH_COL_BLACK_PTR, 0, 0, 0);
    splashColorSet(SPLASH_COL_WHITE_PTR, 0xff, 0xff, 0xff);
}

PdfEnginePoppler::PdfEnginePoppler() = default;

PdfEnginePoppler::~PdfEnginePoppler()
{
    free(_fileName);
    delete _outputDev;
    delete _pdfDoc;
}

bool PdfEnginePoppler::load(const char *fileName)
{
    setFileName(fileName);

    _pdfDoc = new PDFDoc(std::make_unique<GooString>(fileName));
    if (!_pdfDoc->isOk()) {
        return false;
    }
    _pageCount = _pdfDoc->getNumPages();
    return true;
}

SplashOutputDev *PdfEnginePoppler::outputDevice()
{
    if (!_outputDev) {
        bool bitmapTopDown = true;
        _outputDev = new SplashOutputDev(gSplashColorMode, 4, gBgColor, bitmapTopDown);
        if (_outputDev) {
            _outputDev->startDoc(_pdfDoc);
        }
    }
    return _outputDev;
}

SplashBitmap *PdfEnginePoppler::renderBitmap(int pageNo, double zoomReal, int rotation)
{
    assert(outputDevice());
    if (!outputDevice()) {
        return nullptr;
    }

    double hDPI = (double)PDF_FILE_DPI * zoomReal * 0.01;
    double vDPI = (double)PDF_FILE_DPI * zoomReal * 0.01;
    bool useMediaBox = false;
    bool crop = true;
    bool doLinks = true;
    _pdfDoc->displayPage(_outputDev, pageNo, hDPI, vDPI, rotation, useMediaBox, crop, doLinks, nullptr, nullptr);

    SplashBitmap *bmp = _outputDev->takeBitmap();
    return bmp;
}

static int StrList_Len(StrList **root)
{
    int len = 0;
    StrList *cur;
    assert(root);
    if (!root) {
        return 0;
    }
    cur = *root;
    while (cur) {
        ++len;
        cur = cur->next;
    }
    return len;
}

static int StrList_InsertAndOwn(StrList **root, char *txt)
{
    StrList *el;
    assert(root && txt);
    if (!root || !txt) {
        return false;
    }

    el = (StrList *)malloc(sizeof(StrList));
    if (!el) {
        return false;
    }
    el->str = txt;
    el->next = *root;
    *root = el;
    return true;
}

static int StrList_Insert(StrList **root, char *txt)
{
    char *txtDup;

    assert(root && txt);
    if (!root || !txt) {
        return false;
    }
    txtDup = str_dup(txt);
    if (!txtDup) {
        return false;
    }

    if (!StrList_InsertAndOwn(root, txtDup)) {
        free((void *)txtDup);
        return false;
    }
    return true;
}

static void StrList_FreeElement(StrList *el)
{
    if (!el) {
        return;
    }
    free((void *)el->str);
    free((void *)el);
}

static void StrList_Destroy(StrList **root)
{
    StrList *cur;
    StrList *next;

    if (!root) {
        return;
    }
    cur = *root;
    while (cur) {
        next = cur->next;
        StrList_FreeElement(cur);
        cur = next;
    }
    *root = nullptr;
}

static void my_error(ErrorCategory /*category*/, Goffset /*pos*/, const char * /*msg*/)
{
#if 0
    char        buf[4096], *p = buf;

    // NB: this can be called before the globalParams object is created
    if (globalParams && globalParams->getErrQuiet()) {
        return;
    }

    if (pos >= 0) {
      p += _snprintf(p, sizeof(buf)-1, "Error (%lld): ", (long long)pos);
        *p   = '\0';
        OutputDebugString(p);
    } else {
        OutputDebugString("Error: ");
    }

    p = buf;
    p += vsnprintf(p, sizeof(buf) - 1, msg, args);
    while ( p > buf  &&  isspace(p[-1]) )
            *--p = '\0';
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString(buf);

    if (pos >= 0) {
        p += _snprintf(p, sizeof(buf)-1, "Error (%lld): ", (long long)pos);
        *p   = '\0';
        OutputDebugString(buf);
        if (gErrFile)
            fprintf(gErrFile, buf);
    } else {
        OutputDebugString("Error: ");
        if (gErrFile)
            fprintf(gErrFile, "Error: ");
    }
#endif
#if 0
    p = buf;
    va_start(args, msg);
    p += vsnprintf(p, sizeof(buf) - 3, msg, args);
    while ( p > buf  &&  isspace(p[-1]) )
            *--p = '\0';
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString(buf);
    if (gErrFile)
        fprintf(gErrFile, buf);
    va_end(args);
#endif
}

static void LogInfo(const char *fmt, ...) GCC_PRINTF_FORMAT(1, 2);

static void LogInfo(const char *fmt, ...)
{
    va_list args;
    char buf[4096], *p = buf;

    p = buf;
    va_start(args, fmt);
    p += vsnprintf(p, sizeof(buf) - 1, fmt, args);
    *p = '\0';
    fprintf(gOutFile, "%s", buf);
    va_end(args);
    fflush(gOutFile);
}

static void PrintUsageAndExit(int argc, char **argv)
{
    printf("Usage: pdftest [-preview|-slowpreview] [-loadonly] [-timings] [-text] [-resolution NxM] [-recursive] [-page N] [-out out.txt] pdf-files-to-process\n");
    for (int i = 0; i < argc; i++) {
        printf("i=%d, '%s'\n", i, argv[i]);
    }
    exit(0);
}

static void RenderPdfAsText(const char *fileName)
{
    PDFDoc *pdfDoc = nullptr;
    int pageCount;
    double timeInMs;

    assert(fileName);
    if (!fileName) {
        return;
    }

    LogInfo("started: %s\n", fileName);

    auto *textOut = new TextOutputDev(nullptr, true, 0, false, false);
    if (!textOut->isOk()) {
        delete textOut;
        return;
    }

    GooTimer msTimer;
    pdfDoc = new PDFDoc(std::make_unique<GooString>(fileName));
    if (!pdfDoc->isOk()) {
        error(errIO, -1, "RenderPdfFile(): failed to open PDF file {0:s}", fileName);
        goto Exit;
    }

    msTimer.stop();
    timeInMs = msTimer.getElapsed();
    LogInfo("load: %.2f ms\n", timeInMs);

    pageCount = pdfDoc->getNumPages();
    LogInfo("page count: %d\n", pageCount);

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage)) {
            continue;
        }

        msTimer.start();
        int rotate = 0;
        bool useMediaBox = false;
        bool crop = true;
        bool doLinks = false;
        pdfDoc->displayPage(textOut, curPage, 72, 72, rotate, useMediaBox, crop, doLinks);
        GooString txt = textOut->getText(PDFRectangle { 0.0, 0.0, 10000.0, 10000.0 });
        msTimer.stop();
        timeInMs = msTimer.getElapsed();
        if (gfTimings) {
            LogInfo("page %d: %.2f ms\n", curPage, timeInMs);
        }
        printf("%s\n", txt.c_str());
    }

Exit:
    LogInfo("finished: %s\n", fileName);
    delete textOut;
    delete pdfDoc;
}

#ifdef _MSC_VER
#    define POPPLER_TMP_NAME "c:\\poppler_tmp.pdf"
#else
#    define POPPLER_TMP_NAME "/tmp/poppler_tmp.pdf"
#endif

static void RenderPdf(const char *fileName)
{
    const char *fileNameSplash = nullptr;
    PdfEnginePoppler *engineSplash = nullptr;
    int pageCount;
    double timeInMs;

#ifdef COPY_FILE
    // TODO: fails if file already exists and has read-only attribute
    CopyFile(fileName, POPPLER_TMP_NAME, false);
    fileNameSplash = POPPLER_TMP_NAME;
#else
    fileNameSplash = fileName;
#endif
    LogInfo("started: %s\n", fileName);

    engineSplash = new PdfEnginePoppler();

    GooTimer msTimer;
    if (!engineSplash->load(fileNameSplash)) {
        LogInfo("failed to load splash\n");
        goto Error;
    }
    msTimer.stop();
    timeInMs = msTimer.getElapsed();
    LogInfo("load splash: %.2f ms\n", timeInMs);
    pageCount = engineSplash->pageCount();

    LogInfo("page count: %d\n", pageCount);
    if (gfLoadOnly) {
        goto Error;
    }

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage)) {
            continue;
        }

        SplashBitmap *bmpSplash = nullptr;

        GooTimer msRenderTimer;
        bmpSplash = engineSplash->renderBitmap(curPage, 100.0, 0);
        msRenderTimer.stop();
        timeInMs = msRenderTimer.getElapsed();
        if (gfTimings) {
            if (!bmpSplash) {
                LogInfo("page splash %d: failed to render\n", curPage);
            } else {
                LogInfo("page splash %d (%dx%d): %.2f ms\n", curPage, bmpSplash->getWidth(), bmpSplash->getHeight(), timeInMs);
            }
        }

        delete bmpSplash;
    }
Error:
    delete engineSplash;
    LogInfo("finished: %s\n", fileName);
}

static void RenderFile(const char *fileName)
{
    if (gfTextOnly) {
        RenderPdfAsText(fileName);
        return;
    }

    RenderPdf(fileName);
}

static bool ParseInteger(const char *start, const char *end, int *intOut)
{
    char numBuf[16];
    int digitsCount;
    const char *tmp;

    assert(start && end && intOut);
    assert(end >= start);
    if (!start || !end || !intOut || (start > end)) {
        return false;
    }

    digitsCount = 0;
    tmp = start;
    while (tmp <= end) {
        if (isspace(*tmp)) {
            /* do nothing, we allow whitespace */
        } else if (!isdigit(*tmp)) {
            return false;
        }
        numBuf[digitsCount] = *tmp;
        ++digitsCount;
        if (digitsCount == dimof(numBuf) - 3) { /* -3 to be safe */
            return false;
        }
        ++tmp;
    }
    if (0 == digitsCount) {
        return false;
    }
    numBuf[digitsCount] = 0;
    *intOut = atoi(numBuf);
    return true;
}

/* Given 'resolutionString' in format NxM (e.g. "100x200"), parse the string and put N
   into 'resolutionXOut' and M into 'resolutionYOut'.
   Return false if there was an error (e.g. string is not in the right format */
static bool ParseResolutionString(const char *resolutionString, int *resolutionXOut, int *resolutionYOut)
{
    const char *posOfX;

    assert(resolutionString);
    assert(resolutionXOut);
    assert(resolutionYOut);
    if (!resolutionString || !resolutionXOut || !resolutionYOut) {
        return false;
    }
    *resolutionXOut = 0;
    *resolutionYOut = 0;
    posOfX = strchr(resolutionString, 'X');
    if (!posOfX) {
        posOfX = strchr(resolutionString, 'x');
    }
    if (!posOfX) {
        return false;
    }
    if (posOfX == resolutionString) {
        return false;
    }
    if (!ParseInteger(resolutionString, posOfX - 1, resolutionXOut)) {
        return false;
    }
    if (!ParseInteger(posOfX + 1, resolutionString + strlen(resolutionString) - 1, resolutionYOut)) {
        return false;
    }
    return true;
}

static void ParseCommandLine(int argc, char **argv)
{
    char *arg;

    if (argc < 2) {
        PrintUsageAndExit(argc, argv);
    }

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        assert(arg);
        if ('-' == arg[0]) {
            if (str_ieq(arg, TIMINGS_ARG)) {
                gfTimings = true;
            } else if (str_ieq(arg, RESOLUTION_ARG)) {
                ++i;
                if (i == argc) {
                    PrintUsageAndExit(argc, argv); /* expect a file name after that */
                }
                if (!ParseResolutionString(argv[i], &gResolutionX, &gResolutionY)) {
                    PrintUsageAndExit(argc, argv);
                }
                gfForceResolution = true;
            } else if (str_ieq(arg, RECURSIVE_ARG)) {
                gfRecursive = true;
            } else if (str_ieq(arg, OUT_ARG)) {
                /* expect a file name after that */
                ++i;
                if (i == argc) {
                    PrintUsageAndExit(argc, argv);
                }
                gOutFileName = str_dup(argv[i]);
            } else if (str_ieq(arg, TEXT_ARG)) {
                gfTextOnly = true;
            } else if (str_ieq(arg, LOAD_ONLY_ARG)) {
                gfLoadOnly = true;
            } else if (str_ieq(arg, PAGE_ARG)) {
                /* expect an integer after that */
                ++i;
                if (i == argc) {
                    PrintUsageAndExit(argc, argv);
                }
                gPageNo = atoi(argv[i]);
                if (gPageNo < 1) {
                    PrintUsageAndExit(argc, argv);
                }
            } else {
                /* unknown option */
                PrintUsageAndExit(argc, argv);
            }
        } else {
            /* we assume that this is not an option hence it must be
               a name of PDF/directory/file with PDF names */
            StrList_Insert(&gArgsListRoot, arg);
        }
    }
}

static bool IsPdfFileName(char *path)
{
    return str_endswith(path, ".pdf");
}

/* Render 'cmdLineArg', which can be:
   - name of PDF file
*/
static void RenderCmdLineArg(char *cmdLineArg)
{
    assert(cmdLineArg);
    if (!cmdLineArg) {
        return;
    }
    if (IsPdfFileName(cmdLineArg)) {
        RenderFile(cmdLineArg);
    } else {
        error(errCommandLine, -1, "unexpected argument '{0:s}'", cmdLineArg);
    }
}

int main(int argc, char **argv)
{
    setErrorCallback(my_error);
    ParseCommandLine(argc, argv);
    if (0 == StrList_Len(&gArgsListRoot)) {
        PrintUsageAndExit(argc, argv);
    }
    assert(gArgsListRoot);

    SplashColorsInit();
    globalParams = std::make_unique<GlobalParams>();
    if (!globalParams) {
        return 1;
    }
    globalParams->setErrQuiet(false);

    FILE *outFile = nullptr;
    if (gOutFileName) {
        outFile = fopen(gOutFileName, "wb");
        if (!outFile) {
            printf("failed to open -out file %s\n", gOutFileName);
            return 1;
        }
        gOutFile = outFile;
    } else {
        gOutFile = stdout;
    }

    if (gOutFileName) {
        gErrFile = outFile;
    } else {
        gErrFile = stderr;
    }

    StrList *curr = gArgsListRoot;
    while (curr) {
        RenderCmdLineArg(curr->str);
        curr = curr->next;
    }
    if (outFile) {
        fclose(outFile);
    }
    StrList_Destroy(&gArgsListRoot);
    free(gOutFileName);
    return 0;
}
