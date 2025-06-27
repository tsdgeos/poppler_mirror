/* poppler-converter.h: qt interface to poppler
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
 * Copyright (C) 2020, 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
 * Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>.
 * Copyright (C) 2021 Mahmoud Khalil <mahmoudkhalil11@gmail.com>
 * Copyright (C) 2021 Georgiy Sgibnev <georgiy@sgibnev.com>. Work sponsored by lab50.net.
 * Copyright (C) 2022 Martin <martinbts@gmx.net>
 * Copyright (C) 2023 Kevin Ottens <kevin.ottens@enioka.com>. Work sponsored by De Bortoli Wines
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

#ifndef __POPPLER_CONVERTER_H__
#define __POPPLER_CONVERTER_H__

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QVariant>

#include "poppler-export.h"

namespace Poppler {

/**
 * The various types of error strings.
 *
 * \since 25.07
 */
enum class ErrorStringType
{
    /**The string should be treated like a error cod. It could be a hex code, a position in the poppler sources or something similar*/
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

class DocumentData;

class BaseConverterPrivate;
class PSConverterPrivate;
class PDFConverterPrivate;

/**
   \brief Base converter.

   This is the base class for the converters.
*/
class POPPLER_QT6_EXPORT BaseConverter
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
*/
class POPPLER_QT6_EXPORT PSConverter : public BaseConverter
{
    friend class Document;

public:
    /**
      Options for the PS export.
     */
    enum PSOption
    {
        Printing = 0x00000001, ///< The PS is generated for printing purposes
        StrictMargins = 0x00000002,
        ForceRasterization = 0x00000004,
        PrintToEPS = 0x00000008, ///< Output EPS instead of PS
        HideAnnotations = 0x00000010, ///< Don't print annotations
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
     */
    void setPSOptions(PSOptions options);

    /**
      The currently set options for the PS export.

      The default flags are: Printing.
     */
    PSOptions psOptions() const;

    /**
      Sets a function that will be called each time a page is converted.

      The payload belongs to the caller.
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
*/
class POPPLER_QT6_EXPORT PDFConverter : public BaseConverter
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
    class POPPLER_QT6_EXPORT NewSignatureData
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

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::PDFConverter::PDFOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Poppler::PSConverter::PSOptions)

#endif
