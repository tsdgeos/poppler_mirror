/*
 * Copyright (C) 2009-2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
 * Copyright (C) 2017, 2022, 2025, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2018, 2020, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2019, Masamichi Hosoda <trueroad@trueroad.jp>
 * Copyright (C) 2019, 2020, Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Copyright (C) 2025 Nathanael d. Noblet <nathanael@noblet.ca>
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

/**
 \file poppler-document.h
 */
#include "poppler-destination.h"
#include "poppler-document.h"
#include "poppler-embedded-file.h"
#include "poppler-page.h"
#include "poppler-toc.h"

#include "poppler-destination-private.h"
#include "poppler-document-private.h"
#include "poppler-embedded-file-private.h"
#include "poppler-page-private.h"
#include "poppler-private.h"
#include "poppler-toc-private.h"

#include "Catalog.h"
#include "DateInfo.h"
#include "ErrorCodes.h"
#include "GlobalParams.h"
#include "Link.h"
#include "Outline.h"

#include <algorithm>
#include <iterator>
#include <memory>

using namespace poppler;

document_private::document_private(std::unique_ptr<GooString> &&file_path, const std::string &owner_password, const std::string &user_password) : document_private()
{
    doc = std::make_unique<PDFDoc>(std::move(file_path), GooString(owner_password.c_str()), GooString(user_password.c_str()));
}

document_private::document_private(byte_array *file_data, const std::string &owner_password, const std::string &user_password) : document_private()
{
    file_data->swap(doc_data);
    auto memstr = std::make_unique<MemStream>(doc_data.data(), 0, doc_data.size(), Object::null());
    doc = std::make_unique<PDFDoc>(std::move(memstr), GooString(owner_password.c_str()), GooString(user_password.c_str()));
}

document_private::document_private(const char *file_data, int file_data_length, const std::string &owner_password, const std::string &user_password) : document_private()
{
    raw_doc_data = file_data;
    raw_doc_data_length = file_data_length;
    auto memstr = std::make_unique<MemStream>(raw_doc_data, 0, raw_doc_data_length, Object::null());
    doc = std::make_unique<PDFDoc>(std::move(memstr), GooString(owner_password.c_str()), GooString(user_password.c_str()));
}

document_private::document_private() : GlobalParamsIniter(detail::error_function), doc(nullptr), raw_doc_data(nullptr), raw_doc_data_length(0), is_locked(false) { }

document_private::~document_private()
{
    delete_all(embedded_files);
}

document *document_private::check_document(document_private *doc, byte_array *file_data)
{
    if (doc->doc->isOk() || doc->doc->getErrorCode() == errEncrypted) {
        if (doc->doc->getErrorCode() == errEncrypted) {
            doc->is_locked = true;
        }
        return new document(*doc);
    }
    // put back the document data where it was before
    if (file_data) {
        file_data->swap(doc->doc_data);
    }
    delete doc;

    return nullptr;
}

/**
 \class poppler::document poppler-document.h "poppler/cpp/poppler-document.h"

 Represents a PDF %document.
 */

/**
 \enum poppler::document::page_mode_enum

 The various page modes available in a PDF %document.
*/
/**
 \var poppler::document::page_mode_enum poppler::document::use_none

 The %document specifies no particular page mode.
*/
/**
 \var poppler::document::page_mode_enum poppler::document::use_outlines

 The %document specifies its TOC (table of contents) should be open.
*/
/**
 \var poppler::document::page_mode_enum poppler::document::use_thumbs

 The %document specifies that should be open a view of the thumbnails of its
 pages.
*/
/**
 \var poppler::document::page_mode_enum poppler::document::fullscreen

 The %document specifies it wants to be open in a fullscreen mode.
*/
/**
 \var poppler::document::page_mode_enum poppler::document::use_oc

 The %document specifies that should be open a view of its Optional Content
 (also known as layers).
*/
/**
 \var poppler::document::page_mode_enum poppler::document::use_attach

 The %document specifies that should be open a view of its %document-level
 attachments.
 */

document::document(document_private &dd) : d(&dd) { }

document::~document()
{
    delete d;
}

/**
 \returns whether the current %document is locked
 */
bool document::is_locked() const
{
    return d->is_locked;
}

/**
 Unlocks the current document, if locked.

 \returns the new locking status of the document
 */
bool document::unlock(const std::string &owner_password, const std::string &user_password)
{
    if (d->is_locked) {
        document_private *newdoc = nullptr;
        if (!d->doc_data.empty()) {
            newdoc = new document_private(&d->doc_data, owner_password, user_password);
        } else if (d->raw_doc_data) {
            newdoc = new document_private(d->raw_doc_data, d->raw_doc_data_length, owner_password, user_password);
        } else {
            newdoc = new document_private(d->doc->getFileName()->copy(), owner_password, user_password);
        }
        if (!newdoc->doc->isOk()) {
            d->doc_data.swap(newdoc->doc_data);
            delete newdoc;
        } else {
            delete d;
            d = newdoc;
            d->is_locked = false;
        }
    }
    return d->is_locked;
}

/**
 \returns the eventual page mode specified by the current PDF %document
 */
document::page_mode_enum document::page_mode() const
{
    switch (d->doc->getCatalog()->getPageMode()) {
    case Catalog::pageModeNone:
        return use_none;
    case Catalog::pageModeOutlines:
        return use_outlines;
    case Catalog::pageModeThumbs:
        return use_thumbs;
    case Catalog::pageModeFullScreen:
        return fullscreen;
    case Catalog::pageModeOC:
        return use_oc;
    case Catalog::pageModeAttach:
        return use_attach;
    default:
        return use_none;
    }
}

/**
 \returns the eventual page layout specified by the current PDF %document
 */
document::page_layout_enum document::page_layout() const
{
    switch (d->doc->getCatalog()->getPageLayout()) {
    case Catalog::pageLayoutNone:
        return no_layout;
    case Catalog::pageLayoutSinglePage:
        return single_page;
    case Catalog::pageLayoutOneColumn:
        return one_column;
    case Catalog::pageLayoutTwoColumnLeft:
        return two_column_left;
    case Catalog::pageLayoutTwoColumnRight:
        return two_column_right;
    case Catalog::pageLayoutTwoPageLeft:
        return two_page_left;
    case Catalog::pageLayoutTwoPageRight:
        return two_page_right;
    default:
        return no_layout;
    }
}

/**
 Gets the version of the current PDF %document.

 Example:
 \code
 poppler::document *doc = ...;
 // for example, if the document is PDF 1.6:
 int major = 0, minor = 0;
 doc->get_pdf_version(&major, &minor);
 // major == 1
 // minor == 6
 \endcode

 \param major if not NULL, will be set to the "major" number of the version
 \param minor if not NULL, will be set to the "minor" number of the version
 */
void document::get_pdf_version(int *major, int *minor) const
{
    if (major) {
        *major = d->doc->getPDFMajorVersion();
    }
    if (minor) {
        *minor = d->doc->getPDFMinorVersion();
    }
}

/**
 \returns all the information keys available in the %document
 \see info_key, info_date
 */
std::vector<std::string> document::info_keys() const
{
    if (d->is_locked) {
        return std::vector<std::string>();
    }

    Object info = d->doc->getDocInfo();
    if (!info.isDict()) {
        return std::vector<std::string>();
    }

    Dict *info_dict = info.getDict();
    std::vector<std::string> keys(info_dict->getLength());
    for (int i = 0; i < info_dict->getLength(); ++i) {
        keys[i] = std::string(info_dict->getKey(i));
    }

    return keys;
}

/**
 Gets the value of the specified \p key of the document information.

 \returns the value for the \p key, or an empty string if not available
 \see info_keys, info_date
 */
ustring document::info_key(const std::string &key) const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_value(d->doc->getDocInfoStringEntry(key.c_str()));
    if (!goo_value) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_value.get());
}

/**
 Sets the value of the specified \p key of the %document information to \p val.
 If \p val is empty, the entry specified by \p key is removed.

 \returns true on success, false on failure
 */
bool document::set_info_key(const std::string &key, const ustring &val)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_val;

    if (!val.empty()) {
        goo_val = detail::ustring_to_unicode_GooString(val);
    }

    d->doc->setDocInfoStringEntry(key.c_str(), std::move(goo_val));
    return true;
}

/**
 Gets the time_t value of the specified \p key of the document
 information.

 \returns the time_t value for the \p key
 \see info_keys, info_date
 */
time_t document::info_date_t(const std::string &key) const
{
    if (d->is_locked) {
        return time_t(-1);
    }

    std::unique_ptr<GooString> goo_date(d->doc->getDocInfoStringEntry(key.c_str()));
    if (!goo_date) {
        return time_t(-1);
    }

    return dateStringToTime(goo_date.get());
}

/**
 Sets the time_t value of the specified \p key of the %document information
 to \p val.
 If \p val == time_t(-1), the entry specified by \p key is removed.

 \returns true on success, false on failure
 */
bool document::set_info_date_t(const std::string &key, time_t val)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_date;

    if (val != time_t(-1)) {
        goo_date = timeToDateString(&val);
    }

    d->doc->setDocInfoStringEntry(key.c_str(), std::move(goo_date));
    return true;
}

/**
 Gets the %document's title.

 \returns the document's title, or an empty string if not available
 \see set_title, info_key
 */
ustring document::get_title() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_title(d->doc->getDocInfoTitle());
    if (!goo_title) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_title.get());
}

/**
 Sets the %document's title to \p title.
 If \p title is empty, the %document's title is removed.

 \returns true on success, false on failure
 */
bool document::set_title(const ustring &title)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_title;

    if (!title.empty()) {
        goo_title = detail::ustring_to_unicode_GooString(title);
    }

    d->doc->setDocInfoTitle(std::move(goo_title));
    return true;
}

/**
 Gets the document's author.

 \returns the document's author, or an empty string if not available
 \see set_author, info_key
 */
ustring document::get_author() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_author(d->doc->getDocInfoAuthor());
    if (!goo_author) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_author.get());
}

/**
 Sets the %document's author to \p author.
 If \p author is empty, the %document's author is removed.

 \returns true on success, false on failure
 */
bool document::set_author(const ustring &author)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_author;

    if (!author.empty()) {
        goo_author = detail::ustring_to_unicode_GooString(author);
    }

    d->doc->setDocInfoAuthor(std::move(goo_author));
    return true;
}

/**
 Gets the document's subject.

 \returns the document's subject, or an empty string if not available
 \see set_subject, info_key
 */
ustring document::get_subject() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_subject(d->doc->getDocInfoSubject());
    if (!goo_subject) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_subject.get());
}

/**
 Sets the %document's subject to \p subject.
 If \p subject is empty, the %document's subject is removed.

 \returns true on success, false on failure
 */
bool document::set_subject(const ustring &subject)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_subject;

    if (!subject.empty()) {
        goo_subject = detail::ustring_to_unicode_GooString(subject);
    }

    d->doc->setDocInfoSubject(std::move(goo_subject));
    return true;
}

/**
 Gets the document's keywords.

 \returns the document's keywords, or an empty string if not available
 \see set_keywords, info_key
 */
ustring document::get_keywords() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_keywords(d->doc->getDocInfoKeywords());
    if (!goo_keywords) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_keywords.get());
}

/**
 Sets the %document's keywords to \p keywords.
 If \p keywords is empty, the %document's keywords are removed.

 \returns true on success, false on failure
 */
bool document::set_keywords(const ustring &keywords)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_keywords;

    if (!keywords.empty()) {
        goo_keywords = detail::ustring_to_unicode_GooString(keywords);
    }

    d->doc->setDocInfoKeywords(std::move(goo_keywords));
    return true;
}

/**
 Gets the document's creator.

 \returns the document's creator, or an empty string if not available
 \see set_creator, info_key
 */
ustring document::get_creator() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_creator(d->doc->getDocInfoCreator());
    if (!goo_creator) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_creator.get());
}

/**
 Sets the %document's creator to \p creator.
 If \p creator is empty, the %document's creator is removed.

 \returns true on success, false on failure
 */
bool document::set_creator(const ustring &creator)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_creator;

    if (!creator.empty()) {
        goo_creator = detail::ustring_to_unicode_GooString(creator);
    }

    d->doc->setDocInfoCreator(std::move(goo_creator));
    return true;
}

/**
 Gets the document's producer.

 \returns the document's producer, or an empty string if not available
 \see set_producer, info_key
 */
ustring document::get_producer() const
{
    if (d->is_locked) {
        return ustring();
    }

    std::unique_ptr<GooString> goo_producer(d->doc->getDocInfoProducer());
    if (!goo_producer) {
        return ustring();
    }

    return detail::unicode_GooString_to_ustring(goo_producer.get());
}

/**
 Sets the %document's producer to \p producer.
 If \p producer is empty, the %document's producer is removed.

 \returns true on success, false on failure
 */
bool document::set_producer(const ustring &producer)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_producer;

    if (!producer.empty()) {
        goo_producer = detail::ustring_to_unicode_GooString(producer);
    }

    d->doc->setDocInfoProducer(std::move(goo_producer));
    return true;
}

/**
 Gets the document's creation date as a time_t value.

 \returns the document's creation date as a time_t value
 \see set_creation_date, info_date
 */
time_t document::get_creation_date_t() const
{
    if (d->is_locked) {
        return time_t(-1);
    }

    std::unique_ptr<GooString> goo_creation_date(d->doc->getDocInfoCreatDate());
    if (!goo_creation_date) {
        return time_t(-1);
    }

    return dateStringToTime(goo_creation_date.get());
}

/**
 Sets the %document's creation date to \p creation_date.
 If \p creation_date == time_t(-1), the %document's creation date is removed.

 \returns true on success, false on failure
 */
bool document::set_creation_date_t(time_t creation_date)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_creation_date;

    if (creation_date != time_t(-1)) {
        goo_creation_date = timeToDateString(&creation_date);
    }

    d->doc->setDocInfoCreatDate(std::move(goo_creation_date));
    return true;
}

/**
 Gets the document's modification date as a time_t value.

 \returns the document's modification date as a time_t value
 \see set_modification_date, info_date
 */
time_t document::get_modification_date_t() const
{
    if (d->is_locked) {
        return time_t(-1);
    }

    std::unique_ptr<GooString> goo_modification_date(d->doc->getDocInfoModDate());
    if (!goo_modification_date) {
        return time_t(-1);
    }

    return dateStringToTime(goo_modification_date.get());
}

/**
 Sets the %document's modification date to \p mod_date.
 If \p mod_date == time_t(-1), the %document's modification date is removed.

 \returns true on success, false on failure
 */
bool document::set_modification_date_t(time_t mod_date)
{
    if (d->is_locked) {
        return false;
    }

    std::unique_ptr<GooString> goo_mod_date;

    if (mod_date != time_t(-1)) {
        goo_mod_date = timeToDateString(&mod_date);
    }

    d->doc->setDocInfoModDate(std::move(goo_mod_date));
    return true;
}

/**
 Removes the %document's Info dictionary.

 \returns true on success, false on failure
 */
bool document::remove_info()
{
    if (d->is_locked) {
        return false;
    }

    d->doc->removeDocInfo();
    return true;
}

/**
 \returns whether the document is encrypted
 */
bool document::is_encrypted() const
{
    return d->doc->isEncrypted();
}

/**
 \returns whether the document is linearized
 */
bool document::is_linearized() const
{
    return d->doc->isLinearized();
}

/**
 \returns the form type within the document
 \since 25.04
 */
enum document::form_type document::form_type() const
{
    switch (d->doc->getCatalog()->getFormType()) {
    case Catalog::AcroForm:
        return form_type::acro;
    case Catalog::XfaForm:
        return form_type::xfa;
    case Catalog::NoForm:
    default:
        return form_type::none;
    }
}

/**
 \returns true if the document contains javascript
 \since 25.04
 */
bool document::has_javascript() const
{
    return d->doc->getCatalog()->numJS() > 0;
}

/**
 Check for available "document permission".

 \returns whether the specified permission is allowed
 */
bool document::has_permission(permission_enum which) const
{
    switch (which) {
    case perm_print:
        return d->doc->okToPrint();
    case perm_change:
        return d->doc->okToChange();
    case perm_copy:
        return d->doc->okToCopy();
    case perm_add_notes:
        return d->doc->okToAddNotes();
    case perm_fill_forms:
        return d->doc->okToFillForm();
    case perm_accessibility:
        return d->doc->okToAccessibility();
    case perm_assemble:
        return d->doc->okToAssemble();
    case perm_print_high_resolution:
        return d->doc->okToPrintHighRes();
    }
    return true;
}

/**
 Reads the %document metadata string.

 \return the %document metadata string
 */
ustring document::metadata() const
{
    std::unique_ptr<GooString> md(d->doc->getCatalog()->readMetadata());
    if (md) {
        return detail::unicode_GooString_to_ustring(md.get());
    }
    return ustring();
}

/**
 Gets the IDs of the current PDF %document, if available.

 \param permanent_id if not NULL, will be set to the permanent ID of the %document
 \param update_id if not NULL, will be set to the update ID of the %document

 \returns whether the document has the IDs

 \since 0.16
 */
bool document::get_pdf_id(std::string *permanent_id, std::string *update_id) const
{
    GooString goo_permanent_id;
    GooString goo_update_id;

    if (!d->doc->getID(permanent_id ? &goo_permanent_id : nullptr, update_id ? &goo_update_id : nullptr)) {
        return false;
    }

    if (permanent_id) {
        *permanent_id = goo_permanent_id.c_str();
    }
    if (update_id) {
        *update_id = goo_update_id.c_str();
    }

    return true;
}

/**
 Document page count.

 \returns the number of pages of the document
 */
int document::pages() const
{
    return d->doc->getNumPages();
}

/**
 Document page by label reading.

 This creates a new page representing the %document %page whose label is the
 specified \p label. If there is no page with that \p label, NULL is returned.

 \returns a new page object or NULL
 */
page *document::create_page(const ustring &label) const
{
    std::unique_ptr<GooString> goolabel(detail::ustring_to_unicode_GooString(label));
    int index = 0;

    if (!d->doc->getCatalog()->labelToIndex(*goolabel, &index)) {
        return nullptr;
    }
    return create_page(index);
}

/**
 Document page by index reading.

 This creates a new page representing the \p index -th %page of the %document.
 \note the page indexes are in the range [0, pages()[.

 \returns a new page object or NULL
 */
page *document::create_page(int index) const
{
    if (index >= 0 && index < d->doc->getNumPages()) {
        page *p = new page(d, index);
        if (p->d->page) {
            return p;
        }
        delete p;
        return nullptr;
    }
    return nullptr;
}

/**
 Reads all the font information of the %document.

 \note this can be slow for big documents; prefer the use of a font_iterator
 to read incrementally page by page
 \see create_font_iterator
 */
std::vector<font_info> document::fonts() const
{
    std::vector<font_info> result;
    font_iterator it(0, d);
    while (it.has_next()) {
        const std::vector<font_info> l = it.next();
        std::ranges::copy(l, std::back_inserter(result));
    }
    return result;
}

/**
 Creates a new font iterator.

 This creates a new font iterator for reading the font information of the
 %document page by page, starting at the specified \p start_page (0 if not
 specified).

 \returns a new font iterator
 */
font_iterator *document::create_font_iterator(int start_page) const
{
    return new font_iterator(start_page, d);
}

/**
 Reads the TOC (table of contents) of the %document.

 \returns a new toc object if a TOC is available, NULL otherwise
 */
toc *document::create_toc() const
{
    return toc_private::load_from_outline(d->doc->getOutline());
}

/**
 Reads whether the current document has %document-level embedded files
 (attachments).

 This is a very fast way to know whether there are embedded files (also known
 as "attachments") at the %document-level. Note this does not take into account
 files embedded in other ways (e.g. to annotations).

 \returns whether the document has embedded files
 */
bool document::has_embedded_files() const
{
    return d->doc->getCatalog()->numEmbeddedFiles() > 0;
}

/**
 Reads all the %document-level embedded files of the %document.

 \returns the %document-level embedded files
 */
std::vector<embedded_file *> document::embedded_files() const
{
    if (d->is_locked) {
        return std::vector<embedded_file *>();
    }

    if (d->embedded_files.empty() && d->doc->getCatalog()->numEmbeddedFiles() > 0) {
        const int num = d->doc->getCatalog()->numEmbeddedFiles();
        d->embedded_files.resize(num);
        for (int i = 0; i < num; ++i) {
            std::unique_ptr<FileSpec> fs = d->doc->getCatalog()->embeddedFile(i);
            d->embedded_files[i] = embedded_file_private::create(std::move(fs));
        }
    }
    return d->embedded_files;
}

/**
 Creates a map of all the named destinations in the %document.

 \note The destination names may contain \\0 and other binary values
 so they are not printable and cannot convert to null-terminated C strings.

 \returns the map of the each name and destination

 \since 0.74
 */
std::map<std::string, destination> document::create_destination_map() const
{
    std::map<std::string, destination> m;

    Catalog *catalog = d->doc->getCatalog();
    if (!catalog) {
        return m;
    }

    // Iterate from name-dict
    const int nDests = catalog->numDests();
    for (int i = 0; i < nDests; ++i) {
        std::string key(catalog->getDestsName(i));
        std::unique_ptr<LinkDest> link_dest = catalog->getDestsDest(i);

        if (link_dest) {
            destination dest(new destination_private(link_dest.get(), d->doc.get()));

            m.emplace(std::move(key), std::move(dest));
        }
    }

    // Iterate from name-tree
    const int nDestsNameTree = catalog->numDestNameTree();
    for (int i = 0; i < nDestsNameTree; ++i) {
        std::string key(catalog->getDestNameTreeName(i)->toStr());
        std::unique_ptr<LinkDest> link_dest = catalog->getDestNameTreeDest(i);

        if (link_dest) {
            destination dest(new destination_private(link_dest.get(), d->doc.get()));

            m.emplace(std::move(key), std::move(dest));
        }
    }

    return m;
}

/**
 Saves the %document to file \p file_name.

 \returns true on success, false on failure
 */
bool document::save(const std::string &file_name) const
{
    if (d->is_locked) {
        return false;
    }

    return d->doc->saveAs(file_name) == errNone;
}

/**
 Saves the original version of the %document to file \p file_name.

 \returns true on success, false on failure
 */
bool document::save_a_copy(const std::string &file_name) const
{
    if (d->is_locked) {
        return false;
    }

    return d->doc->saveWithoutChangesAs(file_name) == errNone;
}

/**
 Tries to load a PDF %document from the specified file.

 \param file_name the file to open
 \returns a new document if the load succeeded (even if the document is locked),
          NULL otherwise
 */
document *document::load_from_file(const std::string &file_name, const std::string &owner_password, const std::string &user_password)
{
    auto *doc = new document_private(std::make_unique<GooString>(file_name.c_str()), owner_password, user_password);
    return document_private::check_document(doc, nullptr);
}

/**
 Tries to load a PDF %document from the specified data.

 \note if the loading succeeds, the document takes ownership of the
       \p file_data (swap()ing it)

 \param file_data the data representing a document to open
 \returns a new document if the load succeeded (even if the document is locked),
          NULL otherwise
 */
document *document::load_from_data(byte_array *file_data, const std::string &owner_password, const std::string &user_password)
{
    if (!file_data || file_data->size() < 10) {
        return nullptr;
    }

    auto *doc = new document_private(file_data, owner_password, user_password);
    return document_private::check_document(doc, file_data);
}

/**
 Tries to load a PDF %document from the specified data buffer.

 \note the buffer must remain valid for the whole lifetime of the returned
       document

 \param file_data the data buffer representing a document to open
 \param file_data_length the length of the data buffer

 \returns a new document if the load succeeded (even if the document is locked),
          NULL otherwise

 \since 0.16
 */
document *document::load_from_raw_data(const char *file_data, int file_data_length, const std::string &owner_password, const std::string &user_password)
{
    if (!file_data || file_data_length < 10) {
        return nullptr;
    }

    auto *doc = new document_private(file_data, file_data_length, owner_password, user_password);
    return document_private::check_document(doc, nullptr);
}
