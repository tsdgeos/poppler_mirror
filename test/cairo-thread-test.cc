//========================================================================
//
// cairo-thread-test.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2022 Adrian Johnson <ajohnson@redneon.com>
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <condition_variable>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "goo/GooString.h"
#include "CairoOutputDev.h"
#include "CairoFontEngine.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "../utils/numberofcharacters.h"

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include <cairo-svg.h>

static const int renderResolution = 150;

enum OutputType
{
    png,
    pdf,
    ps,
    svg
};

// Lazy creation of PDFDoc
class Document
{
public:
    explicit Document(const std::string &filenameA) : filename(filenameA) { std::call_once(ftLibOnceFlag, FT_Init_FreeType, &ftLib); }

    std::shared_ptr<PDFDoc> getDoc()
    {
        std::call_once(docOnceFlag, &Document::openDocument, this);
        return doc;
    }

    const std::string &getFilename() { return filename; }
    CairoFontEngine *getFontEngine() { return fontEngine.get(); }

private:
    void openDocument()
    {
        doc = PDFDocFactory().createPDFDoc(GooString(filename));
        if (!doc->isOk()) {
            fprintf(stderr, "Error opening PDF file %s\n", filename.c_str());
            exit(1);
        }
        fontEngine = std::make_unique<CairoFontEngine>(ftLib);
    }

    std::string filename;
    std::shared_ptr<PDFDoc> doc;
    std::once_flag docOnceFlag;
    std::unique_ptr<CairoFontEngine> fontEngine;

    static FT_Library ftLib;
    static std::once_flag ftLibOnceFlag;
};

FT_Library Document::ftLib;
std::once_flag Document::ftLibOnceFlag;

struct Job
{
    Job(OutputType typeA, const std::shared_ptr<Document> &documentA, int pageNumA, const std::string &outputFileA) : type(typeA), document(documentA), pageNum(pageNumA), outputFile(outputFileA) { }
    OutputType type;
    std::shared_ptr<Document> document;
    int pageNum;
    std::string outputFile;
};

class JobQueue
{
public:
    JobQueue() : shutdownFlag(false) { }

    void pushJob(std::unique_ptr<Job> &job)
    {
        std::scoped_lock lock { mutex };
        queue.push_back(std::move(job));
        condition.notify_one();
    }

    // Wait for job. If shutdownFlag true, will return null if queue empty.
    std::unique_ptr<Job> popJob()
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return !queue.empty() || shutdownFlag; });
        std::unique_ptr<Job> job;
        if (!queue.empty()) {
            job = std::move(queue.front());
            queue.pop_front();
        } else {
            condition.notify_all(); // notify waitUntilEmpty()
        }
        return job;
    }

    // When called, popJob() will not block on an empty queue instead returning nullptr
    void shutdown()
    {
        shutdownFlag = true;
        condition.notify_all();
    }

    // wait until queue is empty
    void waitUntilEmpty()
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return queue.empty(); });
    }

private:
    std::deque<std::unique_ptr<Job>> queue;
    std::mutex mutex;
    std::condition_variable condition;
    bool shutdownFlag;
};

static cairo_status_t writeStream(void *closure, const unsigned char *data, unsigned int length)
{
    FILE *file = (FILE *)closure;

    if (fwrite(data, length, 1, file) == 1) {
        return CAIRO_STATUS_SUCCESS;
    } else {
        return CAIRO_STATUS_WRITE_ERROR;
    }
}

// PDF/PS/SVG output
static void renderDocument(const Job &job)
{
    FILE *f = openFile(job.outputFile.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "Error opening output file %s\n", job.outputFile.c_str());
        exit(1);
    }

    cairo_surface_t *surface = nullptr;

    switch (job.type) {
    case OutputType::pdf:
        surface = cairo_pdf_surface_create_for_stream(writeStream, f, 1, 1);
        break;
    case OutputType::ps:
        surface = cairo_ps_surface_create_for_stream(writeStream, f, 1, 1);
        break;
    case OutputType::svg:
        surface = cairo_svg_surface_create_for_stream(writeStream, f, 1, 1);
        break;
    case OutputType::png:
        break;
    }

    cairo_surface_set_fallback_resolution(surface, renderResolution, renderResolution);

    std::unique_ptr<CairoOutputDev> cairoOut = std::make_unique<CairoOutputDev>();

    cairoOut->startDoc(job.document->getDoc().get(), job.document->getFontEngine());

    cairo_status_t status;
    for (int pageNum = 1; pageNum <= job.document->getDoc()->getNumPages(); pageNum++) {
        double width = job.document->getDoc()->getPageMediaWidth(pageNum);
        double height = job.document->getDoc()->getPageMediaHeight(pageNum);

        if (job.type == OutputType::pdf) {
            cairo_pdf_surface_set_size(surface, width, height);
        } else if (job.type == OutputType::ps) {
            cairo_ps_surface_set_size(surface, width, height);
        }

        cairo_t *cr = cairo_create(surface);

        cairoOut->setCairo(cr);
        cairoOut->setPrinting(true);

        cairo_save(cr);
        job.document->getDoc()->displayPageSlice(cairoOut.get(), pageNum, 72.0, 72.0, 0, /* rotate */
                                                 true, /* useMediaBox */
                                                 false, /* Crop */
                                                 true /*printing*/, -1, -1, -1, -1);
        cairo_restore(cr);
        cairoOut->setCairo(nullptr);

        status = cairo_status(cr);
        if (status) {
            fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
        }
        cairo_destroy(cr);
    }

    cairo_surface_finish(surface);
    status = cairo_surface_status(surface);
    if (status) {
        fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
    }
    cairo_surface_destroy(surface);
    fclose(f);
}

// PNG page output
static void renderPage(const Job &job)
{
    double width = job.document->getDoc()->getPageMediaWidth(job.pageNum);
    double height = job.document->getDoc()->getPageMediaHeight(job.pageNum);

    // convert from points to pixels
    width *= renderResolution / 72.0;
    height *= renderResolution / 72.0;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, static_cast<int>(ceil(width)), static_cast<int>(ceil(height)));

    std::unique_ptr<CairoOutputDev> cairoOut = std::make_unique<CairoOutputDev>();

    cairoOut->startDoc(job.document->getDoc().get(), job.document->getFontEngine());
    cairo_t *cr = cairo_create(surface);
    cairo_status_t status;

    cairoOut->setCairo(cr);
    cairoOut->setPrinting(false);

    cairo_save(cr);
    cairo_scale(cr, renderResolution / 72.0, renderResolution / 72.0);

    job.document->getDoc()->displayPageSlice(cairoOut.get(), job.pageNum, 72.0, 72.0, 0, /* rotate */
                                             true, /* useMediaBox */
                                             false, /* Crop */
                                             false /*printing */, -1, -1, -1, -1);
    cairo_restore(cr);

    cairoOut->setCairo(nullptr);

    // Blend onto white page
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_restore(cr);

    status = cairo_status(cr);
    if (status) {
        fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
    }
    cairo_destroy(cr);

    FILE *f = openFile(job.outputFile.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "Error opening output file %s\n", job.outputFile.c_str());
        exit(1);
    }
    cairo_surface_write_to_png_stream(surface, writeStream, f);
    fclose(f);

    cairo_surface_finish(surface);
    status = cairo_surface_status(surface);
    if (status) {
        fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
    }
    cairo_surface_destroy(surface);
}

static void runThread(const std::shared_ptr<JobQueue> &jobQueue)
{
    while (true) {
        std::unique_ptr<Job> job = jobQueue->popJob();
        if (!job) {
            break;
        }
        switch (job->type) {
        case OutputType::png:
            renderPage(*job);
            break;
        case OutputType::pdf:
        case OutputType::ps:
        case OutputType::svg:
            renderDocument(*job);
            break;
        }
    }
}

static void printUsage()
{
    int default_threads = std::max(1, (int)std::thread::hardware_concurrency());
    printf("cairo-thread-test [-j jobs] [-p priority] [<output option> <files>...]...\n");
    printf(" -j num       number of concurrent threads (default %d)\n", default_threads);
    printf(" -p <priority>  priority is one of:\n");
    printf("     page        one page at a time will be queued from each document in round-robin fashion (default).\n");
    printf("     document    all pages in the first document will be queued before processing to the next document.\n");
    printf("  Note: documents with vector output will be handled in one job. They can not be parallelized.\n");
    printf(" <output option> is one of -png, -pdf, -ps, -svg\n");
    printf("  The output option will apply to all documents after the option until a different option is specified\n");
}

// Parse -j and -p options. These must appear before any other arguments
static bool getThreadsAndPriority(int &argc, char **&argv, int &numThreads, bool &documentPriority)
{
    numThreads = std::max(1, (int)std::thread::hardware_concurrency());
    documentPriority = false;

    while (argc > 0) {
        std::string arg(*argv);
        if (arg == "-j") {
            argc--;
            argv++;
            if (argc == 0) {
                return false;
            }
            numThreads = atoi(*argv);
            if (numThreads == 0) {
                return false;
            }
            argc--;
            argv++;
        } else if (arg == "-p") {
            argc--;
            argv++;
            if (argc == 0) {
                return false;
            }
            arg = *argv;
            if (arg == "document") {
                documentPriority = true;
            } else if (arg == "page") {
                documentPriority = false;

            } else {
                return false;
            }
            argc--;
            argv++;
        } else {
            // file or output option
            break;
        }
    }
    return true;
}

// eg "-png doc1.pdf -ps doc2.pdf doc3.pdf -png doc4.pdf"
static bool getOutputTypeAndDocument(int &argc, char **&argv, OutputType &outputType, std::string &filename)
{
    static OutputType type;
    static bool typeInitialized = false;

    while (argc > 0) {
        std::string arg(*argv);
        if (arg == "-png") {
            argc--;
            argv++;
            type = OutputType::png;
            typeInitialized = true;
        } else if (arg == "-pdf") {
            argc--;
            argv++;
            type = OutputType::pdf;
            typeInitialized = true;
        } else if (arg == "-ps") {
            argc--;
            argv++;
            type = OutputType::ps;
            typeInitialized = true;
        } else if (arg == "-svg") {
            argc--;
            argv++;
            type = OutputType::svg;
            typeInitialized = true;
        } else {
            // filename
            if (!typeInitialized) {
                return false;
            }
            outputType = type;
            filename = *argv;
            argc--;
            argv++;
            return true;
        }
    }
    return false;
}

// "../a/b/foo.pdf" => "foo"
static std::string getBaseName(const std::string &filename)
{
    // strip everything up to last '/'
    size_t slash_pos = filename.find_last_of('/');
    std::string basename;
    if (slash_pos != std::string::npos) {
        basename = filename.substr(slash_pos + 1, std::string::npos);
    } else {
        basename = filename;
    }

    // remove .pdf extension
    size_t dot_pos = basename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        if (basename.compare(dot_pos, std::string::npos, ".pdf") == 0) {
            basename.erase(dot_pos);
        }
    }
    return basename;
}

// Represents an input file on the command line
struct InputFile
{
    InputFile(const std::string &filename, OutputType typeA) : type(typeA)
    {
        document = std::make_shared<Document>(filename);
        basename = getBaseName(filename);
        currentPage = 0;
        numPages = 0; // filled in later
        numDigits = 0; // filled in later
    }
    std::shared_ptr<Document> document;
    OutputType type;

    // Used when creating jobs for this InputFile
    int currentPage;
    std::string basename;
    int numPages;
    int numDigits;
};

// eg "basename.out-123.png" or "basename.out.pdf"
static std::string getOutputName(const InputFile &input)
{
    std::string output;
    char buf[30];
    switch (input.type) {
    case OutputType::png:
        std::snprintf(buf, sizeof(buf), ".out-%0*d.png", input.numDigits, input.currentPage);
        output = input.basename + buf;
        break;
    case OutputType::pdf:
        output = input.basename + ".out.pdf";
        break;
    case OutputType::ps:
        output = input.basename + ".out.ps";
        break;
    case OutputType::svg:
        output = input.basename + ".out.svg";
        break;
    }
    return output;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printUsage();
        exit(1);
    }

    // skip program name
    argc--;
    argv++;

    int numThreads;
    bool documentPriority;
    if (!getThreadsAndPriority(argc, argv, numThreads, documentPriority)) {
        printUsage();
        exit(1);
    }

    globalParams = std::make_unique<GlobalParams>();

    std::shared_ptr<JobQueue> jobQueue = std::make_shared<JobQueue>();
    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(runThread, jobQueue);
    }

    std::vector<InputFile> inputFiles;

    while (argc > 0) {
        std::string filename;
        OutputType type;
        if (!getOutputTypeAndDocument(argc, argv, type, filename)) {
            printUsage();
            exit(1);
        }
        InputFile input(filename, type);
        inputFiles.push_back(input);
    }

    if (documentPriority) {
        while (true) {
            bool jobAdded = false;
            for (auto &input : inputFiles) {
                if (input.numPages == 0) {
                    // first time seen
                    if (input.type == OutputType::png) {
                        input.numPages = input.document->getDoc()->getNumPages();
                        input.numDigits = numberOfCharacters(input.numPages);
                    } else {
                        input.numPages = 1; // Use 1 for vector output as there is only one output file
                    }
                }
                if (input.currentPage < input.numPages) {
                    input.currentPage++;
                    std::string output = getOutputName(input);
                    std::unique_ptr<Job> job = std::make_unique<Job>(input.type, input.document, input.currentPage, output);
                    jobQueue->pushJob(job);
                    jobAdded = true;
                }
            }
            if (!jobAdded) {
                break;
            }
        }
    } else {
        for (auto &input : inputFiles) {
            if (input.type == OutputType::png) {
                input.numPages = input.document->getDoc()->getNumPages();
                input.numDigits = numberOfCharacters(input.numPages);
                for (int i = 1; i <= input.numPages; i++) {
                    input.currentPage = i;
                    std::string output = getOutputName(input);
                    std::unique_ptr<Job> job = std::make_unique<Job>(input.type, input.document, input.currentPage, output);
                    jobQueue->pushJob(job);
                }
            } else {
                std::string output = getOutputName(input);
                std::unique_ptr<Job> job = std::make_unique<Job>(input.type, input.document, 1, output);
                jobQueue->pushJob(job);
            }
        }
    }

    jobQueue->shutdown();
    jobQueue->waitUntilEmpty();

    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }

    return 0;
}
