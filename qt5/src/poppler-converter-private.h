/* poppler-converter-private.h: Qt interface to poppler
 * Copyright (C) 2007, 2009, 2018, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2008, Pino Toscano <pino@kde.org>
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

#ifndef POPPLER_QT5_CONVERTER_PRIVATE_H
#define POPPLER_QT5_CONVERTER_PRIVATE_H

#include "poppler-qt5.h"
#include <QtCore/QString>

class QIODevice;

namespace Poppler {

class DocumentData;

class BaseConverterPrivate
{
public:
    BaseConverterPrivate();
    virtual ~BaseConverterPrivate();

    BaseConverterPrivate(const BaseConverterPrivate &) = delete;
    BaseConverterPrivate &operator=(const BaseConverterPrivate &) = delete;

    QIODevice *openDevice();
    void closeDevice();

    DocumentData *document;
    QString outputFileName;
    QIODevice *iodev;
    bool ownIodev : 1;
    BaseConverter::Error lastError;
    PDFConverter::SigningResult lastSigningResult;
    Poppler::ErrorString lastSigningErrorDetails;
};

}

#endif
