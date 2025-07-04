/* poppler-qt.h: qt interface to poppler
 * Copyright (C) 2005, Net Integration Technologies, Inc.
 * Copyright (C) 2005, 2007, Brad Hards <bradh@frogmouth.net>
 * Copyright (C) 2005-2015, 2017-2022, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2005, Stefan Kebekus <stefan.kebekus@math.uni-koeln.de>
 * Copyright (C) 2006-2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2009 Shawn Rutledge <shawn.t.rutledge@gmail.com>
 * Copyright (C) 2010 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
 * Copyright (C) 2010 Matthias Fauconneau <matthias.fauconneau@gmail.com>
 * Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
 * Copyright (C) 2011 Glad Deschrijver <glad.deschrijver@gmail.com>
 * Copyright (C) 2012, Guillermo A. Amaral B. <gamaral@kde.org>
 * Copyright (C) 2012, Fabio D'Urso <fabiodurso@hotmail.it>
 * Copyright (C) 2012, Tobias Koenig <tobias.koenig@kdab.com>
 * Copyright (C) 2012, 2014, 2015, 2018, 2019 Adam Reichold <adamreichold@myopera.com>
 * Copyright (C) 2012, 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
 * Copyright (C) 2013 Anthony Granger <grangeranthony@gmail.com>
 * Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
 * Copyright (C) 2017, 2020, 2021 Oliver Sander <oliver.sander@tu-dresden.de>
 * Copyright (C) 2017, 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
 * Copyright (C) 2018, 2021 Nelson Benítez León <nbenitezl@gmail.com>
 * Copyright (C) 2019 Jan Grulich <jgrulich@redhat.com>
 * Copyright (C) 2019 Alexander Volkov <a.volkov@rusbitech.ru>
 * Copyright (C) 2020 Philipp Knechtges <philipp-dev@knechtges.com>
 * Copyright (C) 2020 Katarina Behrens <Katarina.Behrens@cib.de>
 * Copyright (C) 2020 Thorsten Behrens <Thorsten.Behrens@CIB.de>
 * Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 * Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>.
 * Copyright (C) 2021 Mahmoud Khalil <mahmoudkhalil11@gmail.com>
 * Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
 * Copyright (C) 2022 Martin <martinbts@gmx.net>
 * Copyright (C) 2023 Kevin Ottens <kevin.ottens@enioka.com>. Work sponsored by De Bortoli Wines
 * Copyright (C) 2024 Pratham Gandhi <ppg.1382@gmail.com>
 * Copyright (C) 2025, g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#ifndef __POPPLER_QT_H__
#define __POPPLER_QT_H__

#include <functional>

#include "poppler-annotation.h"
#include "poppler-link.h"
#include "poppler-optcontent.h"
#include "poppler-page-transition.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QSet>
#include <QtXml/QDomDocument>
#include "poppler-export.h"

class EmbFile;
class Sound;
class AnnotMovie;

/**
   The %Poppler Qt5 binding.
*/
namespace Poppler {

class Document;
class DocumentData;

class PageData;

class FormField;
class FormFieldSignature;

class TextBoxData;

class PDFConverter;
class PSConverter;

struct OutlineItemData;

/**
    Debug/error function.

    This function type is used for debugging & error output;
    the first parameter is the actual message, the second is the unaltered
    closure argument which was passed to the setDebugErrorFunction call.

    \since 0.16
*/
using PopplerDebugFunc = void (*)(const QString & /*message*/, const QVariant & /*closure*/);

/**
    Set a new debug/error output function.

    If not set, by default error and debug messages will be sent to the
    Qt \p qDebug() function.

    \param debugFunction the new debug function
    \param closure user data which will be passes as-is to the debug function

    \since 0.16
*/
POPPLER_QT5_EXPORT void setDebugErrorFunction(PopplerDebugFunc debugFunction, const QVariant &closure);

/**
 * The various types of error strings.
 *
 * \since 25.07
 */
enum class ErrorStringType
{
    /**The string should be treated like a error code. It could be a hex code, a position in the poppler sources or something similar*/
    ErrorCodeString,
    /**The string should be treated as an advanced error message that can be shown to user */
    UserString
};

/**
 * Combination of an error data and type of error string
 *
 * \since 25.07
 */
struct ErrorString
{
    QVariant data;
    ErrorStringType type;
};

/**
    Describes the physical location of text on a document page

    This very simple class describes the physical location of text
    on the page. It consists of
    - a QString that contains the text
    - a QRectF that gives a box that describes where on the page
    the text is found.
*/
class POPPLER_QT5_EXPORT TextBox
{
    friend class Page;

public:
    /**
       The default constructor sets the \p text and the rectangle that
       contains the text. Coordinates for the \p bBox are in points =
       1/72 of an inch.
    */
    TextBox(const QString &text, const QRectF &bBox);
    /**
      Destructor.
    */
    ~TextBox();

    /**
        Returns the text of this text box
    */
    QString text() const;

    /**
        Returns the position of the text, in point, i.e., 1/72 of
       an inch

       \since 0.8
    */
    QRectF boundingBox() const;

    /**
        Returns the pointer to the next text box, if there is one.

        Otherwise, it returns a null pointer.
    */
    TextBox *nextWord() const;

    /**
        Returns the bounding box of the \p i -th characted of the word.
    */
    QRectF charBoundingBox(int i) const;

    /**
        Returns whether there is a space character after this text box
    */
    bool hasSpaceAfter() const;

private:
    Q_DISABLE_COPY(TextBox)

    TextBoxData *m_data;
};

class FontInfoData;
/**
   Container class for information about a font within a PDF
   document
*/
class POPPLER_QT5_EXPORT FontInfo
{
    friend class Document;

public:
    /**
       The type of font.
    */
    enum Type
    {
        unknown,
        Type1,
        Type1C,
        Type1COT,
        Type3,
        TrueType,
        TrueTypeOT,
        CIDType0,
        CIDType0C,
        CIDType0COT,
        CIDTrueType,
        CIDTrueTypeOT
    };

    /// \cond PRIVATE
    /**
       Create a new font information container.
    */
    FontInfo();

    /**
       Create a new font information container.
    */
    explicit FontInfo(const FontInfoData &fid);
    /// \endcond

    /**
       Copy constructor.
    */
    FontInfo(const FontInfo &fi);

    /**
       Destructor.
    */
    ~FontInfo();

    /**
       The name of the font. Can be a null QString if the font has no name
    */
    QString name() const;

    /**
       The name of the substitute font. Can be a null QString if the font has no substitute font
       @since 0.80
    */
    QString substituteName() const;

    /**
       The path of the font file used to represent this font on this system,
       or a null string is the font is embedded
    */
    QString file() const;

    /**
       Whether the font is embedded in the file, or not

       \return true if the font is embedded
    */
    bool isEmbedded() const;

    /**
       Whether the font provided is only a subset of the full
       font or not. This only has meaning if the font is embedded.

       \return true if the font is only a subset
    */
    bool isSubset() const;

    /**
       The type of font encoding

       \return a enumerated value corresponding to the font encoding used

       \sa typeName for a string equivalent
    */
    Type type() const;

    /**
       The name of the font encoding used

       \note if you are looking for the name of the font (as opposed to the
       encoding format used), you probably want name().

       \sa type for a enumeration version
    */
    QString typeName() const;

    /**
       Standard assignment operator
    */
    FontInfo &operator=(const FontInfo &fi);

private:
    FontInfoData *m_data;
};

class FontIteratorData;
/**
   Iterator for reading the fonts in a document.

   FontIterator provides a Java-style iterator for reading the fonts in a
   document.

   You can use it in the following way:
   \code
Poppler::FontIterator* it = doc->newFontIterator();
while (it->hasNext()) {
QList<Poppler::FontInfo> fonts = it->next();
// do something with the fonts
}
// after doing the job, the iterator must be freed
delete it;
   \endcode

   \since 0.12
*/
class POPPLER_QT5_EXPORT FontIterator
{
    friend class Document;
    friend class DocumentData;

public:
    /**
       Destructor.
    */
    ~FontIterator();

    /**
       Returns the fonts of the current page and then advances the iterator
       to the next page.
    */
    QList<FontInfo> next();

    /**
       Checks whether there is at least one more page to iterate, ie returns
       false when the iterator is beyond the last page.
    */
    bool hasNext() const;

    /**
       Returns the current page where the iterator is.
    */
    int currentPage() const;

private:
    Q_DISABLE_COPY(FontIterator)
    FontIterator(int, DocumentData *dd);

    FontIteratorData *d;
};

class EmbeddedFileData;
/**
   Container class for an embedded file with a PDF document
*/
class POPPLER_QT5_EXPORT EmbeddedFile
{
    friend class DocumentData;
    friend class AnnotationPrivate;

public:
    /// \cond PRIVATE
    explicit EmbeddedFile(EmbFile *embfile);
    /// \endcond

    /**
       Destructor.
    */
    ~EmbeddedFile();

    /**
       The name associated with the file
    */
    QString name() const;

    /**
       The description associated with the file, if any.

       This will return an empty QString if there is no description element
    */
    QString description() const;

    /**
       The size of the file.

       This will return < 0 if there is no size element
    */
    int size() const;

    /**
       The modification date for the embedded file, if known.
    */
    QDateTime modDate() const;

    /**
       The creation date for the embedded file, if known.
    */
    QDateTime createDate() const;

    /**
       The MD5 checksum of the file.

       This will return an empty QByteArray if there is no checksum element.
    */
    QByteArray checksum() const;

    /**
       The MIME type of the file, if known.

       \since 0.8
    */
    QString mimeType() const;

    /**
       The data as a byte array
    */
    QByteArray data();

    /**
       Is the embedded file valid?

       \since 0.12
    */
    bool isValid() const;

    /**
       A QDataStream for the actual data?
    */
    // QDataStream dataStream() const;

private:
    Q_DISABLE_COPY(EmbeddedFile)
    explicit EmbeddedFile(EmbeddedFileData &dd);

    EmbeddedFileData *m_embeddedFile;
};

/**
   \brief A page in a document.

   The Page class represents a single page within a PDF document.

   You cannot construct a Page directly, but you have to use the Document
   functions that return a new Page out of an index or a label.
*/
class POPPLER_QT5_EXPORT Page
{
    friend class Document;

public:
    /**
       Destructor.
    */
    ~Page();

    /**
       The type of rotation to apply for an operation
    */
    enum Rotation
    {
        Rotate0 = 0, ///< Do not rotate
        Rotate90 = 1, ///< Rotate 90 degrees clockwise
        Rotate180 = 2, ///< Rotate 180 degrees
        Rotate270 = 3 ///< Rotate 270 degrees clockwise (90 degrees counterclockwise)
    };

    /**
       The kinds of page actions
    */
    enum PageAction
    {
        Opening, ///< The action when a page is "opened"
        Closing ///< The action when a page is "closed"
    };

    /**
       How the text is going to be returned
       \since 0.16
    */
    enum TextLayout
    {
        PhysicalLayout, ///< The text is layouted to resemble the real page layout
        RawOrderLayout ///< The text is returned without any type of processing
    };

    /**
       Additional flags for the renderToPainter method
       \since 0.16
    */
    enum PainterFlag
    {
        NoPainterFlags = 0x00000000, ///< \since 0.63
        /**
           Do not save/restore the caller-owned painter.

           renderToPainter() by default preserves, using save() + restore(),
           the state of the painter specified; if this is not needed, this
           flag can avoid this job
         */
        DontSaveAndRestore = 0x00000001
    };
    Q_DECLARE_FLAGS(PainterFlags, PainterFlag)

    /**
       Render the page to a QImage using the current
       \link Document::renderBackend() Document renderer\endlink.

       If \p x = \p y = \p w = \p h = -1, the method will automatically
       compute the size of the image from the horizontal and vertical
       resolutions specified in \p xres and \p yres. Otherwise, the
       method renders only a part of the page, specified by the
       parameters (\p x, \p y, \p w, \p h) in pixel coordinates. The returned
       QImage then has size (\p w, \p h), independent of the page
       size.

       \param x specifies the left x-coordinate of the box, in
       pixels.

       \param y specifies the top y-coordinate of the box, in
       pixels.

       \param w specifies the width of the box, in pixels.

       \param h specifies the height of the box, in pixels.

       \param xres horizontal resolution of the graphics device,
       in dots per inch

       \param yres vertical resolution of the graphics device, in
       dots per inch

       \param rotate how to rotate the page

       \warning The parameter (\p x, \p y, \p w, \p h) are not
       well-tested. Unusual or meaningless parameters may lead to
       rather unexpected results.

       \returns a QImage of the page, or a null image on failure.

       \since 0.6
    */
    QImage renderToImage(double xres = 72.0, double yres = 72.0, int x = -1, int y = -1, int w = -1, int h = -1, Rotation rotate = Rotate0) const;

    /**
        Partial Update renderToImage callback.

        This function type is used for doing partial rendering updates;
        the first parameter is the image as rendered up to now, the second is the unaltered
        closure argument which was passed to the renderToImage call.

        \since 0.62
    */
    using RenderToImagePartialUpdateFunc = void (*)(const QImage & /*image*/, const QVariant & /*closure*/);

    /**
        Partial Update query renderToImage callback.

        This function type is used for query if the partial rendering update should happen;
        the parameter is the unaltered closure argument which was passed to the renderToImage call.

        \since 0.62
    */
    using ShouldRenderToImagePartialQueryFunc = bool (*)(const QVariant & /*closure*/);

    /**
       Render the page to a QImage using the current
       \link Document::renderBackend() Document renderer\endlink.

       If \p x = \p y = \p w = \p h = -1, the method will automatically
       compute the size of the image from the horizontal and vertical
       resolutions specified in \p xres and \p yres. Otherwise, the
       method renders only a part of the page, specified by the
       parameters (\p x, \p y, \p w, \p h) in pixel coordinates. The returned
       QImage then has size (\p w, \p h), independent of the page
       size.

       \param x specifies the left x-coordinate of the box, in
       pixels.

       \param y specifies the top y-coordinate of the box, in
       pixels.

       \param w specifies the width of the box, in pixels.

       \param h specifies the height of the box, in pixels.

       \param xres horizontal resolution of the graphics device,
       in dots per inch

       \param yres vertical resolution of the graphics device, in
       dots per inch

       \param rotate how to rotate the page

       \param partialUpdateCallback callback that will be called to
       report a partial rendering update

       \param shouldDoPartialUpdateCallback callback that will be called
       to ask if a partial rendering update is wanted. This exists
       because doing a partial rendering update needs to copy the image
       buffer so if it is not wanted it is better skipped early.

       \param payload opaque structure that will be passed
       back to partialUpdateCallback and shouldDoPartialUpdateCallback.

       \warning The parameter (\p x, \p y, \p w, \p h) are not
       well-tested. Unusual or meaningless parameters may lead to
       rather unexpected results.

       \returns a QImage of the page, or a null image on failure.

       \since 0.62
    */
    QImage renderToImage(double xres, double yres, int x, int y, int w, int h, Rotation rotate, RenderToImagePartialUpdateFunc partialUpdateCallback, ShouldRenderToImagePartialQueryFunc shouldDoPartialUpdateCallback,
                         const QVariant &payload) const;

    /**
        Abort query function callback.

        This function type is used for query if the current rendering/text extraction should be cancelled.

        \since 0.63
    */
    using ShouldAbortQueryFunc = bool (*)(const QVariant & /*closure*/);

    /**
Render the page to a QImage using the current
\link Document::renderBackend() Document renderer\endlink.

If \p x = \p y = \p w = \p h = -1, the method will automatically
compute the size of the image from the horizontal and vertical
resolutions specified in \p xres and \p yres. Otherwise, the
method renders only a part of the page, specified by the
parameters (\p x, \p y, \p w, \p h) in pixel coordinates. The returned
QImage then has size (\p w, \p h), independent of the page
size.

\param x specifies the left x-coordinate of the box, in
pixels.

\param y specifies the top y-coordinate of the box, in
pixels.

\param w specifies the width of the box, in pixels.

\param h specifies the height of the box, in pixels.

\param xres horizontal resolution of the graphics device,
in dots per inch

\param yres vertical resolution of the graphics device, in
dots per inch

\param rotate how to rotate the page

\param partialUpdateCallback callback that will be called to
report a partial rendering update

\param shouldDoPartialUpdateCallback callback that will be called
to ask if a partial rendering update is wanted. This exists
because doing a partial rendering update needs to copy the image
buffer so if it is not wanted it is better skipped early.

\param shouldAbortRenderCallback callback that will be called
to ask if the rendering should be cancelled.

\param payload opaque structure that will be passed
back to partialUpdateCallback, shouldDoPartialUpdateCallback
and shouldAbortRenderCallback.

\warning The parameter (\p x, \p y, \p w, \p h) are not
well-tested. Unusual or meaningless parameters may lead to
rather unexpected results.

\returns a QImage of the page, or a null image on failure.

\since 0.63
*/
    QImage renderToImage(double xres, double yres, int x, int y, int w, int h, Rotation rotate, RenderToImagePartialUpdateFunc partialUpdateCallback, ShouldRenderToImagePartialQueryFunc shouldDoPartialUpdateCallback,
                         ShouldAbortQueryFunc shouldAbortRenderCallback, const QVariant &payload) const;

    /**
       Render the page to the specified QPainter using the current
       \link Document::renderBackend() Document renderer\endlink.

       If \p x = \p y = \p w = \p h = -1, the method will automatically
       compute the size of the page area from the horizontal and vertical
       resolutions specified in \p xres and \p yres. Otherwise, the
       method renders only a part of the page, specified by the
       parameters (\p x, \p y, \p w, \p h) in pixel coordinates.

       \param painter the painter to paint on

       \param x specifies the left x-coordinate of the box, in
       pixels.

       \param y specifies the top y-coordinate of the box, in
       pixels.

       \param w specifies the width of the box, in pixels.

       \param h specifies the height of the box, in pixels.

       \param xres horizontal resolution of the graphics device,
       in dots per inch

       \param yres vertical resolution of the graphics device, in
       dots per inch

       \param rotate how to rotate the page

       \param flags additional painter flags

       \warning The parameter (\p x, \p y, \p w, \p h) are not
       well-tested. Unusual or meaningless parameters may lead to
       rather unexpected results.

       \returns whether the painting succeeded

       \note This method is only supported for the QPainterOutputDev

       \since 0.16
    */
    bool renderToPainter(QPainter *painter, double xres = 72.0, double yres = 72.0, int x = -1, int y = -1, int w = -1, int h = -1, Rotation rotate = Rotate0, PainterFlags flags = NoPainterFlags) const;

    /**
       Get the page thumbnail if it exists.

       \return a QImage of the thumbnail, or a null image
       if the PDF does not contain one for this page

       \since 0.12
    */
    QImage thumbnail() const;

    /**
       Returns the text that is inside a specified rectangle

       \param rect the rectangle specifying the area of interest,
       with coordinates given in points, i.e., 1/72th of an inch.
       If rect is null, all text on the page is given

       \since 0.16
    **/
    QString text(const QRectF &rect, TextLayout textLayout) const;

    /**
       Returns the text that is inside a specified rectangle.
       The text is returned using the physical layout of the page

       \param rect the rectangle specifying the area of interest,
       with coordinates given in points, i.e., 1/72th of an inch.
       If rect is null, all text on the page is given
    **/
    QString text(const QRectF &rect) const;

    /**
       The starting point for a search
    */
    enum SearchDirection
    {
        FromTop, ///< Start sorting at the top of the document
        NextResult, ///< Find the next result, moving "down the page"
        PreviousResult ///< Find the previous result, moving "up the page"
    };

    /**
       The type of search to perform
    */
    enum SearchMode
    {
        CaseSensitive, ///< Case differences cause no match in searching
        CaseInsensitive ///< Case differences are ignored in matching
    };

    /**
       Flags to modify the search behaviour \since 0.31
    */
    enum SearchFlag
    {
        NoSearchFlags = 0x00000000, ///< since 0.63
        IgnoreCase = 0x00000001, ///< Case differences are ignored
        WholeWords = 0x00000002, ///< Only whole words are matched
        IgnoreDiacritics = 0x00000004, ///< Diacritic differences (eg. accents, umlauts, diaeresis) are ignored. \since 0.73
                                       ///< This option will have no effect if the search term contains characters which
                                       ///< are not pure ascii.
        AcrossLines = 0x00000008 ///< Allows to match on text spanning from end of a line to the next line.
                                 ///< It won't match on text spanning more than two lines. Automatically ignores hyphen
                                 ///< at end of line, and allows whitespace in search term to match on newline. \since 21.05.0
    };
    Q_DECLARE_FLAGS(SearchFlags, SearchFlag)

    /**
       Returns true if the specified text was found.

       \param text the text the search
       \param rectXXX in all directions is used to return where the text was found, for NextResult and PreviousResult
                   indicates where to continue searching for
       \param direction in which direction do the search
       \param caseSensitive be case sensitive?
       \param rotate the rotation to apply for the search order
       \since 0.14
    **/
    Q_DECL_DEPRECATED bool search(const QString &text, double &rectLeft, double &rectTop, double &rectRight, double &rectBottom, SearchDirection direction, SearchMode caseSensitive, Rotation rotate = Rotate0) const;

    /**
       Returns true if the specified text was found.

       \param text the text the search
       \param rectXXX in all directions is used to return where the text was found, for NextResult and PreviousResult
                   indicates where to continue searching for
       \param direction in which direction do the search
       \param flags the flags to consider during matching
       \param rotate the rotation to apply for the search order

       \since 0.31
    **/
    bool search(const QString &text, double &sLeft, double &sTop, double &sRight, double &sBottom, SearchDirection direction, SearchFlags flags = NoSearchFlags, Rotation rotate = Rotate0) const;

    /**
       Returns a list of all occurrences of the specified text on the page.

       \param text the text to search
       \param caseSensitive whether to be case sensitive
       \param rotate the rotation to apply for the search order

       \warning Do not use the returned QRectF as arguments of another search call because of truncation issues if qreal is defined as float.

       \since 0.22
    **/
    Q_DECL_DEPRECATED QList<QRectF> search(const QString &text, SearchMode caseSensitive, Rotation rotate = Rotate0) const;

    /**
       Returns a list of all occurrences of the specified text on the page.

       if SearchFlags::AcrossLines is given in \param flags, then rects may just
       be parts of the text itself if it's split between multiple lines.

       \param text the text to search
       \param flags the flags to consider during matching
       \param rotate the rotation to apply for the search order

       \warning Do not use the returned QRectF as arguments of another search call because of truncation issues if qreal is defined as float.

       \since 0.31
    **/
    QList<QRectF> search(const QString &text, SearchFlags flags = NoSearchFlags, Rotation rotate = Rotate0) const;

    /**
       Returns a list of text of the page

       This method returns a QList of TextBoxes that contain all
       the text of the page, with roughly one text word of text
       per TextBox item.

       For text written in western languages (left-to-right and
       up-to-down), the QList contains the text in the proper
       order.

       \note The caller owns the text boxes and they should
             be deleted when no longer required.

       \warning This method is not tested with Asian scripts
    */
    QList<TextBox *> textList(Rotation rotate = Rotate0) const;

    /**
       Returns a list of text of the page

       This method returns a QList of TextBoxes that contain all
       the text of the page, with roughly one text word of text
       per TextBox item.

       For text written in western languages (left-to-right and
       up-to-down), the QList contains the text in the proper
       order.

       \param shouldAbortExtractionCallback callback that will be called
       to ask if the text extraction should be cancelled.

       \param closure opaque structure that will be passed
       back to shouldAbortExtractionCallback.

       \note The caller owns the text boxes and they should
             be deleted when no longer required.

       \warning This method is not tested with Asian scripts

       // \since 0.63
    */
    QList<TextBox *> textList(Rotation rotate, ShouldAbortQueryFunc shouldAbortExtractionCallback, const QVariant &closure) const;

    /**
       \return The dimensions (cropbox) of the page, in points (i.e. 1/72th of an inch)
    */
    QSizeF pageSizeF() const;

    /**
       \return The dimensions (cropbox) of the page, in points (i.e. 1/72th of an inch)
    */
    QSize pageSize() const;

    /**
      Returns the transition of this page

      \returns a pointer to a PageTransition structure that
      defines how transition to this page shall be performed.

      \note The PageTransition structure is owned by this page, and will
      automatically be destroyed when this page class is
      destroyed.
    **/
    PageTransition *transition() const;

    /**
      Gets the page action specified, or NULL if there is no action.

      \since 0.6
    **/
    Link *action(PageAction act) const;

    /**
       Types of orientations that are possible
    */
    enum Orientation
    {
        Landscape, ///< Landscape orientation (portrait, with 90 degrees clockwise rotation )
        Portrait, ///< Normal portrait orientation
        Seascape, ///< Seascape orientation (portrait, with 270 degrees clockwise rotation)
        UpsideDown ///< Upside down orientation (portrait, with 180 degrees rotation)
    };

    /**
       The orientation of the page
    */
    Orientation orientation() const;

    /**
      The default CTM
    */
    void defaultCTM(double *CTM, double dpiX, double dpiY, int rotate, bool upsideDown);

    /**
      Gets the links of the page
    */
    QList<Link *> links() const;

    /**
     Returns the annotations of the page

     \note If you call this method twice, you get different objects
           pointing to the same annotations (see Annotation).
           The caller owns the returned objects and they should be deleted
           when no longer required.
    */
    QList<Annotation *> annotations() const;

    /**
            Returns the annotations of the page

            \param subtypes the subtypes of annotations you are interested in

            \note If you call this method twice, you get different objects
                  pointing to the same annotations (see Annotation).
                  The caller owns the returned objects and they should be deleted
                  when no longer required.

            \since 0.28
    */
    QList<Annotation *> annotations(const QSet<Annotation::SubType> &subtypes) const;

    /**
     Adds an annotation to the page

     \note Ownership of the annotation object stays with the caller, who can
           delete it at any time.
     \since 0.20
    */
    void addAnnotation(const Annotation *ann);

    /**
     Removes an annotation from the page and destroys the annotation object

     \note There mustn't be other Annotation objects pointing this annotation
     \since 0.20
    */
    void removeAnnotation(const Annotation *ann);

    /**
     Returns the form fields on the page
     The caller gets the ownership of the returned objects.

     \since 0.6
    */
    QList<FormField *> formFields() const;

    /**
     Returns the page duration. That is the time, in seconds, that the page
     should be displayed before the presentation automatically advances to the next page.
     Returns < 0 if duration is not set.

     \since 0.6
    */
    double duration() const;

    /**
       Returns the label of the page, or a null string is the page has no label.

     \since 0.6
    **/
    QString label() const;

    /**
       Returns the index of the page.

     \since 0.70
    **/
    int index() const;

private:
    Q_DISABLE_COPY(Page)

    Page(DocumentData *doc, int index);
    PageData *m_page;
};

/**
   \brief Item in the outline of a PDF document

   Represents an item in the outline of PDF document, i.e. a name, an internal or external link and a set of child items.

   \since 0.74
**/
class POPPLER_QT5_EXPORT OutlineItem
{
    friend class Document;

public:
    /**
       Constructs a null item, i.e. one that does not represent a valid item in the outline of some PDF document.
    **/
    OutlineItem();
    ~OutlineItem();

    OutlineItem(const OutlineItem &other);
    OutlineItem &operator=(const OutlineItem &other);

    OutlineItem(OutlineItem &&other) noexcept;
    OutlineItem &operator=(OutlineItem &&other) noexcept;

    /**
       Indicates whether an item is null, i.e. whether it does not represent a valid item in the outline of some PDF document.
    **/
    bool isNull() const;

    /**
       The name of the item which should be displayed to the user.
    **/
    QString name() const;

    /**
       Indicates whether the item should initially be display in an expanded or collapsed state.
    **/
    bool isOpen() const;

    /**
       The destination referred to by this item.

       \returns a shared pointer to an immutable link destination
    **/
    QSharedPointer<const LinkDestination> destination() const;

    /**
       The external file name of the document to which the \see destination refers

       \returns a string with the external file name or an empty string if there is none
     */
    QString externalFileName() const;

    /**
       The URI to which the item links

       \returns a string with the URI which this item links or an empty string if there is none
    **/
    QString uri() const;

    /**
       Determines if this item has any child items

       \returns true if there are any child items
    **/
    bool hasChildren() const;

    /**
       Gets the child items of this item

       \returns a vector outline items, empty if there are none
    **/
    QVector<OutlineItem> children() const;

private:
    explicit OutlineItem(OutlineItemData *data);
    OutlineItemData *m_data;
};

/**
   \brief PDF document.

   The Document class represents a PDF document: its pages, and all the global
   properties, metadata, etc.

   \section ownership Ownership of the returned objects

   All the functions that returns class pointers create new object, and the
   responsibility of those is given to the callee.

   The only exception is \link Poppler::Page::transition() Page::transition()\endlink.

   \section document-loading Loading

   To get a Document, you have to load it via the load() & loadFromData()
   functions.

   In all the functions that have passwords as arguments, they \b must be Latin1
   encoded. If you have a password that is a UTF-8 string, you need to use
   QString::toLatin1() (or similar) to convert the password first.
   If you have a UTF-8 character array, consider converting it to a QString first
   (QString::fromUtf8(), or similar) before converting to Latin1 encoding.

   \section document-rendering Rendering

   To render pages of a document, you have different Document functions to set
   various options.

   \subsection document-rendering-backend Backends

   %Poppler offers a different backends for rendering the pages. Currently
   there are two backends (see #RenderBackend), but only the Splash engine works
   well and has been tested.

   The available rendering backends can be discovered via availableRenderBackends().
   The current rendering backend can be changed using setRenderBackend().
   Please note that setting a backend not listed in the available ones
   will always result in null QImage's.

   \section document-cms Color management support

   %Poppler, if compiled with this support, provides functions to handle color
   profiles.

   To know whether the %Poppler version you are using has support for color
   management, you can query Poppler::isCmsAvailable(). In case it is not
   available, all the color management-related functions will either do nothing
   or return null.
*/
class POPPLER_QT5_EXPORT Document
{
    friend class Page;
    friend class DocumentData;

public:
    /**
       The page mode
    */
    enum PageMode
    {
        UseNone, ///< No mode - neither document outline nor thumbnail images are visible
        UseOutlines, ///< Document outline visible
        UseThumbs, ///< Thumbnail images visible
        FullScreen, ///< Fullscreen mode (no menubar, windows controls etc)
        UseOC, ///< Optional content group panel visible
        UseAttach ///< Attachments panel visible
    };

    /**
       The page layout
    */
    enum PageLayout
    {
        NoLayout, ///< Layout not specified
        SinglePage, ///< Display a single page
        OneColumn, ///< Display a single column of pages
        TwoColumnLeft, ///< Display the pages in two columns, with odd-numbered pages on the left
        TwoColumnRight, ///< Display the pages in two columns, with odd-numbered pages on the right
        TwoPageLeft, ///< Display the pages two at a time, with odd-numbered pages on the left
        TwoPageRight ///< Display the pages two at a time, with odd-numbered pages on the right
    };

    /**
       The render backends available

       \since 0.6
    */
    enum RenderBackend
    {
        SplashBackend, ///< Splash backend
        ArthurBackend, ///< \deprecated The old name of the QPainter backend
        QPainterBackend = ArthurBackend ///< @since 20.11
    };

    /**
       The render hints available

       \since 0.6
    */
    enum RenderHint
    {
        Antialiasing = 0x00000001, ///< Antialiasing for graphics
        TextAntialiasing = 0x00000002, ///< Antialiasing for text
        TextHinting = 0x00000004, ///< Hinting for text \since 0.12.1
        TextSlightHinting = 0x00000008, ///< Lighter hinting for text when combined with TextHinting \since 0.18
        OverprintPreview = 0x00000010, ///< Overprint preview \since 0.22
        ThinLineSolid = 0x00000020, ///< Enhance thin lines solid \since 0.24
        ThinLineShape = 0x00000040, ///< Enhance thin lines shape. Wins over ThinLineSolid \since 0.24
        IgnorePaperColor = 0x00000080, ///< Do not compose with the paper color \since 0.35
        HideAnnotations = 0x00000100 ///< Do not render annotations \since 0.60
    };
    Q_DECLARE_FLAGS(RenderHints, RenderHint)

    /**
       Form types

       \since 0.22
    */
    enum FormType
    {
        NoForm, ///< Document doesn't contain forms
        AcroForm, ///< AcroForm
        XfaForm ///< Adobe XML Forms Architecture (XFA), currently unsupported
    };

    /**
      Set a color display profile for the current document.

      \param outputProfileA is a \c cmsHPROFILE of the LCMS library.

      \note This should be called before any rendering happens.

      \note It is assumed that poppler takes over the owernship of the corresponding cmsHPROFILE. In particular,
      it is no longer the caller's responsibility to close the profile after use.

       \since 0.12
    */
    void setColorDisplayProfile(void *outputProfileA);
    /**
      Set a color display profile for the current document.

      \param name is the name of the display profile to set.

      \note This should be called before any rendering happens.

       \since 0.12
    */
    void setColorDisplayProfileName(const QString &name);
    /**
      Return the current RGB profile.

      \return a \c cmsHPROFILE of the LCMS library.

      \note The returned profile stays a property of poppler and shall NOT be closed by the user. It's
      existence is guaranteed for as long as this instance of the Document class is not deleted.

       \since 0.12
    */
    void *colorRgbProfile() const;
    /**
      Return the current display profile.

      \return a \c cmsHPROFILE of the LCMS library.

      \note The returned profile stays a property of poppler and shall NOT be closed by the user. It's
      existence is guaranteed for as long as this instance of the Document class is not deleted.

       \since 0.12
    */
    void *colorDisplayProfile() const;

    /**
       Load the document from a file on disk

       \param filePath the name (and path, if required) of the file to load
       \param ownerPassword the Latin1-encoded owner password to use in
       loading the file
       \param userPassword the Latin1-encoded user ("open") password
       to use in loading the file

       \return the loaded document, or NULL on error

       \note The caller owns the pointer to Document, and this should
       be deleted when no longer required.

       \warning The returning document may be locked if a password is required
       to open the file, and one is not provided (as the userPassword).
    */
    static Document *load(const QString &filePath, const QByteArray &ownerPassword = QByteArray(), const QByteArray &userPassword = QByteArray());

    /**
       Load the document from a device

       \param device the device of the data to load
       \param ownerPassword the Latin1-encoded owner password to use in
       loading the file
       \param userPassword the Latin1-encoded user ("open") password
       to use in loading the file

       \return the loaded document, or NULL on error

       \note The caller owns the pointer to Document, and this should
       be deleted when no longer required.

       \note The ownership of the device stays with the caller.

       \note if the file is on disk it is recommended to use the other load overload
       since it is less resource intensive

       \warning The returning document may be locked if a password is required
       to open the file, and one is not provided (as the userPassword).

       \since 0.85
    */
    static Document *load(QIODevice *device, const QByteArray &ownerPassword = QByteArray(), const QByteArray &userPassword = QByteArray());

    /**
       Load the document from memory

       \param fileContents the file contents. They are copied so there is no need
                           to keep the byte array around for the full life time of
                           the document.
       \param ownerPassword the Latin1-encoded owner password to use in
       loading the file
       \param userPassword the Latin1-encoded user ("open") password
       to use in loading the file

       \return the loaded document, or NULL on error

       \note The caller owns the pointer to Document, and this should
       be deleted when no longer required.

       \warning The returning document may be locked if a password is required
       to open the file, and one is not provided (as the userPassword).

       \since 0.6
    */
    static Document *loadFromData(const QByteArray &fileContents, const QByteArray &ownerPassword = QByteArray(), const QByteArray &userPassword = QByteArray());

    /**
       Get a specified Page

       Note that this follows the PDF standard of being zero based - if you
       want the first page, then you need an index of zero.

       The caller gets the ownership of the returned object.

       This function can return nullptr if for some reason the page can't be properly parsed.

       \param index the page number index

       \warning The Page object returned by this method internally stores a pointer
       to the document that it was created from.  This pointer will go stale if you
       delete the Document object.  Therefore the Document object needs to be kept alive
       as long as you want to use the Page object.
    */
    Page *page(int index) const;

    /**
       \overload


       The intent is that you can pass in a label like \c "ix" and
       get the page with that label (which might be in the table of
       contents), or pass in \c "1" and get the page that the user
       expects (which might not be the first page, if there is a
       title page and a table of contents).

       \param label the page label
    */
    Page *page(const QString &label) const;

    /**
       The number of pages in the document
    */
    int numPages() const;

    /**
       The type of mode that should be used by the application
       when the document is opened. Note that while this is
       called page mode, it is really viewer application mode.
    */
    PageMode pageMode() const;

    /**
       The layout that pages should be shown in when the document
       is first opened.  This basically describes how pages are
       shown relative to each other.
    */
    PageLayout pageLayout() const;

    /**
       The predominant reading order for text as supplied by
       the document's viewer preferences.

       \since 0.26
    */
    Qt::LayoutDirection textDirection() const;

    /**
       Provide the passwords required to unlock the document

       \param ownerPassword the Latin1-encoded owner password to use in
       loading the file
       \param userPassword the Latin1-encoded user ("open") password
       to use in loading the file
    */
    bool unlock(const QByteArray &ownerPassword, const QByteArray &userPassword);

    /**
       Determine if the document is locked
    */
    bool isLocked() const;

    /**
       The date associated with the document

       You would use this method with something like:
       \code
QDateTime created = m_doc->date("CreationDate");
QDateTime modified = m_doc->date("ModDate");
       \endcode

       The available dates are:
       - CreationDate: the date of creation of the document
       - ModDate: the date of the last change in the document

       \param type the type of date that is required
    */
    QDateTime date(const QString &type) const;

    /**
       Set the Info dict date entry specified by \param key to \param val

       \returns true on success, false on failure
    */
    bool setDate(const QString &key, const QDateTime &val);

    /**
       The date of the creation of the document
    */
    QDateTime creationDate() const;

    /**
       Set the creation date of the document to \param val

       \returns true on success, false on failure
    */
    bool setCreationDate(const QDateTime &val);

    /**
       The date of the last change in the document
    */
    QDateTime modificationDate() const;

    /**
       Set the modification date of the document to \param val

       \returns true on success, false on failure
    */
    bool setModificationDate(const QDateTime &val);

    /**
       Get specified information associated with the document

       You would use this method with something like:
       \code
QString title = m_doc->info("Title");
QString subject = m_doc->info("Subject");
       \endcode

       In addition to \c Title and \c Subject, other information that may
       be available include \c Author, \c Keywords, \c Creator and \c Producer.

       \param type the information that is required

       \sa infoKeys() to get a list of the available keys
    */
    QString info(const QString &type) const;

    /**
       Set the value of the document's Info dictionary entry specified by \param key to \param val

       \returns true on success, false on failure
    */
    bool setInfo(const QString &key, const QString &val);

    /**
       The title of the document
    */
    QString title() const;

    /**
       Set the title of the document to \param val

       \returns true on success, false on failure
    */
    bool setTitle(const QString &val);

    /**
       The author of the document
    */
    QString author() const;

    /**
       Set the author of the document to \param val

       \returns true on success, false on failure
    */
    bool setAuthor(const QString &val);

    /**
       The subject of the document
    */
    QString subject() const;

    /**
       Set the subject of the document to \param val

       \returns true on success, false on failure
    */
    bool setSubject(const QString &val);

    /**
       The keywords of the document
    */
    QString keywords() const;

    /**
       Set the keywords of the document to \param val

       \returns true on success, false on failure
    */
    bool setKeywords(const QString &val);

    /**
       The creator of the document
    */
    QString creator() const;

    /**
       Set the creator of the document to \param val

       \returns true on success, false on failure
    */
    bool setCreator(const QString &val);

    /**
       The producer of the document
    */
    QString producer() const;

    /**
       Set the producer of the document to \param val

       \returns true on success, false on failure
    */
    bool setProducer(const QString &val);

    /**
       Remove the document's Info dictionary

       \returns true on success, false on failure
    */
    bool removeInfo();

    /**
       Obtain a list of the available string information keys.
    */
    QStringList infoKeys() const;

    /**
       Test if the document is encrypted
    */
    bool isEncrypted() const;

    /**
       Test if the document is linearised

       In some cases, this is called "fast web view", since it
       is mostly an optimisation for viewing over the Web.
    */
    bool isLinearized() const;

    /**
       Test if the permissions on the document allow it to be
       printed
    */
    bool okToPrint() const;

    /**
       Test if the permissions on the document allow it to be
       printed at high resolution
    */
    bool okToPrintHighRes() const;

    /**
       Test if the permissions on the document allow it to be
       changed.

       \note depending on the type of change, it may be more
       appropriate to check other properties as well.
    */
    bool okToChange() const;

    /**
       Test if the permissions on the document allow the
       contents to be copied / extracted
    */
    bool okToCopy() const;

    /**
       Test if the permissions on the document allow annotations
       to be added or modified, and interactive form fields (including
       signature fields) to be completed.
    */
    bool okToAddNotes() const;

    /**
       Test if the permissions on the document allow interactive
       form fields (including signature fields) to be completed.

       \note this can be true even if okToAddNotes() is false - this
       means that only form completion is permitted.
    */
    bool okToFillForm() const;

    /**
       Test if the permissions on the document allow interactive
       form fields (including signature fields) to be set, created and
       modified
    */
    bool okToCreateFormFields() const;

    /**
       Test if the permissions on the document allow content extraction
       (text and perhaps other content) for accessibility usage (eg for
       a screen reader)
    */
    bool okToExtractForAccessibility() const;

    /**
       Test if the permissions on the document allow it to be
       "assembled" - insertion, rotation and deletion of pages;
       or creation of bookmarks and thumbnail images.

       \note this can be true even if okToChange() is false
    */
    bool okToAssemble() const;

    /**
       The version of the PDF specification that the document
       conforms to

       \param major an optional pointer to a variable where store the
       "major" number of the version
       \param minor an optional pointer to a variable where store the
       "minor" number of the version

       \deprecated Will be removed in the Qt6 interface.  Use the method
       returning a PdfVersion object instead!

       \since 0.12
    */
    Q_DECL_DEPRECATED void getPdfVersion(int *major, int *minor) const;

    /** \brief The version specification of a pdf file */
    struct PdfVersion
    {
        int major;
        int minor;
    };

    /**
       The version of the PDF specification that the document
       conforms to

       \since 21.08
    */
    PdfVersion getPdfVersion() const;

    /**
       The fonts within the PDF document.

       This is a shorthand for getting all the fonts at once.

       \note this can take a very long time to run with a large
       document. You may wish to use a FontIterator if you have more
       than say 20 pages

       \see newFontIterator()
    */
    QList<FontInfo> fonts() const;

    /**
       Creates a new FontIterator object for font scanning.

       The new iterator can be used for reading the font information of the
       document, reading page by page.

       The caller is responsible for the returned object, ie it should freed
       it when no more useful.

       \param startPage the initial page from which start reading fonts

       \see fonts()

       \since 0.12
    */
    FontIterator *newFontIterator(int startPage = 0) const;

    /**
       The font data if the font is an embedded one.

       \since 0.10
    */
    QByteArray fontData(const FontInfo &fi) const;

    /**
       The documents embedded within the PDF document.

       \note there are two types of embedded document - this call
       only accesses documents that are embedded at the document level.
    */
    QList<EmbeddedFile *> embeddedFiles() const;

    /**
       Whether there are any documents embedded in this PDF document.
    */
    bool hasEmbeddedFiles() const;

    /**
      Gets the table of contents (TOC) of the Document.

      The caller is responsible for the returned object.

      In the tree the tag name is the 'screen' name of the entry. A tag can have
      attributes. Here follows the list of tag attributes with meaning:
      - Destination: A string description of the referred destination
      - DestinationName: A 'named reference' to the viewport
      - ExternalFileName: A link to a external filename
      - Open: A bool value that tells whether the subbranch of the item is open or not

      Resolving the final destination for each item can be done in the following way:
      - first, checking for 'Destination': if not empty, then a LinkDestination
        can be constructed straight with it
      - as second step, if the 'DestinationName' is not empty, then the destination
        can be resolved using linkDestination()

      Note also that if 'ExternalFileName' is not emtpy, then the destination refers
      to that document (and not to the current one).

      \returns the TOC, or NULL if the Document does not have one
    */
    Q_DECL_DEPRECATED QDomDocument *toc() const;

    /**
       Gets the outline of the document

       \returns a vector of outline items, empty if there are none

       \since 0.74
    **/
    QVector<OutlineItem> outline() const;

    /**
       Tries to resolve the named destination \p name.

       \note this operation starts a search through the whole document

       \returns a new LinkDestination object if the named destination was
       actually found, or NULL otherwise
    */
    LinkDestination *linkDestination(const QString &name);

    /**
      Sets the paper color

      \param color the new paper color
     */
    void setPaperColor(const QColor &color);
    /**
      The paper color

      The default color is white.
     */
    QColor paperColor() const;

    /**
     Sets the backend used to render the pages.

     \param backend the new rendering backend

     \since 0.6
     */
    void setRenderBackend(RenderBackend backend);
    /**
      The currently set render backend

      The default backend is \ref SplashBackend

      \since 0.6
     */
    RenderBackend renderBackend() const;

    /**
      The available rendering backends.

      \since 0.6
     */
    static QSet<RenderBackend> availableRenderBackends();

    /**
     Sets the render \p hint .

     \note some hints may not be supported by some rendering backends.

     \param on whether the flag should be added or removed.

     \since 0.6
     */
    void setRenderHint(RenderHint hint, bool on = true);
    /**
      The currently set render hints.

      \since 0.6
     */
    RenderHints renderHints() const;

    /**
      Gets a new PS converter for this document.

      The caller gets the ownership of the returned converter.

      \since 0.6
     */
    PSConverter *psConverter() const;

    /**
      Gets a new PDF converter for this document.

      The caller gets the ownership of the returned converter.

      \since 0.8
     */
    PDFConverter *pdfConverter() const;

    /**
      Gets the metadata stream contents

      \since 0.6
    */
    QString metadata() const;

    /**
       Test whether this document has "optional content".

       Optional content is used to optionally turn on (display)
       and turn off (not display) some elements of the document.
       The most common use of this is for layers in design
       applications, but it can be used for a range of things,
       such as not including some content in printing, and
       displaying content in the appropriate language.

       \since 0.8
    */
    bool hasOptionalContent() const;

    /**
       Itemviews model for optional content.

       The model is owned by the document.

       \since 0.8
    */
    OptContentModel *optionalContentModel();

    /**
       Resets the form with the details contained in the \p link.

       \since 24.07
    */
    void applyResetFormsLink(const LinkResetForm &link);

    /**
       Document-level JavaScript scripts.

       Returns the list of document level JavaScript scripts to be always
       executed before any other script.

       \since 0.10
    */
    QStringList scripts() const;

    /**
      Describes the flags for additional document actions i.e.
      for executing document scripts at different events.
      This flag is used by additionalAction method to return the
      particular Link.

      \since 24.07
    */
    enum DocumentAdditionalActionsType
    {
        CloseDocument, ///< Performed before closing the document
        SaveDocumentStart, ///< Performed before saving the document
        SaveDocumentFinish, ///< Performed after saving the document
        PrintDocumentStart, ///< Performed before printing the document
        PrintDocumentFinish, ///< Performed after printing the document
    };

    /**
      Returns the additional action of the given @p type for the document or
      @c 0 if no action has been defined.

      \since 24.07
    */
    Link *additionalAction(DocumentAdditionalActionsType type) const;

    /**
       The PDF identifiers.

       \param permanentId an optional pointer to a variable where store the
       permanent ID of the document
       \param updateId an optional pointer to a variable where store the
       update ID of the document

       \return whether the document has the IDs

       \since 0.16
    */
    bool getPdfId(QByteArray *permanentId, QByteArray *updateId) const;

    /**
       Returns the type of forms contained in the document

       \since 0.22
    */
    FormType formType() const;

    /**
       Returns the calculate order for forms (using their id)

       \since 0.53
    */
    QVector<int> formCalculateOrder() const;

    /**
     Returns the signatures of this document.

     Prefer to use this over getting the signatures for all the pages of the document
     since there are documents with signatures that don't belong to a given page

     \since 0.88
    */
    QVector<FormFieldSignature *> signatures() const;

    /**
     Returns whether the document's XRef table has been reconstructed or not

     \since 21.06
    */
    bool xrefWasReconstructed() const;

    /**
     Sets the document's XRef reconstruction callback, so whenever a XRef table
     reconstruction happens the callback will get triggered.

     \since 21.06
    */
    void setXRefReconstructedCallback(const std::function<void()> &callback);

    /**
       Destructor.
    */
    ~Document();

private:
    Q_DISABLE_COPY(Document)

    DocumentData *m_doc;

    explicit Document(DocumentData *dataA);
};

class BaseConverterPrivate;
class PSConverterPrivate;
class PDFConverterPrivate;
/**
   \brief Base converter.

   This is the base class for the converters.

   \since 0.8
*/
class POPPLER_QT5_EXPORT BaseConverter
{
    friend class Document;

public:
    /**
      Destructor.
    */
    virtual ~BaseConverter();

    /** Sets the output file name. You must set this or the output device. */
    void setOutputFileName(const QString &outputFileName);

    /**
     * Sets the output device. You must set this or the output file name.
     *
     * \since 0.8
     */
    void setOutputDevice(QIODevice *device);

    /**
      Does the conversion.

      \return whether the conversion succeeded
    */
    virtual bool convert() = 0;

    enum Error
    {
        NoError,
        FileLockedError,
        OpenOutputError,
        NotSupportedInputFileError
    };

    /**
      Returns the last error
      \since 0.12.1
    */
    Error lastError() const;

protected:
    /// \cond PRIVATE
    explicit BaseConverter(BaseConverterPrivate &dd);
    Q_DECLARE_PRIVATE(BaseConverter)
    BaseConverterPrivate *d_ptr;
    /// \endcond

private:
    Q_DISABLE_COPY(BaseConverter)
};

/**
   Converts a PDF to PS

   Sizes have to be in Points (1/72 inch)

   If you are using QPrinter you can get paper size by doing:
   \code
QPrinter dummy(QPrinter::PrinterResolution);
dummy.setFullPage(true);
dummy.setPageSize(myPageSize);
width = dummy.width();
height = dummy.height();
   \endcode

   \since 0.6
*/
class POPPLER_QT5_EXPORT PSConverter : public BaseConverter
{
    friend class Document;

public:
    /**
      Options for the PS export.

      \since 0.10
     */
    enum PSOption
    {
        Printing = 0x00000001, ///< The PS is generated for printing purposes
        StrictMargins = 0x00000002,
        ForceRasterization = 0x00000004,
        PrintToEPS = 0x00000008, ///< Output EPS instead of PS \since 0.20
        HideAnnotations = 0x00000010, ///< Don't print annotations \since 0.20
        ForceOverprintPreview = 0x00000020 ///< Force rasterized overprint preview during conversion \since 23.09
    };
    Q_DECLARE_FLAGS(PSOptions, PSOption)

    /**
      Destructor.
    */
    ~PSConverter() override;

    /** Sets the list of pages to print. Mandatory. */
    void setPageList(const QList<int> &pageList);

    /**
      Sets the title of the PS Document. Optional
    */
    void setTitle(const QString &title);

    /**
      Sets the horizontal DPI. Defaults to 72.0
    */
    void setHDPI(double hDPI);

    /**
      Sets the vertical DPI. Defaults to 72.0
    */
    void setVDPI(double vDPI);

    /**
      Sets the rotate. Defaults to not rotated
    */
    void setRotate(int rotate);

    /**
      Sets the output paper width. Has to be set.
    */
    void setPaperWidth(int paperWidth);

    /**
      Sets the output paper height. Has to be set.
    */
    void setPaperHeight(int paperHeight);

    /**
      Sets the output right margin. Defaults to 0
    */
    void setRightMargin(int marginRight);

    /**
      Sets the output bottom margin. Defaults to 0
    */
    void setBottomMargin(int marginBottom);

    /**
      Sets the output left margin. Defaults to 0
    */
    void setLeftMargin(int marginLeft);

    /**
      Sets the output top margin. Defaults to 0
    */
    void setTopMargin(int marginTop);

    /**
      Defines if margins have to be strictly followed (even if that
      means changing aspect ratio), or if the margins can be adapted
      to keep aspect ratio.

      Defaults to false.
    */
    void setStrictMargins(bool strictMargins);

    /**
      Defines if the page will be rasterized to an image with overprint
      preview enabled before printing.

      Defaults to false

      \since 23.09
    */
    void setForceOverprintPreview(bool forceOverprintPreview);

    /** Defines if the page will be rasterized to an image before printing. Defaults to false */
    void setForceRasterize(bool forceRasterize);

    /**
      Sets the options for the PS export.

      \since 0.10
     */
    void setPSOptions(PSOptions options);

    /**
      The currently set options for the PS export.

      The default flags are: Printing.

      \since 0.10
     */
    PSOptions psOptions() const;

    /**
      Sets a function that will be called each time a page is converted.

      The payload belongs to the caller.

      \since 0.16
     */
    void setPageConvertedCallback(void (*callback)(int page, void *payload), void *payload);

    bool convert() override;

private:
    Q_DECLARE_PRIVATE(PSConverter)
    Q_DISABLE_COPY(PSConverter)

    explicit PSConverter(DocumentData *document);
};

/**
   Converts a PDF to PDF (thus saves a copy of the document).

   \since 0.8
*/
class POPPLER_QT5_EXPORT PDFConverter : public BaseConverter
{
    friend class Document;

public:
    /**
      Options for the PDF export.
     */
    enum PDFOption
    {
        WithChanges = 0x00000001 ///< The changes done to the document are saved as well
    };
    Q_DECLARE_FLAGS(PDFOptions, PDFOption)

    /**
      Destructor.
    */
    ~PDFConverter() override;

    /**
      Sets the options for the PDF export.
     */
    void setPDFOptions(PDFOptions options);
    /**
      The currently set options for the PDF export.
     */
    PDFOptions pdfOptions() const;

    /**
     * Holds data for a new signature
     *  - Common Name of cert to sign (aka nickname)
     *  - password for the cert
     *  - page where to add the signature
     *  - rect for the signature annotation
     *  - text that will be shown inside the rect
     *  - font size and color
     *  - border width and color
     *  - background color
     * \since 21.01
     */
    class POPPLER_QT5_EXPORT NewSignatureData
    {
    public:
        NewSignatureData();
        ~NewSignatureData();
        NewSignatureData(const NewSignatureData &) = delete;
        NewSignatureData &operator=(const NewSignatureData &) = delete;

        QString certNickname() const;
        void setCertNickname(const QString &certNickname);

        QString password() const;
        void setPassword(const QString &password);

        int page() const;
        void setPage(int page);

        QRectF boundingRectangle() const;
        void setBoundingRectangle(const QRectF &rect);

        QString signatureText() const;
        void setSignatureText(const QString &text);

        /**
         * If this text is not empty, the signature representation
         * will split in two, with this text on the left and signatureText
         * on the right
         *
         * \since 21.06
         */
        QString signatureLeftText() const;
        void setSignatureLeftText(const QString &text);

        /**
         * Signature's property Reason.
         *
         * Default: an empty string.
         *
         * \since 21.10
         */
        QString reason() const;
        void setReason(const QString &reason);

        /**
         * Signature's property Location.
         *
         * Default: an empty string.
         *
         * \since 21.10
         */
        QString location() const;
        void setLocation(const QString &location);

        /**
         * Default: 10
         */
        double fontSize() const;
        void setFontSize(double fontSize);

        /**
         * Default: 20
         *
         * \since 21.06
         */
        double leftFontSize() const;
        void setLeftFontSize(double fontSize);

        /**
         * Default: red
         */
        QColor fontColor() const;
        void setFontColor(const QColor &color);

        /**
         * Default: red
         */
        QColor borderColor() const;
        void setBorderColor(const QColor &color);

        /**
         * border width in points
         *
         * Default: 1.5
         *
         * \since 21.05
         */
        double borderWidth() const;
        void setBorderWidth(double width);

        /**
         * Default: QColor(240, 240, 240)
         */
        QColor backgroundColor() const;
        void setBackgroundColor(const QColor &color);

        /**
         * Default: QUuid::createUuid().toString()
         */
        QString fieldPartialName() const;
        void setFieldPartialName(const QString &name);

        /**
         * Document owner password (needed if the document that is being signed is password protected)
         *
         * Default: no password
         *
         * \since 22.02
         */
        QByteArray documentOwnerPassword() const;
        void setDocumentOwnerPassword(const QByteArray &password);

        /**
         * Document user password (needed if the document that is being signed is password protected)
         *
         * Default: no password
         *
         * \since 22.02
         */
        QByteArray documentUserPassword() const;
        void setDocumentUserPassword(const QByteArray &password);

        /**
         * Filesystem path to an image file to be used as background
         * image for the signature annotation widget.
         *
         * Default: empty
         *
         * \since 22.02
         */
        QString imagePath() const;
        void setImagePath(const QString &path);

    private:
        struct NewSignatureDataPrivate;
        NewSignatureDataPrivate *const d;
    };

    /**
        Sign PDF at given Annotation / signature form

        \param data new signature data

        \return whether the signing succeeded

        \since 21.01
    */
    bool sign(const NewSignatureData &data);

    /**
     * \since 25.07
     *
     * BIC/SIC: Merge with SigningResult in poppler-converter and in poppler-annotation and poppler-form
     */
    enum SigningResult
    {
        SigningSuccess, ///< No error
        FieldAlreadySigned, ///< Trying to sign a field that is already signed
        GenericSigningError, ///< Unclassified error
        InternalError, ///< Unexpected error, likely a bug in poppler
        KeyMissing, ///< Key not found (Either the input key is not from the list or the available keys has changed underneath)
        WriteFailed, ///< Write failed (permissions, faulty disk, ...)
        UserCancelled, ///< User cancelled the process
        BadPassphrase, ///< User entered bad passphrase
    };

    /**
     * The last signing result, mostly relevant if \ref sign returns false
     * \since 25.07
     */
    SigningResult lastSigningResult() const;

    /**
     * A string with a string that might offer more details of the signing result failure
     * \note the string here is likely not super useful for end users, but might give more details to a trained supporter / bug triager
     * \since 25.07
     */
    ErrorString lastSigningErrorDetails() const;

    bool convert() override;

private:
    Q_DECLARE_PRIVATE(PDFConverter)
    Q_DISABLE_COPY(PDFConverter)

    explicit PDFConverter(DocumentData *document);
};

/**
   Conversion from PDF date string format to QDateTime
*/
POPPLER_QT5_EXPORT Q_DECL_DEPRECATED QDateTime convertDate(char *dateString);

/**
   Conversion from PDF date string format to QDateTime

   \since 0.64
*/
POPPLER_QT5_EXPORT QDateTime convertDate(const char *dateString);

/**
   Whether the color management functions are available.

   \since 0.12
*/
POPPLER_QT5_EXPORT bool isCmsAvailable();

/**
   Whether the overprint preview functionality is available.

   \since 0.22
*/
POPPLER_QT5_EXPORT bool isOverprintPreviewAvailable();

class SoundData;
/**
   Container class for a sound file in a PDF document.

    A sound can be either External (in that case should be loaded the file
   whose url is represented by url() ), or Embedded, and the player has to
   play the data contained in data().

   \since 0.6
*/
class POPPLER_QT5_EXPORT SoundObject
{
public:
    /**
       The type of sound
    */
    enum SoundType
    {
        External, ///< The real sound file is external
        Embedded ///< The sound is contained in the data
    };

    /**
       The encoding format used for the sound
    */
    enum SoundEncoding
    {
        Raw, ///< Raw encoding, with unspecified or unsigned values in the range [ 0, 2^B - 1 ]
        Signed, ///< Twos-complement values
        muLaw, ///< mu-law-encoded samples
        ALaw ///< A-law-encoded samples
    };

    /// \cond PRIVATE
    explicit SoundObject(Sound *popplersound);
    /// \endcond

    ~SoundObject();

    /**
       Is the sound embedded (SoundObject::Embedded) or external (SoundObject::External)?
    */
    SoundType soundType() const;

    /**
       The URL of the sound file to be played, in case of SoundObject::External
    */
    QString url() const;

    /**
       The data of the sound, in case of SoundObject::Embedded
    */
    QByteArray data() const;

    /**
       The sampling rate of the sound
    */
    double samplingRate() const;

    /**
       The number of sound channels to use to play the sound
    */
    int channels() const;

    /**
       The number of bits per sample value per channel
    */
    int bitsPerSample() const;

    /**
       The encoding used for the sound
    */
    SoundEncoding soundEncoding() const;

private:
    Q_DISABLE_COPY(SoundObject)

    SoundData *m_soundData;
};

class MovieData;
/**
   Container class for a movie object in a PDF document.

   \since 0.10
*/
class POPPLER_QT5_EXPORT MovieObject
{
    friend class AnnotationPrivate;

public:
    /**
       The play mode for playing the movie
    */
    enum PlayMode
    {
        PlayOnce, ///< Play the movie once, closing the movie controls at the end
        PlayOpen, ///< Like PlayOnce, but leaving the controls open
        PlayRepeat, ///< Play continuously until stopped
        PlayPalindrome ///< Play forward, then backward, then again foward and so on until stopped
    };

    ~MovieObject();

    /**
       The URL of the movie to be played
    */
    QString url() const;

    /**
       The size of the movie
    */
    QSize size() const;

    /**
       The rotation (either 0, 90, 180, or 270 degrees clockwise) for the movie,
    */
    int rotation() const;

    /**
       Whether show a bar with movie controls
    */
    bool showControls() const;

    /**
       How to play the movie
    */
    PlayMode playMode() const;

    /**
       Returns whether a poster image should be shown if the movie is not playing.
       \since 0.22
    */
    bool showPosterImage() const;

    /**
       Returns the poster image that should be shown if the movie is not playing.
       If the image is null but showImagePoster() returns @c true, the first frame of the movie
       should be used as poster image.
       \since 0.22
    */
    QImage posterImage() const;

private:
    /// \cond PRIVATE
    explicit MovieObject(AnnotMovie *ann);
    /// \endcond

    Q_DISABLE_COPY(MovieObject)

    MovieData *m_movieData;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::Page::PainterFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::Page::SearchFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::Document::RenderHints)
Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::PDFConverter::PDFOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::PSConverter::PSOptions)

#endif
