/* poppler-document.cc: glib wrapper for poppler
 * Copyright (C) 2005, Red Hat, Inc.
 *
 * Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
 * Copyright (C) 2018, 2019, 2021, 2022 Marek Kasik <mkasik@redhat.com>
 * Copyright (C) 2019 Masamichi Hosoda <trueroad@trueroad.jp>
 * Copyright (C) 2019, 2021, 2024 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2020, 2022 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2021 André Guerreiro <aguerreiro1985@gmail.com>
 * Copyright (C) 2024 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include <cstring>

#include <glib.h>

#ifndef G_OS_WIN32
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

#ifndef __GI_SCANNER__
#    include <memory>

#    include <goo/gfile.h>
#    include <splash/SplashBitmap.h>
#    include <CachedFile.h>
#    include <DateInfo.h>
#    include <FILECacheLoader.h>
#    include <GlobalParams.h>
#    include <PDFDoc.h>
#    include <SignatureInfo.h>
#    include <Outline.h>
#    include <ErrorCodes.h>
#    include <UnicodeMap.h>
#    include <GfxState.h>
#    include <SplashOutputDev.h>
#    include <Stream.h>
#    include <FontInfo.h>
#    include <PDFDocEncoding.h>
#    include <OptionalContent.h>
#    include <ViewerPreferences.h>
#    include "UTF.h"
#endif

#include "poppler.h"
#include "poppler-private.h"
#include "poppler-enums.h"
#include "poppler-input-stream.h"
#include "poppler-cached-file-loader.h"

#ifdef G_OS_WIN32
#    include <stringapiset.h>
#endif

/**
 * SECTION:poppler-document
 * @short_description: Information about a document
 * @title: PopplerDocument
 *
 * The #PopplerDocument is an object used to refer to a main document.
 */

enum
{
    PROP_0,
    PROP_TITLE,
    PROP_FORMAT,
    PROP_FORMAT_MAJOR,
    PROP_FORMAT_MINOR,
    PROP_SUBTYPE,
    PROP_SUBTYPE_STRING,
    PROP_SUBTYPE_PART,
    PROP_SUBTYPE_CONF,
    PROP_AUTHOR,
    PROP_SUBJECT,
    PROP_KEYWORDS,
    PROP_CREATOR,
    PROP_PRODUCER,
    PROP_CREATION_DATE,
    PROP_MOD_DATE,
    PROP_LINEARIZED,
    PROP_PAGE_LAYOUT,
    PROP_PAGE_MODE,
    PROP_VIEWER_PREFERENCES,
    PROP_PERMISSIONS,
    PROP_METADATA,
    PROP_PRINT_SCALING,
    PROP_PRINT_DUPLEX,
    PROP_PRINT_N_COPIES,
    PROP_CREATION_DATETIME,
    PROP_MOD_DATETIME
};

static void poppler_document_layers_free(PopplerDocument *document);

typedef struct _PopplerDocumentClass PopplerDocumentClass;
struct _PopplerDocumentClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE(PopplerDocument, poppler_document, G_TYPE_OBJECT)

static PopplerDocument *_poppler_document_new_from_pdfdoc(std::unique_ptr<GlobalParamsIniter> &&initer, PDFDoc *newDoc, GError **error)
{
    PopplerDocument *document;

    if (!newDoc->isOk()) {
        int fopen_errno;
        switch (newDoc->getErrorCode()) {
        case errOpenFile:
            // If there was an error opening the file, count it as a G_FILE_ERROR
            // and set the GError parameters accordingly. (this assumes that the
            // only way to get an errOpenFile error is if newDoc was created using
            // a filename and thus fopen was called, which right now is true.
            fopen_errno = newDoc->getFopenErrno();
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(fopen_errno), "%s", g_strerror(fopen_errno));
            break;
        case errBadCatalog:
            g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_BAD_CATALOG, "Failed to read the document catalog");
            break;
        case errDamaged:
            g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_DAMAGED, "PDF document is damaged");
            break;
        case errEncrypted:
            g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_ENCRYPTED, "Document is encrypted");
            break;
        default:
            g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Failed to load document");
        }

        delete newDoc;
        return nullptr;
    }

    document = (PopplerDocument *)g_object_new(POPPLER_TYPE_DOCUMENT, nullptr);
    document->initer = std::move(initer);
    document->doc = newDoc;

    document->output_dev = new CairoOutputDev();
    document->output_dev->startDoc(document->doc);

    return document;
}

static std::optional<GooString> poppler_password_to_latin1(const gchar *password)
{
    gchar *password_latin;

    if (!password) {
        return {};
    }

    password_latin = g_convert(password, -1, "ISO-8859-1", "UTF-8", nullptr, nullptr, nullptr);
    std::optional<GooString> password_g = GooString(password_latin);
    g_free(password_latin);

    return password_g;
}

/**
 * poppler_document_new_from_file:
 * @uri: uri of the file to load
 * @password: (allow-none): password to unlock the file with, or %NULL
 * @error: (allow-none): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument.  If %NULL is returned, then @error will be
 * set. Possible errors include those in the #POPPLER_ERROR and #G_FILE_ERROR
 * domains.
 *
 * Return value: A newly created #PopplerDocument, or %NULL
 **/
PopplerDocument *poppler_document_new_from_file(const char *uri, const char *password, GError **error)
{
    PDFDoc *newDoc;
    char *filename;

    auto initer = std::make_unique<GlobalParamsIniter>(_poppler_error_cb);

    filename = g_filename_from_uri(uri, nullptr, error);
    if (!filename) {
        return nullptr;
    }

    const std::optional<GooString> password_g = poppler_password_to_latin1(password);

#ifdef G_OS_WIN32
    wchar_t *filenameW;
    int length;

    length = MultiByteToWideChar(CP_UTF8, 0, filename, -1, nullptr, 0);

    filenameW = new WCHAR[length];
    if (!filenameW)
        return nullptr;

    length = MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenameW, length);

    newDoc = new PDFDoc(filenameW, length, password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        delete newDoc;
        newDoc = new PDFDoc(filenameW, length, GooString(password), GooString(password));
    }
    delete[] filenameW;
#else
    newDoc = new PDFDoc(std::make_unique<GooString>(filename), password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        delete newDoc;
        newDoc = new PDFDoc(std::make_unique<GooString>(filename), GooString(password), GooString(password));
    }
#endif
    g_free(filename);

    return _poppler_document_new_from_pdfdoc(std::move(initer), newDoc, error);
}

/**
 * poppler_document_new_from_data:
 * @data: (array length=length) (element-type guint8): the pdf data
 * @length: the length of #data
 * @password: (nullable): password to unlock the file with, or %NULL
 * @error: (nullable): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument.  If %NULL is returned, then @error will be
 * set. Possible errors include those in the #POPPLER_ERROR and #G_FILE_ERROR
 * domains.
 *
 * Note that @data is not copied nor is a new reference to it created.
 * It must remain valid and cannot be destroyed as long as the returned
 * document exists.
 *
 * Return value: A newly created #PopplerDocument, or %NULL
 *
 * Deprecated: 0.82: This requires directly managing @length and @data.
 * Use poppler_document_new_from_bytes() instead.
 **/
PopplerDocument *poppler_document_new_from_data(char *data, int length, const char *password, GError **error)
{
    PDFDoc *newDoc;
    MemStream *str;

    auto initer = std::make_unique<GlobalParamsIniter>(_poppler_error_cb);

    // create stream
    str = new MemStream(data, 0, length, Object(objNull));

    const std::optional<GooString> password_g = poppler_password_to_latin1(password);
    newDoc = new PDFDoc(str, password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        str = dynamic_cast<MemStream *>(str->copy());
        delete newDoc;
        newDoc = new PDFDoc(str, GooString(password), GooString(password));
    }

    return _poppler_document_new_from_pdfdoc(std::move(initer), newDoc, error);
}

class BytesStream : public MemStream
{
    std::unique_ptr<GBytes, decltype(&g_bytes_unref)> m_bytes;

public:
    BytesStream(GBytes *bytes, Object &&dictA) : MemStream(static_cast<const char *>(g_bytes_get_data(bytes, nullptr)), 0, g_bytes_get_size(bytes), std::move(dictA)), m_bytes { g_bytes_ref(bytes), &g_bytes_unref } { }
    ~BytesStream() override;
};

BytesStream::~BytesStream() = default;

class OwningFileStream final : public FileStream
{
public:
    OwningFileStream(std::unique_ptr<GooFile> fileA, Object &&dictA) : FileStream(fileA.get(), 0, false, fileA->size(), std::move(dictA)), file(std::move(fileA)) { }

    ~OwningFileStream() override;

private:
    std::unique_ptr<GooFile> file;
};

OwningFileStream::~OwningFileStream() = default;

/**
 * poppler_document_new_from_bytes:
 * @bytes: a #GBytes
 * @password: (allow-none): password to unlock the file with, or %NULL
 * @error: (allow-none): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument from @bytes. The returned document
 * will hold a reference to @bytes.
 *
 * On error,  %NULL is returned, with @error set. Possible errors include
 * those in the #POPPLER_ERROR and #G_FILE_ERROR domains.
 *
 * Return value: (transfer full): a newly created #PopplerDocument, or %NULL
 *
 * Since: 0.82
 **/
PopplerDocument *poppler_document_new_from_bytes(GBytes *bytes, const char *password, GError **error)
{
    PDFDoc *newDoc;
    BytesStream *str;

    g_return_val_if_fail(bytes != nullptr, nullptr);
    g_return_val_if_fail(error == nullptr || *error == nullptr, nullptr);

    auto initer = std::make_unique<GlobalParamsIniter>(_poppler_error_cb);

    // create stream
    str = new BytesStream(bytes, Object(objNull));

    const std::optional<GooString> password_g = poppler_password_to_latin1(password);
    newDoc = new PDFDoc(str, password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        str = dynamic_cast<BytesStream *>(str->copy());
        delete newDoc;
        newDoc = new PDFDoc(str, GooString(password), GooString(password));
    }

    return _poppler_document_new_from_pdfdoc(std::move(initer), newDoc, error);
}

static inline gboolean stream_is_memory_buffer_or_local_file(GInputStream *stream)
{
    return G_IS_MEMORY_INPUT_STREAM(stream) || (G_IS_FILE_INPUT_STREAM(stream) && strcmp(g_type_name_from_instance((GTypeInstance *)stream), "GLocalFileInputStream") == 0);
}

/**
 * poppler_document_new_from_stream:
 * @stream: a #GInputStream to read from
 * @length: the stream length, or -1 if not known
 * @password: (allow-none): password to unlock the file with, or %NULL
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: (allow-none): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument reading the PDF contents from @stream.
 * Note that the given #GInputStream must be seekable or %G_IO_ERROR_NOT_SUPPORTED
 * will be returned.
 * Possible errors include those in the #POPPLER_ERROR, #G_FILE_ERROR
 * and #G_IO_ERROR domains.
 *
 * Returns: (transfer full): a new #PopplerDocument, or %NULL
 *
 * Since: 0.22
 */
PopplerDocument *poppler_document_new_from_stream(GInputStream *stream, goffset length, const char *password, GCancellable *cancellable, GError **error)
{
    PDFDoc *newDoc;
    BaseStream *str;

    g_return_val_if_fail(G_IS_INPUT_STREAM(stream), NULL);
    g_return_val_if_fail(length == (goffset)-1 || length > 0, NULL);

    auto initer = std::make_unique<GlobalParamsIniter>(_poppler_error_cb);

    if (!G_IS_SEEKABLE(stream) || !g_seekable_can_seek(G_SEEKABLE(stream))) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Stream is not seekable");
        return nullptr;
    }

    if (stream_is_memory_buffer_or_local_file(stream)) {
        if (length == (goffset)-1) {
            if (!g_seekable_seek(G_SEEKABLE(stream), 0, G_SEEK_END, cancellable, error)) {
                g_prefix_error(error, "Unable to determine length of stream: ");
                return nullptr;
            }
            length = g_seekable_tell(G_SEEKABLE(stream));
        }
        str = new PopplerInputStream(stream, cancellable, 0, false, length, Object(objNull));
    } else {
        CachedFile *cachedFile = new CachedFile(new PopplerCachedFileLoader(stream, cancellable, length));
        str = new CachedFileStream(cachedFile, 0, false, cachedFile->getLength(), Object(objNull));
    }

    const std::optional<GooString> password_g = poppler_password_to_latin1(password);
    newDoc = new PDFDoc(str, password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        str = str->copy();
        delete newDoc;
        newDoc = new PDFDoc(str, GooString(password), GooString(password));
    }

    return _poppler_document_new_from_pdfdoc(std::move(initer), newDoc, error);
}

/**
 * poppler_document_new_from_gfile:
 * @file: a #GFile to load
 * @password: (allow-none): password to unlock the file with, or %NULL
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: (allow-none): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument reading the PDF contents from @file.
 * Possible errors include those in the #POPPLER_ERROR and #G_FILE_ERROR
 * domains.
 *
 * Returns: (transfer full): a new #PopplerDocument, or %NULL
 *
 * Since: 0.22
 */
PopplerDocument *poppler_document_new_from_gfile(GFile *file, const char *password, GCancellable *cancellable, GError **error)
{
    PopplerDocument *document;
    GFileInputStream *stream;

    g_return_val_if_fail(G_IS_FILE(file), NULL);

    if (g_file_is_native(file)) {
        gchar *uri;

        uri = g_file_get_uri(file);
        document = poppler_document_new_from_file(uri, password, error);
        g_free(uri);

        return document;
    }

    stream = g_file_read(file, cancellable, error);
    if (!stream) {
        return nullptr;
    }

    document = poppler_document_new_from_stream(G_INPUT_STREAM(stream), -1, password, cancellable, error);
    g_object_unref(stream);

    return document;
}

#ifndef G_OS_WIN32

/**
 * poppler_document_new_from_fd:
 * @fd: a valid file descriptor
 * @password: (allow-none): password to unlock the file with, or %NULL
 * @error: (allow-none): Return location for an error, or %NULL
 *
 * Creates a new #PopplerDocument reading the PDF contents from the file
 * descriptor @fd. @fd must refer to a regular file, or STDIN, and be open
 * for reading.
 * Possible errors include those in the #POPPLER_ERROR and #G_FILE_ERROR
 * domains.
 * Note that this function takes ownership of @fd; you must not operate on it
 * again, nor close it.
 *
 * Returns: (transfer full): a new #PopplerDocument, or %NULL
 *
 * Since: 21.12.0
 */
PopplerDocument *poppler_document_new_from_fd(int fd, const char *password, GError **error)
{
    struct stat statbuf;
    int flags;
    BaseStream *stream;
    PDFDoc *newDoc;

    g_return_val_if_fail(fd != -1, nullptr);

    auto initer = std::make_unique<GlobalParamsIniter>(_poppler_error_cb);

    if (fstat(fd, &statbuf) == -1 || (flags = fcntl(fd, F_GETFL, &flags)) == -1) {
        int errsv = errno;
        g_set_error_literal(error, G_FILE_ERROR, g_file_error_from_errno(errsv), g_strerror(errsv));
        close(fd);
        return nullptr;
    }

    switch (flags & O_ACCMODE) {
    case O_RDONLY:
    case O_RDWR:
        break;
    case O_WRONLY:
    default:
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_BADF, "File descriptor %d is not readable", fd);
        close(fd);
        return nullptr;
    }

    if (fd == fileno(stdin) || !S_ISREG(statbuf.st_mode)) {
        FILE *file;
        if (fd == fileno(stdin)) {
            file = stdin;
        } else {
            file = fdopen(fd, "rb");
            if (!file) {
                int errsv = errno;
                g_set_error_literal(error, G_FILE_ERROR, g_file_error_from_errno(errsv), g_strerror(errsv));
                close(fd);
                return nullptr;
            }
        }

        CachedFile *cachedFile = new CachedFile(new FILECacheLoader(file));
        stream = new CachedFileStream(cachedFile, 0, false, cachedFile->getLength(), Object(objNull));
    } else {
        stream = new OwningFileStream(GooFile::open(fd), Object(objNull));
    }

    const std::optional<GooString> password_g = poppler_password_to_latin1(password);
    newDoc = new PDFDoc(stream, password_g, password_g);
    if (!newDoc->isOk() && newDoc->getErrorCode() == errEncrypted && password) {
        /* Try again with original password (which comes from GTK in UTF8) Issue #824 */
        stream = stream->copy();
        delete newDoc;
        newDoc = new PDFDoc(stream, GooString(password), GooString(password));
    }

    return _poppler_document_new_from_pdfdoc(std::move(initer), newDoc, error);
}

#endif /* !G_OS_WIN32 */

static gboolean handle_save_error(int err_code, GError **error)
{
    switch (err_code) {
    case errNone:
        break;
    case errOpenFile:
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_OPEN_FILE, "Failed to open file for writing");
        break;
    case errEncrypted:
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_ENCRYPTED, "Document is encrypted");
        break;
    default:
        g_set_error(error, POPPLER_ERROR, POPPLER_ERROR_INVALID, "Failed to save document");
    }

    return err_code == errNone;
}

/**
 * poppler_document_save:
 * @document: a #PopplerDocument
 * @uri: uri of file to save
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Saves @document. Any change made in the document such as
 * form fields filled, annotations added or modified
 * will be saved.
 * If @error is set, %FALSE will be returned. Possible errors
 * include those in the #G_FILE_ERROR domain.
 *
 * Return value: %TRUE, if the document was successfully saved
 **/
gboolean poppler_document_save(PopplerDocument *document, const char *uri, GError **error)
{
    char *filename;
    gboolean retval = FALSE;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    filename = g_filename_from_uri(uri, nullptr, error);
    if (filename != nullptr) {
        GooString fname(filename);
        int err_code;
        g_free(filename);

        err_code = document->doc->saveAs(fname);
        retval = handle_save_error(err_code, error);
    }

    return retval;
}

/**
 * poppler_document_save_a_copy:
 * @document: a #PopplerDocument
 * @uri: uri of file to save
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Saves a copy of the original @document.
 * Any change made in the document such as
 * form fields filled by the user will not be saved.
 * If @error is set, %FALSE will be returned. Possible errors
 * include those in the #G_FILE_ERROR domain.
 *
 * Return value: %TRUE, if the document was successfully saved
 **/
gboolean poppler_document_save_a_copy(PopplerDocument *document, const char *uri, GError **error)
{
    char *filename;
    gboolean retval = FALSE;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    filename = g_filename_from_uri(uri, nullptr, error);
    if (filename != nullptr) {
        GooString fname(filename);
        int err_code;
        g_free(filename);

        err_code = document->doc->saveWithoutChangesAs(fname);
        retval = handle_save_error(err_code, error);
    }

    return retval;
}

#ifndef G_OS_WIN32

/**
 * poppler_document_save_to_fd:
 * @document: a #PopplerDocument
 * @fd: a valid file descriptor open for writing
 * @include_changes: whether to include user changes (e.g. form fills)
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Saves @document. Any change made in the document such as
 * form fields filled, annotations added or modified
 * will be saved if @include_changes is %TRUE, or discarded i
 * @include_changes is %FALSE.
 *
 * Note that this function takes ownership of @fd; you must not operate on it
 * again, nor close it.
 *
 * If @error is set, %FALSE will be returned. Possible errors
 * include those in the #G_FILE_ERROR domain.
 *
 * Return value: %TRUE, if the document was successfully saved
 *
 * Since: 21.12.0
 **/
gboolean poppler_document_save_to_fd(PopplerDocument *document, int fd, gboolean include_changes, GError **error)
{
    FILE *file;
    OutStream *stream;
    int rv;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);
    g_return_val_if_fail(fd != -1, FALSE);

    file = fdopen(fd, "wb");
    if (file == nullptr) {
        int errsv = errno;
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errsv), "Failed to open FD %d for writing: %s", fd, g_strerror(errsv));
        return FALSE;
    }

    stream = new FileOutStream(file, 0);
    if (include_changes) {
        rv = document->doc->saveAs(stream);
    } else {
        rv = document->doc->saveWithoutChangesAs(stream);
    }
    delete stream;

    return handle_save_error(rv, error);
}

#endif /* !G_OS_WIN32 */

static void poppler_document_finalize(GObject *object)
{
    PopplerDocument *document = POPPLER_DOCUMENT(object);

    poppler_document_layers_free(document);
    delete document->output_dev;
    delete document->doc;
    delete document->initer.release();

    G_OBJECT_CLASS(poppler_document_parent_class)->finalize(object);
}

/**
 * poppler_document_get_id:
 * @document: A #PopplerDocument
 * @permanent_id: (out) (allow-none): location to store an allocated string, use g_free() to free the returned string
 * @update_id: (out) (allow-none): location to store an allocated string, use g_free() to free the returned string
 *
 * Returns the PDF file identifier represented as two byte string arrays of size 32.
 * @permanent_id is the permanent identifier that is built based on the file
 * contents at the time it was originally created, so that this identifer
 * never changes. @update_id is the update identifier that is built based on
 * the file contents at the time it was last updated.
 *
 * Note that returned strings are not null-terminated, they have a fixed
 * size of 32 bytes.
 *
 * Returns: %TRUE if the @document contains an id, %FALSE otherwise
 *
 * Since: 0.16
 */
gboolean poppler_document_get_id(PopplerDocument *document, gchar **permanent_id, gchar **update_id)
{
    GooString permanent;
    GooString update;
    gboolean retval = FALSE;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    if (permanent_id) {
        *permanent_id = nullptr;
    }
    if (update_id) {
        *update_id = nullptr;
    }

    if (document->doc->getID(permanent_id ? &permanent : nullptr, update_id ? &update : nullptr)) {
        if (permanent_id) {
            *permanent_id = g_new(char, 32);
            memcpy(*permanent_id, permanent.c_str(), 32);
        }
        if (update_id) {
            *update_id = g_new(char, 32);
            memcpy(*update_id, update.c_str(), 32);
        }

        retval = TRUE;
    }

    return retval;
}

/**
 * poppler_document_get_n_pages:
 * @document: A #PopplerDocument
 *
 * Returns the number of pages in a loaded document.
 *
 * Return value: Number of pages
 **/
int poppler_document_get_n_pages(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), 0);

    return document->doc->getNumPages();
}

/**
 * poppler_document_get_page:
 * @document: A #PopplerDocument
 * @index: a page index
 *
 * Returns the #PopplerPage indexed at @index.  This object is owned by the
 * caller.
 *
 * Return value: (transfer full) : The #PopplerPage at @index
 **/
PopplerPage *poppler_document_get_page(PopplerDocument *document, int index)
{
    Page *page;

    g_return_val_if_fail(0 <= index && index < poppler_document_get_n_pages(document), NULL);

    page = document->doc->getPage(index + 1);
    if (!page) {
        return nullptr;
    }

    return _poppler_page_new(document, page, index);
}

/**
 * poppler_document_get_page_by_label:
 * @document: A #PopplerDocument
 * @label: a page label
 *
 * Returns the #PopplerPage reference by @label.  This object is owned by the
 * caller.  @label is a human-readable string representation of the page number,
 * and can be document specific.  Typically, it is a value such as "iii" or "3".
 *
 * By default, "1" refers to the first page.
 *
 * Return value: (transfer full) :The #PopplerPage referenced by @label
 **/
PopplerPage *poppler_document_get_page_by_label(PopplerDocument *document, const char *label)
{
    Catalog *catalog;
    GooString label_g(label);
    int index;

    catalog = document->doc->getCatalog();
    if (!catalog->labelToIndex(&label_g, &index)) {
        return nullptr;
    }

    return poppler_document_get_page(document, index);
}

/**
 * poppler_document_get_n_attachments:
 * @document: A #PopplerDocument
 *
 * Returns the number of attachments in a loaded document.
 * See also poppler_document_get_attachments()
 *
 * Return value: Number of attachments
 *
 * Since: 0.18
 */
guint poppler_document_get_n_attachments(PopplerDocument *document)
{
    Catalog *catalog;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), 0);

    catalog = document->doc->getCatalog();

    return catalog && catalog->isOk() ? catalog->numEmbeddedFiles() : 0;
}

/**
 * poppler_document_has_attachments:
 * @document: A #PopplerDocument
 *
 * Returns %TRUE of @document has any attachments.
 *
 * Return value: %TRUE, if @document has attachments.
 **/
gboolean poppler_document_has_attachments(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    return (poppler_document_get_n_attachments(document) != 0);
}

/**
 * poppler_document_get_attachments:
 * @document: A #PopplerDocument
 *
 * Returns a #GList containing #PopplerAttachment<!-- -->s.  These attachments
 * are unowned, and must be unreffed, and the list must be freed with
 * g_list_free().
 *
 * Return value: (element-type PopplerAttachment) (transfer full): a list of available attachments.
 **/
GList *poppler_document_get_attachments(PopplerDocument *document)
{
    Catalog *catalog;
    int n_files, i;
    GList *retval = nullptr;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    catalog = document->doc->getCatalog();
    if (catalog == nullptr || !catalog->isOk()) {
        return nullptr;
    }

    n_files = catalog->numEmbeddedFiles();
    for (i = 0; i < n_files; i++) {
        PopplerAttachment *attachment;

        const std::unique_ptr<FileSpec> emb_file = catalog->embeddedFile(i);
        if (!emb_file->isOk() || !emb_file->getEmbeddedFile()->isOk()) {
            continue;
        }

        attachment = _poppler_attachment_new(emb_file.get());

        if (attachment != nullptr) {
            retval = g_list_prepend(retval, attachment);
        }
    }
    return g_list_reverse(retval);
}

/**
 * poppler_named_dest_from_bytestring:
 * @data: (array length=length): the bytestring data
 * @length: the bytestring length
 *
 * Converts a bytestring into a zero-terminated string suitable to
 * pass to poppler_document_find_dest().
 *
 * Note that the returned string has no defined encoding and is not
 * suitable for display to the user.
 *
 * The returned data must be freed using g_free().
 *
 * Returns: (transfer full): the named dest
 *
 * Since: 0.73
 */
char *poppler_named_dest_from_bytestring(const guint8 *data, gsize length)
{
    const guint8 *p, *pend;
    char *dest, *q;

    g_return_val_if_fail(length != 0 || data != nullptr, nullptr);
    /* Each source byte needs maximally 2 destination chars (\\ or \0) */
    q = dest = (gchar *)g_malloc(length * 2 + 1);

    pend = data + length;
    for (p = data; p < pend; ++p) {
        switch (*p) {
        case '\0':
            *q++ = '\\';
            *q++ = '0';
            break;
        case '\\':
            *q++ = '\\';
            *q++ = '\\';
            break;
        default:
            *q++ = *p;
            break;
        }
    }

    *q = 0; /* zero terminate */
    return dest;
}

/**
 * poppler_named_dest_to_bytestring:
 * @name: the named dest string
 * @length: (out): a location to store the length of the returned bytestring
 *
 * Converts a named dest string (e.g. from #PopplerDest.named_dest) into a
 * bytestring, inverting the transformation of
 * poppler_named_dest_from_bytestring().
 *
 * Note that the returned data is not zero terminated and may also
 * contains embedded NUL bytes.
 *
 * If @name is not a valid named dest string, returns %NULL.
 *
 * The returned data must be freed using g_free().
 *
 * Returns: (array length=length) (transfer full) (nullable): a new bytestring,
 *   or %NULL
 *
 * Since: 0.73
 */
guint8 *poppler_named_dest_to_bytestring(const char *name, gsize *length)
{
    const char *p;
    guint8 *data, *q;
    gsize len;

    g_return_val_if_fail(name != nullptr, nullptr);
    g_return_val_if_fail(length != nullptr, nullptr);

    len = strlen(name);
    q = data = (guint8 *)g_malloc(len);
    for (p = name; *p; ++p) {
        if (*p == '\\') {
            p++;
            len--;
            if (*p == '0') {
                *q++ = '\0';
            } else if (*p == '\\') {
                *q++ = '\\';
            } else {
                goto invalid;
            }
        } else {
            *q++ = *p;
        }
    }

    *length = len;
    return data;

invalid:
    g_free(data);
    *length = 0;
    return nullptr;
}

/**
 * poppler_document_find_dest:
 * @document: A #PopplerDocument
 * @link_name: a named destination
 *
 * Creates a #PopplerDest for the named destination @link_name in @document.
 *
 * Note that named destinations are bytestrings, not string. That means that
 * unless @link_name was returned by a poppler function (e.g. is
 * #PopplerDest.named_dest), it needs to be converted to string
 * using poppler_named_dest_from_bytestring() before being passed to this
 * function.
 *
 * The returned value must be freed with poppler_dest_free().
 *
 * Return value: (transfer full): a new #PopplerDest destination, or %NULL if
 *   @link_name is not a destination.
 **/
PopplerDest *poppler_document_find_dest(PopplerDocument *document, const gchar *link_name)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);
    g_return_val_if_fail(link_name != nullptr, nullptr);

    gsize len;
    guint8 *data = poppler_named_dest_to_bytestring(link_name, &len);
    if (data == nullptr) {
        return nullptr;
    }

    GooString g_link_name((const char *)data, (int)len);
    g_free(data);

    std::unique_ptr<LinkDest> link_dest = document->doc->findDest(&g_link_name);
    if (link_dest == nullptr) {
        return nullptr;
    }

    PopplerDest *dest = _poppler_dest_new_goto(document, link_dest.get());

    return dest;
}

static gint _poppler_dest_compare_keys(gconstpointer a, gconstpointer b, gpointer user_data)
{
    return g_strcmp0(static_cast<const gchar *>(a), static_cast<const gchar *>(b));
}

static void _poppler_dest_destroy_value(gpointer value)
{
    poppler_dest_free(static_cast<PopplerDest *>(value));
}

/**
 * poppler_document_create_dests_tree:
 * @document: A #PopplerDocument
 *
 * Creates a balanced binary tree of all named destinations in @document
 *
 * The tree key is strings in the form returned by
 * poppler_named_dest_to_bytestring() which constains a destination name.
 * The tree value is the #PopplerDest which contains a named destination.
 * The return value must be freed with g_tree_destroy().
 *
 * Returns: (transfer full) (nullable): the #GTree, or %NULL
 * Since: 0.78
 **/
GTree *poppler_document_create_dests_tree(PopplerDocument *document)
{
    GTree *tree;
    Catalog *catalog;
    PopplerDest *dest;
    int i;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);

    catalog = document->doc->getCatalog();
    if (catalog == nullptr) {
        return nullptr;
    }

    tree = g_tree_new_full(_poppler_dest_compare_keys, nullptr, g_free, _poppler_dest_destroy_value);

    // Iterate from name-dict
    const int nDests = catalog->numDests();
    for (i = 0; i < nDests; ++i) {
        // The names of name-dict cannot contain \0,
        // so we can use strlen().
        auto name = catalog->getDestsName(i);
        std::unique_ptr<LinkDest> link_dest = catalog->getDestsDest(i);
        if (link_dest) {
            gchar *key = poppler_named_dest_from_bytestring(reinterpret_cast<const guint8 *>(name), strlen(name));
            dest = _poppler_dest_new_goto(document, link_dest.get());
            g_tree_insert(tree, key, dest);
        }
    }

    // Iterate form name-tree
    const int nDestsNameTree = catalog->numDestNameTree();
    for (i = 0; i < nDestsNameTree; ++i) {
        auto name = catalog->getDestNameTreeName(i);
        std::unique_ptr<LinkDest> link_dest = catalog->getDestNameTreeDest(i);
        if (link_dest) {
            gchar *key = poppler_named_dest_from_bytestring(reinterpret_cast<const guint8 *>(name->c_str()), name->getLength());
            dest = _poppler_dest_new_goto(document, link_dest.get());
            g_tree_insert(tree, key, dest);
        }
    }

    return tree;
}

char *_poppler_goo_string_to_utf8(const GooString *s)
{
    if (s == nullptr) {
        return nullptr;
    }

    char *result;

    if (hasUnicodeByteOrderMark(s->toStr())) {
        result = g_convert(s->c_str() + 2, s->getLength() - 2, "UTF-8", "UTF-16BE", nullptr, nullptr, nullptr);
    } else if (hasUnicodeByteOrderMarkLE(s->toStr())) {
        result = g_convert(s->c_str() + 2, s->getLength() - 2, "UTF-8", "UTF-16LE", nullptr, nullptr, nullptr);
    } else {
        int len;
        gunichar *ucs4_temp;
        int i;

        len = s->getLength();
        ucs4_temp = g_new(gunichar, len + 1);
        for (i = 0; i < len; ++i) {
            ucs4_temp[i] = pdfDocEncoding[(unsigned char)s->getChar(i)];
        }
        ucs4_temp[i] = 0;

        result = g_ucs4_to_utf8(ucs4_temp, -1, nullptr, nullptr, nullptr);

        g_free(ucs4_temp);
    }

    return result;
}

static GooString *_poppler_goo_string_from_utf8(const gchar *src)
{
    if (src == nullptr) {
        return nullptr;
    }

    gsize outlen;

    gchar *utf16 = g_convert(src, -1, "UTF-16BE", "UTF-8", nullptr, &outlen, nullptr);
    if (utf16 == nullptr) {
        return nullptr;
    }

    GooString *result = new GooString(utf16, outlen);
    g_free(utf16);

    if (!hasUnicodeByteOrderMark(result->toStr())) {
        prependUnicodeByteOrderMark(result->toNonConstStr());
    }

    return result;
}

static PopplerPageLayout convert_page_layout(Catalog::PageLayout pageLayout)
{
    switch (pageLayout) {
    case Catalog::pageLayoutSinglePage:
        return POPPLER_PAGE_LAYOUT_SINGLE_PAGE;
    case Catalog::pageLayoutOneColumn:
        return POPPLER_PAGE_LAYOUT_ONE_COLUMN;
    case Catalog::pageLayoutTwoColumnLeft:
        return POPPLER_PAGE_LAYOUT_TWO_COLUMN_LEFT;
    case Catalog::pageLayoutTwoColumnRight:
        return POPPLER_PAGE_LAYOUT_TWO_COLUMN_RIGHT;
    case Catalog::pageLayoutTwoPageLeft:
        return POPPLER_PAGE_LAYOUT_TWO_PAGE_LEFT;
    case Catalog::pageLayoutTwoPageRight:
        return POPPLER_PAGE_LAYOUT_TWO_PAGE_RIGHT;
    case Catalog::pageLayoutNone:
    default:
        return POPPLER_PAGE_LAYOUT_UNSET;
    }
}

static PopplerPageMode convert_page_mode(Catalog::PageMode pageMode)
{
    switch (pageMode) {
    case Catalog::pageModeOutlines:
        return POPPLER_PAGE_MODE_USE_OUTLINES;
    case Catalog::pageModeThumbs:
        return POPPLER_PAGE_MODE_USE_THUMBS;
    case Catalog::pageModeFullScreen:
        return POPPLER_PAGE_MODE_FULL_SCREEN;
    case Catalog::pageModeOC:
        return POPPLER_PAGE_MODE_USE_OC;
    case Catalog::pageModeAttach:
        return POPPLER_PAGE_MODE_USE_ATTACHMENTS;
    case Catalog::pageModeNone:
    default:
        return POPPLER_PAGE_MODE_UNSET;
    }
}

static PopplerPDFSubtype convert_pdf_subtype(PDFSubtype pdfSubtype)
{
    switch (pdfSubtype) {
    case subtypePDFA:
        return POPPLER_PDF_SUBTYPE_PDF_A;
    case subtypePDFE:
        return POPPLER_PDF_SUBTYPE_PDF_E;
    case subtypePDFUA:
        return POPPLER_PDF_SUBTYPE_PDF_UA;
    case subtypePDFVT:
        return POPPLER_PDF_SUBTYPE_PDF_VT;
    case subtypePDFX:
        return POPPLER_PDF_SUBTYPE_PDF_X;
    case subtypeNone:
        return POPPLER_PDF_SUBTYPE_NONE;
    case subtypeNull:
    default:
        return POPPLER_PDF_SUBTYPE_UNSET;
    }
}

static PopplerPDFPart convert_pdf_subtype_part(PDFSubtypePart pdfSubtypePart)
{
    switch (pdfSubtypePart) {
    case subtypePart1:
        return POPPLER_PDF_SUBTYPE_PART_1;
    case subtypePart2:
        return POPPLER_PDF_SUBTYPE_PART_2;
    case subtypePart3:
        return POPPLER_PDF_SUBTYPE_PART_3;
    case subtypePart4:
        return POPPLER_PDF_SUBTYPE_PART_4;
    case subtypePart5:
        return POPPLER_PDF_SUBTYPE_PART_5;
    case subtypePart6:
        return POPPLER_PDF_SUBTYPE_PART_6;
    case subtypePart7:
        return POPPLER_PDF_SUBTYPE_PART_7;
    case subtypePart8:
        return POPPLER_PDF_SUBTYPE_PART_8;
    case subtypePartNone:
        return POPPLER_PDF_SUBTYPE_PART_NONE;
    case subtypePartNull:
    default:
        return POPPLER_PDF_SUBTYPE_PART_UNSET;
    }
}

static PopplerPDFConformance convert_pdf_subtype_conformance(PDFSubtypeConformance pdfSubtypeConf)
{
    switch (pdfSubtypeConf) {
    case subtypeConfA:
        return POPPLER_PDF_SUBTYPE_CONF_A;
    case subtypeConfB:
        return POPPLER_PDF_SUBTYPE_CONF_B;
    case subtypeConfG:
        return POPPLER_PDF_SUBTYPE_CONF_G;
    case subtypeConfN:
        return POPPLER_PDF_SUBTYPE_CONF_N;
    case subtypeConfP:
        return POPPLER_PDF_SUBTYPE_CONF_P;
    case subtypeConfPG:
        return POPPLER_PDF_SUBTYPE_CONF_PG;
    case subtypeConfU:
        return POPPLER_PDF_SUBTYPE_CONF_U;
    case subtypeConfNone:
        return POPPLER_PDF_SUBTYPE_CONF_NONE;
    case subtypeConfNull:
    default:
        return POPPLER_PDF_SUBTYPE_CONF_UNSET;
    }
}

/**
 * poppler_document_get_pdf_version_string:
 * @document: A #PopplerDocument
 *
 * Returns the PDF version of @document as a string (e.g. PDF-1.6)
 *
 * Return value: a new allocated string containing the PDF version
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_pdf_version_string(PopplerDocument *document)
{
    gchar *retval;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    retval = g_strndup("PDF-", 15); /* allocates 16 chars, pads with \0s */
    g_ascii_formatd(retval + 4, 15 + 1 - 4, "%.2g", document->doc->getPDFMajorVersion() + document->doc->getPDFMinorVersion() / 10.0);
    return retval;
}

/**
 * poppler_document_get_pdf_version:
 * @document: A #PopplerDocument
 * @major_version: (out) (nullable): return location for the PDF major version number
 * @minor_version: (out) (nullable): return location for the PDF minor version number
 *
 * Updates values referenced by @major_version & @minor_version with the
 * major and minor PDF versions of @document.
 *
 * Since: 0.16
 **/
void poppler_document_get_pdf_version(PopplerDocument *document, guint *major_version, guint *minor_version)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    if (major_version) {
        *major_version = document->doc->getPDFMajorVersion();
    }
    if (minor_version) {
        *minor_version = document->doc->getPDFMinorVersion();
    }
}

/**
 * poppler_document_get_title:
 * @document: A #PopplerDocument
 *
 * Returns the document's title
 *
 * Return value: a new allocated string containing the title
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_title(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_title = document->doc->getDocInfoTitle();
    return _poppler_goo_string_to_utf8(goo_title.get());
}

/**
 * poppler_document_set_title:
 * @document: A #PopplerDocument
 * @title: A new title
 *
 * Sets the document's title. If @title is %NULL, Title entry
 * is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_title(PopplerDocument *document, const gchar *title)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_title;
    if (!title) {
        goo_title = nullptr;
    } else {
        goo_title = _poppler_goo_string_from_utf8(title);
        if (!goo_title) {
            return;
        }
    }
    document->doc->setDocInfoTitle(goo_title);
}

/**
 * poppler_document_get_author:
 * @document: A #PopplerDocument
 *
 * Returns the author of the document
 *
 * Return value: a new allocated string containing the author
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_author(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_author = document->doc->getDocInfoAuthor();
    return _poppler_goo_string_to_utf8(goo_author.get());
}

/**
 * poppler_document_set_author:
 * @document: A #PopplerDocument
 * @author: A new author
 *
 * Sets the document's author. If @author is %NULL, Author
 * entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_author(PopplerDocument *document, const gchar *author)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_author;
    if (!author) {
        goo_author = nullptr;
    } else {
        goo_author = _poppler_goo_string_from_utf8(author);
        if (!goo_author) {
            return;
        }
    }
    document->doc->setDocInfoAuthor(goo_author);
}

/**
 * poppler_document_get_subject:
 * @document: A #PopplerDocument
 *
 * Returns the subject of the document
 *
 * Return value: a new allocated string containing the subject
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_subject(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_subject = document->doc->getDocInfoSubject();
    return _poppler_goo_string_to_utf8(goo_subject.get());
}

/**
 * poppler_document_set_subject:
 * @document: A #PopplerDocument
 * @subject: A new subject
 *
 * Sets the document's subject. If @subject is %NULL, Subject
 * entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_subject(PopplerDocument *document, const gchar *subject)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_subject;
    if (!subject) {
        goo_subject = nullptr;
    } else {
        goo_subject = _poppler_goo_string_from_utf8(subject);
        if (!goo_subject) {
            return;
        }
    }
    document->doc->setDocInfoSubject(goo_subject);
}

/**
 * poppler_document_get_keywords:
 * @document: A #PopplerDocument
 *
 * Returns the keywords associated to the document
 *
 * Return value: a new allocated string containing keywords associated
 *               to @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_keywords(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_keywords = document->doc->getDocInfoKeywords();
    return _poppler_goo_string_to_utf8(goo_keywords.get());
}

/**
 * poppler_document_set_keywords:
 * @document: A #PopplerDocument
 * @keywords: New keywords
 *
 * Sets the document's keywords. If @keywords is %NULL,
 * Keywords entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_keywords(PopplerDocument *document, const gchar *keywords)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_keywords;
    if (!keywords) {
        goo_keywords = nullptr;
    } else {
        goo_keywords = _poppler_goo_string_from_utf8(keywords);
        if (!goo_keywords) {
            return;
        }
    }
    document->doc->setDocInfoKeywords(goo_keywords);
}

/**
 * poppler_document_get_creator:
 * @document: A #PopplerDocument
 *
 * Returns the creator of the document. If the document was converted
 * from another format, the creator is the name of the product
 * that created the original document from which it was converted.
 *
 * Return value: a new allocated string containing the creator
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_creator(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_creator = document->doc->getDocInfoCreator();
    return _poppler_goo_string_to_utf8(goo_creator.get());
}

/**
 * poppler_document_set_creator:
 * @document: A #PopplerDocument
 * @creator: A new creator
 *
 * Sets the document's creator. If @creator is %NULL, Creator
 * entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_creator(PopplerDocument *document, const gchar *creator)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_creator;
    if (!creator) {
        goo_creator = nullptr;
    } else {
        goo_creator = _poppler_goo_string_from_utf8(creator);
        if (!goo_creator) {
            return;
        }
    }
    document->doc->setDocInfoCreator(goo_creator);
}

/**
 * poppler_document_get_producer:
 * @document: A #PopplerDocument
 *
 * Returns the producer of the document. If the document was converted
 * from another format, the producer is the name of the product
 * that converted it to PDF
 *
 * Return value: a new allocated string containing the producer
 *               of @document, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_producer(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    const std::unique_ptr<GooString> goo_producer = document->doc->getDocInfoProducer();
    return _poppler_goo_string_to_utf8(goo_producer.get());
}

/**
 * poppler_document_set_producer:
 * @document: A #PopplerDocument
 * @producer: A new producer
 *
 * Sets the document's producer. If @producer is %NULL,
 * Producer entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_producer(PopplerDocument *document, const gchar *producer)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *goo_producer;
    if (!producer) {
        goo_producer = nullptr;
    } else {
        goo_producer = _poppler_goo_string_from_utf8(producer);
        if (!goo_producer) {
            return;
        }
    }
    document->doc->setDocInfoProducer(goo_producer);
}

/**
 * poppler_document_get_creation_date:
 * @document: A #PopplerDocument
 *
 * Returns the date the document was created as seconds since the Epoch
 *
 * Return value: the date the document was created, or -1
 *
 * Since: 0.16
 **/
time_t poppler_document_get_creation_date(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), (time_t)-1);

    const std::unique_ptr<GooString> str = document->doc->getDocInfoCreatDate();
    if (!str) {
        return (time_t)-1;
    }

    time_t date;
    gboolean success = _poppler_convert_pdf_date_to_gtime(str.get(), &date);

    return (success) ? date : (time_t)-1;
}

/**
 * poppler_document_set_creation_date:
 * @document: A #PopplerDocument
 * @creation_date: A new creation date
 *
 * Sets the document's creation date. If @creation_date is -1, CreationDate
 * entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_creation_date(PopplerDocument *document, time_t creation_date)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *str = creation_date == (time_t)-1 ? nullptr : timeToDateString(&creation_date);
    document->doc->setDocInfoCreatDate(str);
}

/**
 * poppler_document_get_creation_date_time:
 * @document: A #PopplerDocument
 *
 * Returns the date the document was created as a #GDateTime
 *
 * Returns: (nullable): the date the document was created, or %NULL
 *
 * Since: 20.09.0
 **/
GDateTime *poppler_document_get_creation_date_time(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);

    std::unique_ptr<GooString> str { document->doc->getDocInfoCreatDate() };

    if (!str) {
        return nullptr;
    }

    return _poppler_convert_pdf_date_to_date_time(str.get());
}

/**
 * poppler_document_set_creation_date_time:
 * @document: A #PopplerDocument
 * @creation_datetime: (nullable): A new creation #GDateTime
 *
 * Sets the document's creation date. If @creation_datetime is %NULL,
 * CreationDate entry is removed from the document's Info dictionary.
 *
 * Since: 20.09.0
 **/
void poppler_document_set_creation_date_time(PopplerDocument *document, GDateTime *creation_datetime)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *str = nullptr;

    if (creation_datetime) {
        str = _poppler_convert_date_time_to_pdf_date(creation_datetime);
    }

    document->doc->setDocInfoCreatDate(str);
}

/**
 * poppler_document_get_modification_date:
 * @document: A #PopplerDocument
 *
 * Returns the date the document was most recently modified as seconds since the Epoch
 *
 * Return value: the date the document was most recently modified, or -1
 *
 * Since: 0.16
 **/
time_t poppler_document_get_modification_date(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), (time_t)-1);

    const std::unique_ptr<GooString> str = document->doc->getDocInfoModDate();
    if (!str) {
        return (time_t)-1;
    }

    time_t date;
    gboolean success = _poppler_convert_pdf_date_to_gtime(str.get(), &date);

    return (success) ? date : (time_t)-1;
}

/**
 * poppler_document_set_modification_date:
 * @document: A #PopplerDocument
 * @modification_date: A new modification date
 *
 * Sets the document's modification date. If @modification_date is -1, ModDate
 * entry is removed from the document's Info dictionary.
 *
 * Since: 0.46
 **/
void poppler_document_set_modification_date(PopplerDocument *document, time_t modification_date)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *str = modification_date == (time_t)-1 ? nullptr : timeToDateString(&modification_date);
    document->doc->setDocInfoModDate(str);
}

/**
 * poppler_document_get_modification_date_time:
 * @document: A #PopplerDocument
 *
 * Returns the date the document was most recently modified as a #GDateTime
 *
 * Returns: (nullable): the date the document was modified, or %NULL
 *
 * Since: 20.09.0
 **/
GDateTime *poppler_document_get_modification_date_time(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);

    std::unique_ptr<GooString> str { document->doc->getDocInfoModDate() };

    if (!str) {
        return nullptr;
    }

    return _poppler_convert_pdf_date_to_date_time(str.get());
}

/**
 * poppler_document_set_modification_date_time:
 * @document: A #PopplerDocument
 * @modification_datetime: (nullable): A new modification #GDateTime
 *
 * Sets the document's modification date. If @modification_datetime is %NULL,
 * ModDate entry is removed from the document's Info dictionary.
 *
 * Since: 20.09.0
 **/
void poppler_document_set_modification_date_time(PopplerDocument *document, GDateTime *modification_datetime)
{
    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    GooString *str = nullptr;

    if (modification_datetime) {
        str = _poppler_convert_date_time_to_pdf_date(modification_datetime);
    }

    document->doc->setDocInfoModDate(str);
}

/**
 * poppler_document_is_linearized:
 * @document: A #PopplerDocument
 *
 * Returns whether @document is linearized or not. Linearization of PDF
 * enables efficient incremental access of the PDF file in a network environment.
 *
 * Return value: %TRUE if @document is linearized, %FALSE otherwise
 *
 * Since: 0.16
 **/
gboolean poppler_document_is_linearized(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    return document->doc->isLinearized();
}

/**
 * poppler_document_get_n_signatures:
 * @document: A #PopplerDocument
 *
 * Returns how many digital signatures @document contains.
 * PDF digital signatures ensure that the content hash not been altered since last edit and
 * that it was produced by someone the user can trust
 *
 * Return value: The number of signatures found in the document
 *
 * Since: 21.12.0
 **/
gint poppler_document_get_n_signatures(const PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), 0);

    return document->doc->getSignatureFields().size();
}

/**
 * poppler_document_get_signature_fields:
 * @document: A #PopplerDocument
 *
 * Returns a #GList containing all signature #PopplerFormField<!-- -->s in the document.
 *
 * Return value: (element-type PopplerFormField) (transfer full): a list of all signature form fields.
 *
 * Since: 22.02.0
 **/
GList *poppler_document_get_signature_fields(PopplerDocument *document)
{
    std::vector<FormFieldSignature *> signature_fields;
    FormWidget *widget;
    GList *result = nullptr;
    gsize i;

    signature_fields = document->doc->getSignatureFields();

    for (i = 0; i < signature_fields.size(); i++) {
        widget = signature_fields[i]->getCreateWidget();

        if (widget != nullptr) {
            result = g_list_prepend(result, _poppler_form_field_new(document, widget));
        }
    }

    return g_list_reverse(result);
}

/**
 * poppler_document_get_page_layout:
 * @document: A #PopplerDocument
 *
 * Returns the page layout that should be used when the document is opened
 *
 * Return value: a #PopplerPageLayout that should be used when the document is opened
 *
 * Since: 0.16
 **/
PopplerPageLayout poppler_document_get_page_layout(PopplerDocument *document)
{
    Catalog *catalog;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PAGE_LAYOUT_UNSET);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        return convert_page_layout(catalog->getPageLayout());
    }

    return POPPLER_PAGE_LAYOUT_UNSET;
}

/**
 * poppler_document_get_page_mode:
 * @document: A #PopplerDocument
 *
 * Returns a #PopplerPageMode representing how the document should
 * be initially displayed when opened.
 *
 * Return value: a #PopplerPageMode that should be used when document is opened
 *
 * Since: 0.16
 **/
PopplerPageMode poppler_document_get_page_mode(PopplerDocument *document)
{
    Catalog *catalog;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PAGE_MODE_UNSET);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        return convert_page_mode(catalog->getPageMode());
    }

    return POPPLER_PAGE_MODE_UNSET;
}

/**
 * poppler_document_get_print_scaling:
 * @document: A #PopplerDocument
 *
 * Returns the print scaling value suggested by author of the document.
 *
 * Return value: a #PopplerPrintScaling that should be used when document is printed
 *
 * Since: 0.73
 **/
PopplerPrintScaling poppler_document_get_print_scaling(PopplerDocument *document)
{
    Catalog *catalog;
    ViewerPreferences *preferences;
    PopplerPrintScaling print_scaling = POPPLER_PRINT_SCALING_APP_DEFAULT;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PRINT_SCALING_APP_DEFAULT);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        preferences = catalog->getViewerPreferences();
        if (preferences) {
            switch (preferences->getPrintScaling()) {
            default:
            case ViewerPreferences::PrintScaling::printScalingAppDefault:
                print_scaling = POPPLER_PRINT_SCALING_APP_DEFAULT;
                break;
            case ViewerPreferences::PrintScaling::printScalingNone:
                print_scaling = POPPLER_PRINT_SCALING_NONE;
                break;
            }
        }
    }

    return print_scaling;
}

/**
 * poppler_document_get_print_duplex:
 * @document: A #PopplerDocument
 *
 * Returns the duplex mode value suggested for printing by author of the document.
 * Value POPPLER_PRINT_DUPLEX_NONE means that the document does not specify this
 * preference.
 *
 * Returns: a #PopplerPrintDuplex that should be used when document is printed
 *
 * Since: 0.80
 **/
PopplerPrintDuplex poppler_document_get_print_duplex(PopplerDocument *document)
{
    Catalog *catalog;
    ViewerPreferences *preferences;
    PopplerPrintDuplex duplex = POPPLER_PRINT_DUPLEX_NONE;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PRINT_DUPLEX_NONE);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        preferences = catalog->getViewerPreferences();
        if (preferences) {
            switch (preferences->getDuplex()) {
            default:
            case ViewerPreferences::Duplex::duplexNone:
                duplex = POPPLER_PRINT_DUPLEX_NONE;
                break;
            case ViewerPreferences::Duplex::duplexSimplex:
                duplex = POPPLER_PRINT_DUPLEX_SIMPLEX;
                break;
            case ViewerPreferences::Duplex::duplexDuplexFlipShortEdge:
                duplex = POPPLER_PRINT_DUPLEX_DUPLEX_FLIP_SHORT_EDGE;
                break;
            case ViewerPreferences::Duplex::duplexDuplexFlipLongEdge:
                duplex = POPPLER_PRINT_DUPLEX_DUPLEX_FLIP_LONG_EDGE;
                break;
            }
        }
    }

    return duplex;
}

/**
 * poppler_document_get_print_n_copies:
 * @document: A #PopplerDocument
 *
 * Returns the suggested number of copies to be printed.
 * This preference should be applied only if returned value
 * is greater than 1 since value 1 usually means that
 * the document does not specify it.
 *
 * Returns: Number of copies
 *
 * Since: 0.80
 **/
gint poppler_document_get_print_n_copies(PopplerDocument *document)
{
    Catalog *catalog;
    ViewerPreferences *preferences;
    gint retval = 1;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), 1);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        preferences = catalog->getViewerPreferences();
        if (preferences) {
            retval = preferences->getNumCopies();
        }
    }

    return retval;
}

/**
 * poppler_document_get_print_page_ranges:
 * @document: A #PopplerDocument
 * @n_ranges: (out): return location for number of ranges
 *
 * Returns the suggested page ranges to print in the form of array
 * of #PopplerPageRange<!-- -->s and number of ranges.
 * %NULL pointer means that the document does not specify page ranges
 * for printing.
 *
 * Returns: (array length=n_ranges) (transfer full): an array
 *          of #PopplerPageRange<!-- -->s or %NULL. Free the array when
 *          it is no longer needed.
 *
 * Since: 0.80
 **/
PopplerPageRange *poppler_document_get_print_page_ranges(PopplerDocument *document, int *n_ranges)
{
    Catalog *catalog;
    ViewerPreferences *preferences;
    std::vector<std::pair<int, int>> ranges;
    PopplerPageRange *result = nullptr;

    g_return_val_if_fail(n_ranges != nullptr, nullptr);
    *n_ranges = 0;
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        preferences = catalog->getViewerPreferences();
        if (preferences) {
            ranges = preferences->getPrintPageRange();

            *n_ranges = ranges.size();
            result = g_new(PopplerPageRange, ranges.size());
            for (size_t i = 0; i < ranges.size(); ++i) {
                result[i].start_page = ranges[i].first;
                result[i].end_page = ranges[i].second;
            }
        }
    }

    return result;
}

/**
 * poppler_document_get_permissions:
 * @document: A #PopplerDocument
 *
 * Returns the flags specifying which operations are permitted when the document is opened.
 *
 * Return value: a set of flags from  #PopplerPermissions enumeration
 *
 * Since: 0.16
 **/
PopplerPermissions poppler_document_get_permissions(PopplerDocument *document)
{
    guint flag = 0;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PERMISSIONS_FULL);

    if (document->doc->okToPrint()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_PRINT;
    }
    if (document->doc->okToChange()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_MODIFY;
    }
    if (document->doc->okToCopy()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_COPY;
    }
    if (document->doc->okToAddNotes()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_ADD_NOTES;
    }
    if (document->doc->okToFillForm()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_FILL_FORM;
    }
    if (document->doc->okToAccessibility()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_EXTRACT_CONTENTS;
    }
    if (document->doc->okToAssemble()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_ASSEMBLE;
    }
    if (document->doc->okToPrintHighRes()) {
        flag |= POPPLER_PERMISSIONS_OK_TO_PRINT_HIGH_RESOLUTION;
    }

    return (PopplerPermissions)flag;
}

/**
 * poppler_document_get_pdf_subtype_string:
 * @document: A #PopplerDocument
 *
 * Returns the PDF subtype version of @document as a string.
 *
 * Returns: (transfer full) (nullable): a newly allocated string containing
 * the PDF subtype version of @document, or %NULL
 *
 * Since: 0.70
 **/
gchar *poppler_document_get_pdf_subtype_string(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    std::unique_ptr<GooString> infostring;

    switch (document->doc->getPDFSubtype()) {
    case subtypePDFA:
        infostring = document->doc->getDocInfoStringEntry("GTS_PDFA1Version");
        break;
    case subtypePDFE:
        infostring = document->doc->getDocInfoStringEntry("GTS_PDFEVersion");
        break;
    case subtypePDFUA:
        infostring = document->doc->getDocInfoStringEntry("GTS_PDFUAVersion");
        break;
    case subtypePDFVT:
        infostring = document->doc->getDocInfoStringEntry("GTS_PDFVTVersion");
        break;
    case subtypePDFX:
        infostring = document->doc->getDocInfoStringEntry("GTS_PDFXVersion");
        break;
    case subtypeNone:
    case subtypeNull:
    default: {
        /* nothing */
    }
    }

    return _poppler_goo_string_to_utf8(infostring.get());
}

/**
 * poppler_document_get_pdf_subtype:
 * @document: A #PopplerDocument
 *
 * Returns the subtype of @document as a #PopplerPDFSubtype.
 *
 * Returns: the document's subtype
 *
 * Since: 0.70
 **/
PopplerPDFSubtype poppler_document_get_pdf_subtype(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PDF_SUBTYPE_NONE);

    return convert_pdf_subtype(document->doc->getPDFSubtype());
}

/**
 * poppler_document_get_pdf_part:
 * @document: A #PopplerDocument
 *
 * Returns the part of the conforming standard that the @document adheres to
 * as a #PopplerPDFSubtype.
 *
 * Returns: the document's subtype part
 *
 * Since: 0.70
 **/
PopplerPDFPart poppler_document_get_pdf_part(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PDF_SUBTYPE_PART_NONE);

    return convert_pdf_subtype_part(document->doc->getPDFSubtypePart());
}

/**
 * poppler_document_get_pdf_conformance:
 * @document: A #PopplerDocument
 *
 * Returns the conformance level of the @document as #PopplerPDFConformance.
 *
 * Returns: the document's subtype conformance level
 *
 * Since: 0.70
 **/
PopplerPDFConformance poppler_document_get_pdf_conformance(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), POPPLER_PDF_SUBTYPE_CONF_NONE);

    return convert_pdf_subtype_conformance(document->doc->getPDFSubtypeConformance());
}

/**
 * poppler_document_get_metadata:
 * @document: A #PopplerDocument
 *
 * Returns the XML metadata string of the document
 *
 * Return value: a new allocated string containing the XML
 *               metadata, or %NULL
 *
 * Since: 0.16
 **/
gchar *poppler_document_get_metadata(PopplerDocument *document)
{
    Catalog *catalog;
    gchar *retval = nullptr;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        std::unique_ptr<GooString> s = catalog->readMetadata();

        if (s) {
            retval = g_strdup(s->c_str());
        }
    }

    return retval;
}

/**
 * poppler_document_reset_form:
 * @document: A #PopplerDocument
 * @fields: (element-type utf8) (nullable): list of fields to reset
 * @exclude_fields: whether to reset all fields except those in @fields
 *
 * Resets the form fields specified by fields if exclude_fields is FALSE.
 * Resets all others if exclude_fields is TRUE.
 * All form fields are reset regardless of the exclude_fields flag
 * if fields is empty.
 *
 * Since: 0.90
 **/
void poppler_document_reset_form(PopplerDocument *document, GList *fields, gboolean exclude_fields)
{
    std::vector<std::string> list;
    Catalog *catalog;
    GList *iter;
    Form *form;

    g_return_if_fail(POPPLER_IS_DOCUMENT(document));

    catalog = document->doc->getCatalog();
    if (catalog && catalog->isOk()) {
        form = catalog->getForm();

        if (form) {
            for (iter = fields; iter != nullptr; iter = iter->next) {
                list.emplace_back((char *)iter->data);
            }

            form->reset(list, exclude_fields);
        }
    }
}

/**
 * poppler_document_has_javascript:
 * @document: A #PopplerDocument
 *
 * Returns whether @document has any javascript in it.
 *
 * Since: 0.90
 **/
gboolean poppler_document_has_javascript(PopplerDocument *document)
{
    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), FALSE);

    return document->doc->hasJavascript();
}

static void poppler_document_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    PopplerDocument *document = POPPLER_DOCUMENT(object);
    guint version;

    switch (prop_id) {
    case PROP_TITLE:
        g_value_take_string(value, poppler_document_get_title(document));
        break;
    case PROP_FORMAT:
        g_value_take_string(value, poppler_document_get_pdf_version_string(document));
        break;
    case PROP_FORMAT_MAJOR:
        poppler_document_get_pdf_version(document, &version, nullptr);
        g_value_set_uint(value, version);
        break;
    case PROP_FORMAT_MINOR:
        poppler_document_get_pdf_version(document, nullptr, &version);
        g_value_set_uint(value, version);
        break;
    case PROP_AUTHOR:
        g_value_take_string(value, poppler_document_get_author(document));
        break;
    case PROP_SUBJECT:
        g_value_take_string(value, poppler_document_get_subject(document));
        break;
    case PROP_KEYWORDS:
        g_value_take_string(value, poppler_document_get_keywords(document));
        break;
    case PROP_CREATOR:
        g_value_take_string(value, poppler_document_get_creator(document));
        break;
    case PROP_PRODUCER:
        g_value_take_string(value, poppler_document_get_producer(document));
        break;
    case PROP_CREATION_DATE:
        g_value_set_int(value, poppler_document_get_creation_date(document));
        break;
    case PROP_CREATION_DATETIME:
        g_value_take_boxed(value, poppler_document_get_creation_date_time(document));
        break;
    case PROP_MOD_DATE:
        g_value_set_int(value, poppler_document_get_modification_date(document));
        break;
    case PROP_MOD_DATETIME:
        g_value_take_boxed(value, poppler_document_get_modification_date_time(document));
        break;
    case PROP_LINEARIZED:
        g_value_set_boolean(value, poppler_document_is_linearized(document));
        break;
    case PROP_PAGE_LAYOUT:
        g_value_set_enum(value, poppler_document_get_page_layout(document));
        break;
    case PROP_PAGE_MODE:
        g_value_set_enum(value, poppler_document_get_page_mode(document));
        break;
    case PROP_VIEWER_PREFERENCES:
        /* FIXME: write... */
        g_value_set_flags(value, POPPLER_VIEWER_PREFERENCES_UNSET);
        break;
    case PROP_PRINT_SCALING:
        g_value_set_enum(value, poppler_document_get_print_scaling(document));
        break;
    case PROP_PRINT_DUPLEX:
        g_value_set_enum(value, poppler_document_get_print_duplex(document));
        break;
    case PROP_PRINT_N_COPIES:
        g_value_set_int(value, poppler_document_get_print_n_copies(document));
        break;
    case PROP_PERMISSIONS:
        g_value_set_flags(value, poppler_document_get_permissions(document));
        break;
    case PROP_SUBTYPE:
        g_value_set_enum(value, poppler_document_get_pdf_subtype(document));
        break;
    case PROP_SUBTYPE_STRING:
        g_value_take_string(value, poppler_document_get_pdf_subtype_string(document));
        break;
    case PROP_SUBTYPE_PART:
        g_value_set_enum(value, poppler_document_get_pdf_part(document));
        break;
    case PROP_SUBTYPE_CONF:
        g_value_set_enum(value, poppler_document_get_pdf_conformance(document));
        break;
    case PROP_METADATA:
        g_value_take_string(value, poppler_document_get_metadata(document));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void poppler_document_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    PopplerDocument *document = POPPLER_DOCUMENT(object);

    switch (prop_id) {
    case PROP_TITLE:
        poppler_document_set_title(document, g_value_get_string(value));
        break;
    case PROP_AUTHOR:
        poppler_document_set_author(document, g_value_get_string(value));
        break;
    case PROP_SUBJECT:
        poppler_document_set_subject(document, g_value_get_string(value));
        break;
    case PROP_KEYWORDS:
        poppler_document_set_keywords(document, g_value_get_string(value));
        break;
    case PROP_CREATOR:
        poppler_document_set_creator(document, g_value_get_string(value));
        break;
    case PROP_PRODUCER:
        poppler_document_set_producer(document, g_value_get_string(value));
        break;
    case PROP_CREATION_DATE:
        poppler_document_set_creation_date(document, g_value_get_int(value));
        break;
    case PROP_CREATION_DATETIME:
        poppler_document_set_creation_date_time(document, (GDateTime *)g_value_get_boxed(value));
        break;
    case PROP_MOD_DATE:
        poppler_document_set_modification_date(document, g_value_get_int(value));
        break;
    case PROP_MOD_DATETIME:
        poppler_document_set_modification_date_time(document, (GDateTime *)g_value_get_boxed(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void poppler_document_class_init(PopplerDocumentClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_document_finalize;
    gobject_class->get_property = poppler_document_get_property;
    gobject_class->set_property = poppler_document_set_property;

    /**
     * PopplerDocument:title:
     *
     * The document's title or %NULL
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_TITLE, g_param_spec_string("title", "Document Title", "The title of the document", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:format:
     *
     * The PDF version as string. See also poppler_document_get_pdf_version_string()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_FORMAT, g_param_spec_string("format", "PDF Format", "The PDF version of the document", nullptr, G_PARAM_READABLE));

    /**
     * PopplerDocument:format-major:
     *
     * The PDF major version number. See also poppler_document_get_pdf_version()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_FORMAT_MAJOR, g_param_spec_uint("format-major", "PDF Format Major", "The PDF major version number of the document", 0, G_MAXUINT, 1, G_PARAM_READABLE));

    /**
     * PopplerDocument:format-minor:
     *
     * The PDF minor version number. See also poppler_document_get_pdf_version()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_FORMAT_MINOR, g_param_spec_uint("format-minor", "PDF Format Minor", "The PDF minor version number of the document", 0, G_MAXUINT, 0, G_PARAM_READABLE));

    /**
     * PopplerDocument:author:
     *
     * The author of the document
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_AUTHOR, g_param_spec_string("author", "Author", "The author of the document", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:subject:
     *
     * The subject of the document
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SUBJECT, g_param_spec_string("subject", "Subject", "Subjects the document touches", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:keywords:
     *
     * The keywords associated to the document
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_KEYWORDS, g_param_spec_string("keywords", "Keywords", "Keywords", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:creator:
     *
     * The creator of the document. See also poppler_document_get_creator()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_CREATOR, g_param_spec_string("creator", "Creator", "The software that created the document", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:producer:
     *
     * The producer of the document. See also poppler_document_get_producer()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PRODUCER, g_param_spec_string("producer", "Producer", "The software that converted the document", nullptr, G_PARAM_READWRITE));

    /**
     * PopplerDocument:creation-date:
     *
     * The date the document was created as seconds since the Epoch, or -1
     *
     * Deprecated: 20.09.0: This will overflow in 2038. Use creation-datetime
     * instead.
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_CREATION_DATE,
                                    g_param_spec_int("creation-date", "Creation Date", "The date and time the document was created", -1, G_MAXINT, -1, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_DEPRECATED)));

    /**
     * PopplerDocument:creation-datetime:
     * The #GDateTime the document was created.
     *
     * Since: 20.09.0
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_CREATION_DATETIME, g_param_spec_boxed("creation-datetime", "Creation DateTime", "The date and time the document was created", G_TYPE_DATE_TIME, G_PARAM_READWRITE));

    /**
     * PopplerDocument:mod-date:
     *
     * The date the document was most recently modified as seconds since the Epoch, or -1
     *
     * Deprecated: 20.09.0: This will overflow in 2038. Use mod-datetime instead.
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_MOD_DATE,
                                    g_param_spec_int("mod-date", "Modification Date", "The date and time the document was modified", -1, G_MAXINT, -1, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_DEPRECATED)));

    /**
     * PopplerDocument:mod-datetime:
     *
     * The #GDateTime the document was most recently modified.
     *
     * Since: 20.09.0
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_MOD_DATETIME, g_param_spec_boxed("mod-datetime", "Modification DateTime", "The date and time the document was modified", G_TYPE_DATE_TIME, G_PARAM_READWRITE));

    /**
     * PopplerDocument:linearized:
     *
     * Whether document is linearized. See also poppler_document_is_linearized()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_LINEARIZED, g_param_spec_boolean("linearized", "Fast Web View Enabled", "Is the document optimized for web viewing?", FALSE, G_PARAM_READABLE));

    /**
     * PopplerDocument:page-layout:
     *
     * The page layout that should be used when the document is opened
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PAGE_LAYOUT, g_param_spec_enum("page-layout", "Page Layout", "Initial Page Layout", POPPLER_TYPE_PAGE_LAYOUT, POPPLER_PAGE_LAYOUT_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:page-mode:
     *
     * The mode that should be used when the document is opened
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PAGE_MODE, g_param_spec_enum("page-mode", "Page Mode", "Page Mode", POPPLER_TYPE_PAGE_MODE, POPPLER_PAGE_MODE_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:viewer-preferences:
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_VIEWER_PREFERENCES,
                                    g_param_spec_flags("viewer-preferences", "Viewer Preferences", "Viewer Preferences", POPPLER_TYPE_VIEWER_PREFERENCES, POPPLER_VIEWER_PREFERENCES_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:print-scaling:
     *
     * Since: 0.73
     */
    g_object_class_install_property(
            G_OBJECT_CLASS(klass), PROP_PRINT_SCALING,
            g_param_spec_enum("print-scaling", "Print Scaling", "Print Scaling Viewer Preference", POPPLER_TYPE_PRINT_SCALING, POPPLER_PRINT_SCALING_APP_DEFAULT, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    /**
     * PopplerDocument:print-duplex:
     *
     * Since: 0.80
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PRINT_DUPLEX,
                                    g_param_spec_enum("print-duplex", "Print Duplex", "Duplex Viewer Preference", POPPLER_TYPE_PRINT_DUPLEX, POPPLER_PRINT_DUPLEX_NONE, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    /**
     * PopplerDocument:print-n-copies:
     *
     * Suggested number of copies to be printed for this document
     *
     * Since: 0.80
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PRINT_N_COPIES,
                                    g_param_spec_int("print-n-copies", "Number of Copies to Print", "Number of Copies Viewer Preference", 1, G_MAXINT, 1, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    /**
     * PopplerDocument:permissions:
     *
     * Flags specifying which operations are permitted when the document is opened
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_PERMISSIONS, g_param_spec_flags("permissions", "Permissions", "Permissions", POPPLER_TYPE_PERMISSIONS, POPPLER_PERMISSIONS_FULL, G_PARAM_READABLE));

    /**
     * PopplerDocument:subtype:
     *
     *  Document PDF subtype type
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SUBTYPE, g_param_spec_enum("subtype", "PDF Format Subtype Type", "The PDF subtype of the document", POPPLER_TYPE_PDF_SUBTYPE, POPPLER_PDF_SUBTYPE_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:subtype-string:
     *
     *  Document PDF subtype. See also poppler_document_get_pdf_subtype_string()
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SUBTYPE_STRING, g_param_spec_string("subtype-string", "PDF Format Subtype", "The PDF subtype of the document", nullptr, G_PARAM_READABLE));

    /**
     * PopplerDocument:subtype-part:
     *
     *  Document PDF subtype part
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SUBTYPE_PART,
                                    g_param_spec_enum("subtype-part", "PDF Format Subtype Part", "The part of PDF conformance", POPPLER_TYPE_PDF_PART, POPPLER_PDF_SUBTYPE_PART_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:subtype-conformance:
     *
     *  Document PDF subtype conformance
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_SUBTYPE_CONF,
                                    g_param_spec_enum("subtype-conformance", "PDF Format Subtype Conformance", "The conformance level of PDF subtype", POPPLER_TYPE_PDF_CONFORMANCE, POPPLER_PDF_SUBTYPE_CONF_UNSET, G_PARAM_READABLE));

    /**
     * PopplerDocument:metadata:
     *
     * Document metadata in XML format, or %NULL
     */
    g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_METADATA, g_param_spec_string("metadata", "XML Metadata", "Embedded XML metadata", nullptr, G_PARAM_READABLE));
}

static void poppler_document_init(PopplerDocument *document) { }

/* PopplerIndexIter: For determining the index of a tree */
struct _PopplerIndexIter
{
    PopplerDocument *document;
    const std::vector<OutlineItem *> *items;
    int index;
};

G_DEFINE_BOXED_TYPE(PopplerIndexIter, poppler_index_iter, poppler_index_iter_copy, poppler_index_iter_free)

/**
 * poppler_index_iter_copy:
 * @iter: a #PopplerIndexIter
 *
 * Creates a new #PopplerIndexIter as a copy of @iter.  This must be freed with
 * poppler_index_iter_free().
 *
 * Return value: a new #PopplerIndexIter
 **/
PopplerIndexIter *poppler_index_iter_copy(PopplerIndexIter *iter)
{
    PopplerIndexIter *new_iter;

    g_return_val_if_fail(iter != nullptr, NULL);

    new_iter = g_slice_dup(PopplerIndexIter, iter);
    new_iter->document = (PopplerDocument *)g_object_ref(new_iter->document);

    return new_iter;
}

/**
 * poppler_index_iter_new:
 * @document: a #PopplerDocument
 *
 * Returns the root #PopplerIndexIter for @document, or %NULL.  This must be
 * freed with poppler_index_iter_free().
 *
 * Certain documents have an index associated with them.  This index can be used
 * to help the user navigate the document, and is similar to a table of
 * contents.  Each node in the index will contain a #PopplerAction that can be
 * displayed to the user &mdash; typically a #POPPLER_ACTION_GOTO_DEST or a
 * #POPPLER_ACTION_URI<!-- -->.
 *
 * Here is a simple example of some code that walks the full index:
 *
 * <informalexample><programlisting>
 * static void
 * walk_index (PopplerIndexIter *iter)
 * {
 *   do
 *     {
 *       /<!-- -->* Get the action and do something with it *<!-- -->/
 *       PopplerIndexIter *child = poppler_index_iter_get_child (iter);
 *       if (child)
 *         walk_index (child);
 *       poppler_index_iter_free (child);
 *     }
 *   while (poppler_index_iter_next (iter));
 * }
 * ...
 * {
 *   iter = poppler_index_iter_new (document);
 *   walk_index (iter);
 *   poppler_index_iter_free (iter);
 * }
 *</programlisting></informalexample>
 *
 * Return value: a new #PopplerIndexIter
 **/
PopplerIndexIter *poppler_index_iter_new(PopplerDocument *document)
{
    PopplerIndexIter *iter;
    Outline *outline;
    const std::vector<OutlineItem *> *items;

    outline = document->doc->getOutline();
    if (outline == nullptr) {
        return nullptr;
    }

    items = outline->getItems();
    if (items == nullptr) {
        return nullptr;
    }

    iter = g_slice_new(PopplerIndexIter);
    iter->document = (PopplerDocument *)g_object_ref(document);
    iter->items = items;
    iter->index = 0;

    return iter;
}

/**
 * poppler_index_iter_get_child:
 * @parent: a #PopplerIndexIter
 *
 * Returns a newly created child of @parent, or %NULL if the iter has no child.
 * See poppler_index_iter_new() for more information on this function.
 *
 * Return value: a new #PopplerIndexIter
 **/
PopplerIndexIter *poppler_index_iter_get_child(PopplerIndexIter *parent)
{
    PopplerIndexIter *child;
    OutlineItem *item;

    g_return_val_if_fail(parent != nullptr, NULL);

    item = (*parent->items)[parent->index];
    item->open();
    if (!(item->hasKids() && item->getKids())) {
        return nullptr;
    }

    child = g_slice_new0(PopplerIndexIter);
    child->document = (PopplerDocument *)g_object_ref(parent->document);
    child->items = item->getKids();

    g_assert(child->items);

    return child;
}

static gchar *unicode_to_char(const Unicode *unicode, int len)
{
    const UnicodeMap *uMap = globalParams->getUtf8Map();

    GooString gstr;
    gchar buf[8]; /* 8 is enough for mapping an unicode char to a string */
    int i, n;

    for (i = 0; i < len; ++i) {
        n = uMap->mapUnicode(unicode[i], buf, sizeof(buf));
        gstr.append(buf, n);
    }

    return g_strdup(gstr.c_str());
}

/**
 * poppler_index_iter_is_open:
 * @iter: a #PopplerIndexIter
 *
 * Returns whether this node should be expanded by default to the user.  The
 * document can provide a hint as to how the document's index should be expanded
 * initially.
 *
 * Return value: %TRUE, if the document wants @iter to be expanded
 **/
gboolean poppler_index_iter_is_open(PopplerIndexIter *iter)
{
    OutlineItem *item;

    item = (*iter->items)[iter->index];

    return item->isOpen();
}

/**
 * poppler_index_iter_get_action:
 * @iter: a #PopplerIndexIter
 *
 * Returns the #PopplerAction associated with @iter.  It must be freed with
 * poppler_action_free().
 *
 * Return value: a new #PopplerAction
 **/
PopplerAction *poppler_index_iter_get_action(PopplerIndexIter *iter)
{
    OutlineItem *item;
    const LinkAction *link_action;
    PopplerAction *action;
    gchar *title;

    g_return_val_if_fail(iter != nullptr, NULL);

    item = (*iter->items)[iter->index];
    link_action = item->getAction();

    const std::vector<Unicode> &itemTitle = item->getTitle();
    title = unicode_to_char(itemTitle.data(), itemTitle.size());

    action = _poppler_action_new(iter->document, link_action, title);
    g_free(title);

    return action;
}

/**
 * poppler_index_iter_next:
 * @iter: a #PopplerIndexIter
 *
 * Sets @iter to point to the next action at the current level, if valid.  See
 * poppler_index_iter_new() for more information.
 *
 * Return value: %TRUE, if @iter was set to the next action
 **/
gboolean poppler_index_iter_next(PopplerIndexIter *iter)
{
    g_return_val_if_fail(iter != nullptr, FALSE);

    iter->index++;
    if (iter->index >= (int)iter->items->size()) {
        return FALSE;
    }

    return TRUE;
}

/**
 * poppler_index_iter_free:
 * @iter: a #PopplerIndexIter
 *
 * Frees @iter.
 **/
void poppler_index_iter_free(PopplerIndexIter *iter)
{
    if (G_UNLIKELY(iter == nullptr)) {
        return;
    }

    g_object_unref(iter->document);
    g_slice_free(PopplerIndexIter, iter);
}

struct _PopplerFontsIter
{
    std::vector<FontInfo *> items;
    int index;
};

G_DEFINE_BOXED_TYPE(PopplerFontsIter, poppler_fonts_iter, poppler_fonts_iter_copy, poppler_fonts_iter_free)

/**
 * poppler_fonts_iter_get_full_name:
 * @iter: a #PopplerFontsIter
 *
 * Returns the full name of the font associated with @iter
 *
 * Returns: the font full name
 */
const char *poppler_fonts_iter_get_full_name(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    const std::optional<std::string> &name = info->getName();
    if (name) {
        return name->c_str();
    } else {
        return nullptr;
    }
}

/**
 * poppler_fonts_iter_get_name:
 * @iter: a #PopplerFontsIter
 *
 * Returns the name of the font associated with @iter
 *
 * Returns: the font name
 */
const char *poppler_fonts_iter_get_name(PopplerFontsIter *iter)
{
    FontInfo *info;
    const char *name;

    name = poppler_fonts_iter_get_full_name(iter);
    info = iter->items[iter->index];

    if (info->getSubset() && name) {
        while (*name && *name != '+') {
            name++;
        }

        if (*name) {
            name++;
        }
    }

    return name;
}

/**
 * poppler_fonts_iter_get_substitute_name:
 * @iter: a #PopplerFontsIter
 *
 * The name of the substitute font of the font associated with @iter or %NULL if
 * the font is embedded
 *
 * Returns: the name of the substitute font or %NULL if font is embedded
 *
 * Since: 0.20
 */
const char *poppler_fonts_iter_get_substitute_name(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    const std::optional<std::string> &name = info->getSubstituteName();
    if (name) {
        return name->c_str();
    } else {
        return nullptr;
    }
}

/**
 * poppler_fonts_iter_get_file_name:
 * @iter: a #PopplerFontsIter
 *
 * The filename of the font associated with @iter or %NULL if
 * the font is embedded
 *
 * Returns: the filename of the font or %NULL if font is embedded
 */
const char *poppler_fonts_iter_get_file_name(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    const std::optional<std::string> &file = info->getFile();
    if (file) {
        return file->c_str();
    } else {
        return nullptr;
    }
}

/**
 * poppler_fonts_iter_get_font_type:
 * @iter: a #PopplerFontsIter
 *
 * Returns the type of the font associated with @iter
 *
 * Returns: the font type
 */
PopplerFontType poppler_fonts_iter_get_font_type(PopplerFontsIter *iter)
{
    FontInfo *info;

    g_return_val_if_fail(iter != nullptr, POPPLER_FONT_TYPE_UNKNOWN);

    info = iter->items[iter->index];

    return (PopplerFontType)info->getType();
}

/**
 * poppler_fonts_iter_get_encoding:
 * @iter: a #PopplerFontsIter
 *
 * Returns the encoding of the font associated with @iter
 *
 * Returns: the font encoding
 *
 * Since: 0.20
 */
const char *poppler_fonts_iter_get_encoding(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    const std::string &encoding = info->getEncoding();
    if (!encoding.empty()) {
        return encoding.c_str();
    } else {
        return nullptr;
    }
}

/**
 * poppler_fonts_iter_is_embedded:
 * @iter: a #PopplerFontsIter
 *
 * Returns whether the font associated with @iter is embedded in the document
 *
 * Returns: %TRUE if font is embedded, %FALSE otherwise
 */
gboolean poppler_fonts_iter_is_embedded(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    return info->getEmbedded();
}

/**
 * poppler_fonts_iter_is_subset:
 * @iter: a #PopplerFontsIter
 *
 * Returns whether the font associated with @iter is a subset of another font
 *
 * Returns: %TRUE if font is a subset, %FALSE otherwise
 */
gboolean poppler_fonts_iter_is_subset(PopplerFontsIter *iter)
{
    FontInfo *info;

    info = iter->items[iter->index];

    return info->getSubset();
}

/**
 * poppler_fonts_iter_next:
 * @iter: a #PopplerFontsIter
 *
 * Sets @iter to point to the next font
 *
 * Returns: %TRUE, if @iter was set to the next font
 **/
gboolean poppler_fonts_iter_next(PopplerFontsIter *iter)
{
    g_return_val_if_fail(iter != nullptr, FALSE);

    iter->index++;
    if (iter->index >= (int)iter->items.size()) {
        return FALSE;
    }

    return TRUE;
}

/**
 * poppler_fonts_iter_copy:
 * @iter: a #PopplerFontsIter to copy
 *
 * Creates a copy of @iter
 *
 * Returns: a new allocated copy of @iter
 */
PopplerFontsIter *poppler_fonts_iter_copy(PopplerFontsIter *iter)
{
    PopplerFontsIter *new_iter;

    g_return_val_if_fail(iter != nullptr, NULL);

    new_iter = g_slice_dup(PopplerFontsIter, iter);

    new_iter->items.resize(iter->items.size());
    for (std::size_t i = 0; i < iter->items.size(); i++) {
        FontInfo *info = iter->items[i];
        new_iter->items[i] = new FontInfo(*info);
    }

    return new_iter;
}

/**
 * poppler_fonts_iter_free:
 * @iter: a #PopplerFontsIter
 *
 * Frees the given #PopplerFontsIter
 */
void poppler_fonts_iter_free(PopplerFontsIter *iter)
{
    if (G_UNLIKELY(iter == nullptr)) {
        return;
    }

    for (auto entry : iter->items) {
        delete entry;
    }
    iter->items.~vector<FontInfo *>();

    g_slice_free(PopplerFontsIter, iter);
}

static PopplerFontsIter *poppler_fonts_iter_new(std::vector<FontInfo *> &&items)
{
    PopplerFontsIter *iter;

    iter = g_slice_new(PopplerFontsIter);
    new ((void *)&iter->items) std::vector<FontInfo *>(std::move(items));
    iter->index = 0;

    return iter;
}

typedef struct _PopplerFontInfoClass PopplerFontInfoClass;
struct _PopplerFontInfoClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE(PopplerFontInfo, poppler_font_info, G_TYPE_OBJECT)

static void poppler_font_info_finalize(GObject *object);

static void poppler_font_info_class_init(PopplerFontInfoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_font_info_finalize;
}

static void poppler_font_info_init(PopplerFontInfo *font_info)
{
    font_info->document = nullptr;
    font_info->scanner = nullptr;
}

static void poppler_font_info_finalize(GObject *object)
{
    PopplerFontInfo *font_info = POPPLER_FONT_INFO(object);

    delete font_info->scanner;
    g_object_unref(font_info->document);

    G_OBJECT_CLASS(poppler_font_info_parent_class)->finalize(object);
}

/**
 * poppler_font_info_new:
 * @document: a #PopplerDocument
 *
 * Creates a new #PopplerFontInfo object
 *
 * Returns: a new #PopplerFontInfo instance
 */
PopplerFontInfo *poppler_font_info_new(PopplerDocument *document)
{
    PopplerFontInfo *font_info;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);

    font_info = (PopplerFontInfo *)g_object_new(POPPLER_TYPE_FONT_INFO, nullptr);
    font_info->document = (PopplerDocument *)g_object_ref(document);
    font_info->scanner = new FontInfoScanner(document->doc);

    return font_info;
}

/**
 * poppler_font_info_scan:
 * @font_info: a #PopplerFontInfo
 * @n_pages: number of pages to scan
 * @iter: (out): return location for a #PopplerFontsIter
 *
 * Scans the document associated with @font_info for fonts. At most
 * @n_pages will be scanned starting from the current iterator. @iter will
 * point to the first font scanned.
 *
 * Here is a simple example of code to scan fonts in a document
 *
 * <informalexample><programlisting>
 * font_info = poppler_font_info_new (document);
 * scanned_pages = 0;
 * while (scanned_pages <= poppler_document_get_n_pages(document)) {
 *         poppler_font_info_scan (font_info, 20, &fonts_iter);
 *         scanned_pages += 20;
 *         if (!fonts_iter)
 *                 continue; /<!-- -->* No fonts found in these 20 pages *<!-- -->/
 *         do {
 *                 /<!-- -->* Do something with font iter *<!-- -->/
 *                 g_print ("Font Name: %s\n", poppler_fonts_iter_get_name (fonts_iter));
 *         } while (poppler_fonts_iter_next (fonts_iter));
 *         poppler_fonts_iter_free (fonts_iter);
 * }
 * </programlisting></informalexample>
 *
 * Returns: %TRUE, if fonts were found
 */
gboolean poppler_font_info_scan(PopplerFontInfo *font_info, int n_pages, PopplerFontsIter **iter)
{
    g_return_val_if_fail(iter != nullptr, FALSE);

    std::vector<FontInfo *> items = font_info->scanner->scan(n_pages);

    if (items.empty()) {
        *iter = nullptr;
        return FALSE;
    } else {
        *iter = poppler_fonts_iter_new(std::move(items));
    }

    return TRUE;
}

/* For backward compatibility */
void poppler_font_info_free(PopplerFontInfo *font_info)
{
    g_return_if_fail(font_info != nullptr);

    g_object_unref(font_info);
}

/* Optional content (layers) */
static Layer *layer_new(OptionalContentGroup *oc)
{
    Layer *layer;

    layer = g_slice_new0(Layer);
    layer->oc = oc;

    return layer;
}

static void layer_free(Layer *layer)
{
    if (G_UNLIKELY(!layer)) {
        return;
    }

    if (layer->kids) {
        g_list_free_full(layer->kids, (GDestroyNotify)layer_free);
    }

    if (layer->label) {
        g_free(layer->label);
    }

    g_slice_free(Layer, layer);
}

static GList *get_optional_content_rbgroups(OCGs *ocg)
{
    Array *rb;
    GList *groups = nullptr;

    rb = ocg->getRBGroupsArray();

    if (rb) {
        int i, j;

        for (i = 0; i < rb->getLength(); ++i) {
            Array *rb_array;
            GList *group = nullptr;

            Object obj = rb->get(i);
            if (!obj.isArray()) {
                continue;
            }

            rb_array = obj.getArray();
            for (j = 0; j < rb_array->getLength(); ++j) {
                OptionalContentGroup *oc;

                const Object &ref = rb_array->getNF(j);
                if (!ref.isRef()) {
                    continue;
                }

                oc = ocg->findOcgByRef(ref.getRef());
                group = g_list_prepend(group, oc);
            }

            groups = g_list_prepend(groups, group);
        }
    }

    return groups;
}

GList *_poppler_document_get_layer_rbgroup(PopplerDocument *document, Layer *layer)
{
    GList *l;

    for (l = document->layers_rbgroups; l && l->data; l = g_list_next(l)) {
        GList *group = (GList *)l->data;

        if (g_list_find(group, layer->oc)) {
            return group;
        }
    }

    return nullptr;
}

static GList *get_optional_content_items_sorted(OCGs *ocg, Layer *parent, Array *order)
{
    GList *items = nullptr;
    Layer *last_item = parent;
    int i;

    for (i = 0; i < order->getLength(); ++i) {
        Object orderItem = order->get(i);

        if (orderItem.isDict()) {
            const Object &ref = order->getNF(i);
            if (ref.isRef()) {
                OptionalContentGroup *oc = ocg->findOcgByRef(ref.getRef());
                Layer *layer = layer_new(oc);

                items = g_list_prepend(items, layer);
                last_item = layer;
            }
        } else if (orderItem.isArray() && orderItem.arrayGetLength() > 0) {
            if (!last_item) {
                last_item = layer_new(nullptr);
                items = g_list_prepend(items, last_item);
            }
            last_item->kids = get_optional_content_items_sorted(ocg, last_item, orderItem.getArray());
            last_item = nullptr;
        } else if (orderItem.isString()) {
            last_item->label = _poppler_goo_string_to_utf8(orderItem.getString());
        }
    }

    return g_list_reverse(items);
}

static GList *get_optional_content_items(OCGs *ocg)
{
    Array *order;
    GList *items = nullptr;

    order = ocg->getOrderArray();

    if (order) {
        items = get_optional_content_items_sorted(ocg, nullptr, order);
    } else {
        const auto &ocgs = ocg->getOCGs();

        for (const auto &oc : ocgs) {
            Layer *layer = layer_new(oc.second.get());

            items = g_list_prepend(items, layer);
        }

        items = g_list_reverse(items);
    }

    return items;
}

GList *_poppler_document_get_layers(PopplerDocument *document)
{
    if (!document->layers) {
        Catalog *catalog = document->doc->getCatalog();
        OCGs *ocg = catalog->getOptContentConfig();

        if (!ocg) {
            return nullptr;
        }

        document->layers = get_optional_content_items(ocg);
        document->layers_rbgroups = get_optional_content_rbgroups(ocg);
    }

    return document->layers;
}

static void poppler_document_layers_free(PopplerDocument *document)
{
    if (G_UNLIKELY(!document->layers)) {
        return;
    }

    g_list_free_full(document->layers, (GDestroyNotify)layer_free);
    g_list_free_full(document->layers_rbgroups, (GDestroyNotify)g_list_free);

    document->layers = nullptr;
    document->layers_rbgroups = nullptr;
}

/* PopplerLayersIter */
struct _PopplerLayersIter
{
    PopplerDocument *document;
    GList *items;
    int index;
};

G_DEFINE_BOXED_TYPE(PopplerLayersIter, poppler_layers_iter, poppler_layers_iter_copy, poppler_layers_iter_free)

/**
 * poppler_layers_iter_copy:
 * @iter: a #PopplerLayersIter
 *
 * Creates a new #PopplerLayersIter as a copy of @iter.  This must be freed with
 * poppler_layers_iter_free().
 *
 * Return value: a new #PopplerLayersIter
 *
 * Since 0.12
 **/
PopplerLayersIter *poppler_layers_iter_copy(PopplerLayersIter *iter)
{
    PopplerLayersIter *new_iter;

    g_return_val_if_fail(iter != nullptr, NULL);

    new_iter = g_slice_dup(PopplerLayersIter, iter);
    new_iter->document = (PopplerDocument *)g_object_ref(new_iter->document);

    return new_iter;
}

/**
 * poppler_layers_iter_free:
 * @iter: a #PopplerLayersIter
 *
 * Frees @iter.
 *
 * Since: 0.12
 **/
void poppler_layers_iter_free(PopplerLayersIter *iter)
{
    if (G_UNLIKELY(iter == nullptr)) {
        return;
    }

    g_object_unref(iter->document);
    g_slice_free(PopplerLayersIter, iter);
}

/**
 * poppler_layers_iter_new:
 * @document: a #PopplerDocument
 *
 * Since: 0.12
 **/
PopplerLayersIter *poppler_layers_iter_new(PopplerDocument *document)
{
    PopplerLayersIter *iter;
    GList *items;

    items = _poppler_document_get_layers(document);

    if (!items) {
        return nullptr;
    }

    iter = g_slice_new0(PopplerLayersIter);
    iter->document = (PopplerDocument *)g_object_ref(document);
    iter->items = items;

    return iter;
}

/**
 * poppler_layers_iter_get_child:
 * @parent: a #PopplerLayersIter
 *
 * Returns a newly created child of @parent, or %NULL if the iter has no child.
 * See poppler_layers_iter_new() for more information on this function.
 *
 * Return value: a new #PopplerLayersIter, or %NULL
 *
 * Since: 0.12
 **/
PopplerLayersIter *poppler_layers_iter_get_child(PopplerLayersIter *parent)
{
    PopplerLayersIter *child;
    Layer *layer;

    g_return_val_if_fail(parent != nullptr, NULL);

    layer = (Layer *)g_list_nth_data(parent->items, parent->index);
    if (!layer || !layer->kids) {
        return nullptr;
    }

    child = g_slice_new0(PopplerLayersIter);
    child->document = (PopplerDocument *)g_object_ref(parent->document);
    child->items = layer->kids;

    g_assert(child->items);

    return child;
}

/**
 * poppler_layers_iter_get_title:
 * @iter: a #PopplerLayersIter
 *
 * Returns the title associated with @iter.  It must be freed with
 * g_free().
 *
 * Return value: a new string containing the @iter's title or %NULL if @iter doesn't have a title.
 * The returned string should be freed with g_free() when no longer needed.
 *
 * Since: 0.12
 **/
gchar *poppler_layers_iter_get_title(PopplerLayersIter *iter)
{
    Layer *layer;

    g_return_val_if_fail(iter != nullptr, NULL);

    layer = (Layer *)g_list_nth_data(iter->items, iter->index);

    return layer->label ? g_strdup(layer->label) : nullptr;
}

/**
 * poppler_layers_iter_get_layer:
 * @iter: a #PopplerLayersIter
 *
 * Returns the #PopplerLayer associated with @iter.
 *
 * Return value: (transfer full): a new #PopplerLayer, or %NULL if
 * there isn't any layer associated with @iter
 *
 * Since: 0.12
 **/
PopplerLayer *poppler_layers_iter_get_layer(PopplerLayersIter *iter)
{
    Layer *layer;
    PopplerLayer *poppler_layer = nullptr;

    g_return_val_if_fail(iter != nullptr, NULL);

    layer = (Layer *)g_list_nth_data(iter->items, iter->index);
    if (layer->oc) {
        GList *rb_group = nullptr;

        rb_group = _poppler_document_get_layer_rbgroup(iter->document, layer);
        poppler_layer = _poppler_layer_new(iter->document, layer, rb_group);
    }

    return poppler_layer;
}

/**
 * poppler_layers_iter_next:
 * @iter: a #PopplerLayersIter
 *
 * Sets @iter to point to the next action at the current level, if valid.  See
 * poppler_layers_iter_new() for more information.
 *
 * Return value: %TRUE, if @iter was set to the next action
 *
 * Since: 0.12
 **/
gboolean poppler_layers_iter_next(PopplerLayersIter *iter)
{
    g_return_val_if_fail(iter != nullptr, FALSE);

    iter->index++;
    if (iter->index >= (gint)g_list_length(iter->items)) {
        return FALSE;
    }

    return TRUE;
}

typedef struct _PopplerPSFileClass PopplerPSFileClass;
struct _PopplerPSFileClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE(PopplerPSFile, poppler_ps_file, G_TYPE_OBJECT)

static void poppler_ps_file_finalize(GObject *object);

static void poppler_ps_file_class_init(PopplerPSFileClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = poppler_ps_file_finalize;
}

static void poppler_ps_file_init(PopplerPSFile *ps_file)
{
    ps_file->out = nullptr;
    ps_file->fd = -1;
    ps_file->filename = nullptr;
    ps_file->paper_width = -1;
    ps_file->paper_height = -1;
    ps_file->duplex = FALSE;
}

static void poppler_ps_file_finalize(GObject *object)
{
    PopplerPSFile *ps_file = POPPLER_PS_FILE(object);

    delete ps_file->out;
    g_object_unref(ps_file->document);
    g_free(ps_file->filename);
#ifndef G_OS_WIN32
    if (ps_file->fd != -1) {
        close(ps_file->fd);
    }
#endif /* !G_OS_WIN32 */

    G_OBJECT_CLASS(poppler_ps_file_parent_class)->finalize(object);
}

/**
 * poppler_ps_file_new:
 * @document: a #PopplerDocument
 * @filename: the path of the output filename
 * @first_page: the first page to print
 * @n_pages: the number of pages to print
 *
 * Create a new postscript file to render to
 *
 * Return value: (transfer full): a PopplerPSFile
 **/
PopplerPSFile *poppler_ps_file_new(PopplerDocument *document, const char *filename, int first_page, int n_pages)
{
    PopplerPSFile *ps_file;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), NULL);
    g_return_val_if_fail(filename != nullptr, NULL);
    g_return_val_if_fail(n_pages > 0, NULL);

    ps_file = (PopplerPSFile *)g_object_new(POPPLER_TYPE_PS_FILE, nullptr);
    ps_file->document = (PopplerDocument *)g_object_ref(document);
    ps_file->filename = g_strdup(filename);
    ps_file->first_page = first_page + 1;
    ps_file->last_page = first_page + 1 + n_pages - 1;

    return ps_file;
}

#ifndef G_OS_WIN32

/**
 * poppler_ps_file_new_fd:
 * @document: a #PopplerDocument
 * @fd: a valid file descriptor open for writing
 * @first_page: the first page to print
 * @n_pages: the number of pages to print
 *
 * Create a new postscript file to render to.
 * Note that this function takes ownership of @fd; you must not operate on it
 * again, nor close it.
 *
 * Return value: (transfer full): a #PopplerPSFile
 *
 * Since: 21.12.0
 **/
PopplerPSFile *poppler_ps_file_new_fd(PopplerDocument *document, int fd, int first_page, int n_pages)
{
    PopplerPSFile *ps_file;

    g_return_val_if_fail(POPPLER_IS_DOCUMENT(document), nullptr);
    g_return_val_if_fail(fd != -1, nullptr);
    g_return_val_if_fail(n_pages > 0, nullptr);

    ps_file = (PopplerPSFile *)g_object_new(POPPLER_TYPE_PS_FILE, nullptr);
    ps_file->document = (PopplerDocument *)g_object_ref(document);
    ps_file->fd = fd;
    ps_file->first_page = first_page + 1;
    ps_file->last_page = first_page + 1 + n_pages - 1;

    return ps_file;
}

#endif /* !G_OS_WIN32 */

/**
 * poppler_ps_file_set_paper_size:
 * @ps_file: a PopplerPSFile which was not yet printed to.
 * @width: the paper width in 1/72 inch
 * @height: the paper height in 1/72 inch
 *
 * Set the output paper size. These values will end up in the
 * DocumentMedia, the BoundingBox DSC comments and other places in the
 * generated PostScript.
 *
 **/
void poppler_ps_file_set_paper_size(PopplerPSFile *ps_file, double width, double height)
{
    g_return_if_fail(ps_file->out == nullptr);

    ps_file->paper_width = width;
    ps_file->paper_height = height;
}

/**
 * poppler_ps_file_set_duplex:
 * @ps_file: a PopplerPSFile which was not yet printed to
 * @duplex: whether to force duplex printing (on printers which support this)
 *
 * Enable or disable Duplex printing.
 *
 **/
void poppler_ps_file_set_duplex(PopplerPSFile *ps_file, gboolean duplex)
{
    g_return_if_fail(ps_file->out == nullptr);

    ps_file->duplex = duplex;
}

/**
 * poppler_ps_file_free:
 * @ps_file: a PopplerPSFile
 *
 * Frees @ps_file
 *
 **/
void poppler_ps_file_free(PopplerPSFile *ps_file)
{
    g_return_if_fail(ps_file != nullptr);
    g_object_unref(ps_file);
}

/**
 * poppler_document_get_form_field:
 * @document: a #PopplerDocument
 * @id: an id of a #PopplerFormField
 *
 * Returns the #PopplerFormField for the given @id. It must be freed with
 * g_object_unref()
 *
 * Return value: (transfer full): a new #PopplerFormField or %NULL if
 * not found
 **/
PopplerFormField *poppler_document_get_form_field(PopplerDocument *document, gint id)
{
    Page *page;
    unsigned pageNum;
    unsigned fieldNum;
    FormWidget *field;

    FormWidget::decodeID(id, &pageNum, &fieldNum);

    page = document->doc->getPage(pageNum);
    if (!page) {
        return nullptr;
    }

    const std::unique_ptr<FormPageWidgets> widgets = page->getFormWidgets();
    if (!widgets) {
        return nullptr;
    }

    field = widgets->getWidget(fieldNum);
    if (field) {
        return _poppler_form_field_new(document, field);
    }

    return nullptr;
}

gboolean _poppler_convert_pdf_date_to_gtime(const GooString *date, time_t *gdate)
{
    gchar *date_string;
    gboolean retval;

    if (hasUnicodeByteOrderMark(date->toStr())) {
        date_string = g_convert(date->c_str() + 2, date->getLength() - 2, "UTF-8", "UTF-16BE", nullptr, nullptr, nullptr);
    } else {
        date_string = g_strndup(date->c_str(), date->getLength());
    }

    retval = poppler_date_parse(date_string, gdate);
    g_free(date_string);

    return retval;
}

/**
 * _poppler_convert_pdf_date_to_date_time:
 * @date: a PDF date
 *
 * Converts the PDF date in @date to a #GDateTime.
 *
 * Returns: The converted date, or %NULL on error.
 **/
GDateTime *_poppler_convert_pdf_date_to_date_time(const GooString *date)
{
    GDateTime *date_time = nullptr;
    GTimeZone *time_zone = nullptr;
    int year, mon, day, hour, min, sec, tzHours, tzMins;
    char tz;

    if (parseDateString(date, &year, &mon, &day, &hour, &min, &sec, &tz, &tzHours, &tzMins)) {
        if (tz == '+' || tz == '-') {
            gchar *identifier;

            identifier = g_strdup_printf("%c%02u:%02u", tz, tzHours, tzMins);
#if GLIB_CHECK_VERSION(2, 68, 0)
            time_zone = g_time_zone_new_identifier(identifier);
            if (!time_zone) {
                g_debug("Failed to create time zone for identifier \"%s\"", identifier);
                time_zone = g_time_zone_new_utc();
            }
#else
            time_zone = g_time_zone_new(identifier);
#endif
            g_free(identifier);
        } else if (tz == '\0' || tz == 'Z') {
            time_zone = g_time_zone_new_utc();
        } else {
            g_warning("unexpected tz val '%c'", tz);
            time_zone = g_time_zone_new_utc();
        }

        date_time = g_date_time_new(time_zone, year, mon, day, hour, min, sec);
        g_time_zone_unref(time_zone);
    }

    return date_time;
}

/**
 * _poppler_convert_date_time_to_pdf_date:
 * @datetime: a #GDateTime
 *
 * Converts a #GDateTime to a PDF date.
 *
 * Returns: The converted date
 **/
GooString *_poppler_convert_date_time_to_pdf_date(GDateTime *datetime)
{
    int offset_min;
    gchar *date_str;
    std::string out_str;

    offset_min = g_date_time_get_utc_offset(datetime) / 1000000 / 60;
    date_str = g_date_time_format(datetime, "D:%Y%m%d%H%M%S");

    if (offset_min == 0) {
        out_str = GooString::format("{0:s}Z", date_str);
    } else {
        char tz = offset_min > 0 ? '+' : '-';

        out_str = GooString::format("{0:s}{1:c}{2:02d}'{3:02d}'", date_str, tz, offset_min / 60, offset_min % 60);
    }

    g_free(date_str);
    return new GooString(std::move(out_str));
}

static void _poppler_sign_document_thread(GTask *task, PopplerDocument *document, const PopplerSigningData *signing_data, GCancellable *cancellable)
{
    const PopplerCertificateInfo *certificate_info;
    const char *signing_data_signature_text;
    const PopplerColor *font_color;
    const PopplerColor *border_color;
    const PopplerColor *background_color;
    gboolean ret;

    g_return_if_fail(POPPLER_IS_DOCUMENT(document));
    g_return_if_fail(signing_data != nullptr);

    signing_data_signature_text = poppler_signing_data_get_signature_text(signing_data);
    if (signing_data_signature_text == nullptr) {
        g_task_return_new_error(task, POPPLER_ERROR, POPPLER_ERROR_SIGNING, "No signature given");
        return;
    }

    certificate_info = poppler_signing_data_get_certificate_info(signing_data);
    if (certificate_info == nullptr) {
        g_task_return_new_error(task, POPPLER_ERROR, POPPLER_ERROR_SIGNING, "Invalid certificate information provided for signing");
        return;
    }

    PopplerPage *page = poppler_document_get_page(document, poppler_signing_data_get_page(signing_data));
    if (page == nullptr) {
        g_task_return_new_error(task, POPPLER_ERROR, POPPLER_ERROR_SIGNING, "Invalid page number selected for signing");
        return;
    }

    font_color = poppler_signing_data_get_font_color(signing_data);
    border_color = poppler_signing_data_get_border_color(signing_data);
    background_color = poppler_signing_data_get_background_color(signing_data);

    std::unique_ptr<GooString> signature_text = std::make_unique<GooString>(utf8ToUtf16WithBom(signing_data_signature_text));
    std::unique_ptr<GooString> signature_text_left = std::make_unique<GooString>(utf8ToUtf16WithBom(poppler_signing_data_get_signature_text_left(signing_data)));
    const auto field_partial_name = new GooString(poppler_signing_data_get_field_partial_name(signing_data), strlen(poppler_signing_data_get_field_partial_name(signing_data)));
    const auto owner_pwd = std::optional<GooString>(poppler_signing_data_get_document_owner_password(signing_data));
    const auto user_pwd = std::optional<GooString>(poppler_signing_data_get_document_user_password(signing_data));
    const auto reason = std::unique_ptr<GooString>(poppler_signing_data_get_reason(signing_data) ? new GooString(poppler_signing_data_get_reason(signing_data), strlen(poppler_signing_data_get_reason(signing_data))) : nullptr);
    const auto location = std::unique_ptr<GooString>(poppler_signing_data_get_location(signing_data) ? new GooString(poppler_signing_data_get_location(signing_data), strlen(poppler_signing_data_get_location(signing_data))) : nullptr);
    const PopplerRectangle *rect = poppler_signing_data_get_signature_rectangle(signing_data);

    ret = document->doc->sign(poppler_signing_data_get_destination_filename(signing_data), poppler_certificate_info_get_id((PopplerCertificateInfo *)certificate_info),
                              poppler_signing_data_get_password(signing_data) ? poppler_signing_data_get_password(signing_data) : "", field_partial_name, poppler_signing_data_get_page(signing_data) + 1,
                              PDFRectangle(rect->x1, rect->y1, rect->x2, rect->y2), *signature_text, *signature_text_left, poppler_signing_data_get_font_size(signing_data), poppler_signing_data_get_left_font_size(signing_data),
                              std::make_unique<AnnotColor>(font_color->red, font_color->green, font_color->blue), poppler_signing_data_get_border_width(signing_data),
                              std::make_unique<AnnotColor>(border_color->red, border_color->green, border_color->blue), std::make_unique<AnnotColor>(background_color->red, background_color->green, background_color->blue), reason.get(),
                              location.get(), poppler_signing_data_get_image_path(signing_data) ? poppler_signing_data_get_image_path(signing_data) : "", owner_pwd, user_pwd);

    g_task_return_boolean(task, ret);
}

/**
 * poppler_document_sign:
 * @document: a #PopplerDocument
 * @signing_data: a #PopplerSigningData
 * @cancellable: a #GCancellable
 * @callback: a #GAsyncReadyCallback
 * @user_data: user data used by callback function
 *
 * Sign #document using #signing_data.
 *
 * Since: 23.07.0
 **/
void poppler_document_sign(PopplerDocument *document, const PopplerSigningData *signing_data, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    g_return_if_fail(POPPLER_IS_DOCUMENT(document));
    g_return_if_fail(signing_data != nullptr);

    task = g_task_new(document, cancellable, callback, user_data);
    g_task_set_task_data(task, (void *)signing_data, nullptr);

    g_task_run_in_thread(task, (GTaskThreadFunc)_poppler_sign_document_thread);
    g_object_unref(task);
}

/**
 * poppler_document_sign_finish:
 * @document: a #PopplerDocument
 * @result: a #GAsyncResult
 * @error: a #GError
 *
 * Finish poppler_sign_document and get return status or error.
 *
 * Returns: %TRUE on successful signing a document, otherwise %FALSE and error is set.
 *
 * Since: 23.07.0
 **/
gboolean poppler_document_sign_finish(PopplerDocument *document, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail(g_task_is_valid(result, document), FALSE);

    return g_task_propagate_boolean(G_TASK(result), error);
}
