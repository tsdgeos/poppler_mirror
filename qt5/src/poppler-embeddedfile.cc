/* poppler-document.cc: qt interface to poppler
 * Copyright (C) 2005, 2008, 2009, 2012, 2013, 2018, 2022, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2005, Brad Hards <bradh@frogmouth.net>
 * Copyright (C) 2008, 2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
 * Copyright (C) 2023, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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

#include "poppler-qt5.h"

#include <QtCore/QString>
#include <QtCore/QDateTime>

#include "Object.h"
#include "Stream.h"
#include "Catalog.h"

#include "poppler-private.h"
#include "poppler-embeddedfile-private.h"

namespace Poppler {

EmbeddedFileData::EmbeddedFileData(std::unique_ptr<FileSpec> &&fs) : filespec(std::move(fs)) { }

EmbFile *EmbeddedFileData::embFile() const
{
    return filespec->isOk() ? filespec->getEmbeddedFile() : nullptr;
}

EmbeddedFile::EmbeddedFile(EmbFile *embfile) : m_embeddedFile(nullptr)
{
    assert(!"You must not use this private constructor!");
}

EmbeddedFile::EmbeddedFile(EmbeddedFileData &dd) : m_embeddedFile(&dd) { }

EmbeddedFile::~EmbeddedFile()
{
    delete m_embeddedFile;
}

QString EmbeddedFile::name() const
{
    const GooString *goo = m_embeddedFile->filespec->getFileName();
    return goo ? UnicodeParsedString(goo) : QString();
}

QString EmbeddedFile::description() const
{
    const GooString *goo = m_embeddedFile->filespec->getDescription();
    return goo ? UnicodeParsedString(goo) : QString();
}

int EmbeddedFile::size() const
{
    return m_embeddedFile->embFile() ? m_embeddedFile->embFile()->size() : -1;
}

QDateTime EmbeddedFile::modDate() const
{
    const GooString *goo = m_embeddedFile->embFile() ? m_embeddedFile->embFile()->modDate() : nullptr;
    return goo ? convertDate(goo->c_str()) : QDateTime();
}

QDateTime EmbeddedFile::createDate() const
{
    const GooString *goo = m_embeddedFile->embFile() ? m_embeddedFile->embFile()->createDate() : nullptr;
    return goo ? convertDate(goo->c_str()) : QDateTime();
}

QByteArray EmbeddedFile::checksum() const
{
    const GooString *goo = m_embeddedFile->embFile() ? m_embeddedFile->embFile()->checksum() : nullptr;
    return goo ? QByteArray::fromRawData(goo->c_str(), goo->getLength()) : QByteArray();
}

QString EmbeddedFile::mimeType() const
{
    const GooString *goo = m_embeddedFile->embFile() ? m_embeddedFile->embFile()->mimeType() : nullptr;
    return goo ? QString(goo->c_str()) : QString();
}

QByteArray EmbeddedFile::data()
{
    if (!isValid()) {
        return QByteArray();
    }
    Stream *stream = m_embeddedFile->embFile() ? m_embeddedFile->embFile()->stream() : nullptr;
    if (!stream) {
        return QByteArray();
    }

    if (!stream->reset()) {
        return QByteArray();
    }
    auto data = stream->toUnsignedChars();
    return QByteArray(reinterpret_cast<const char *>(data.data()), data.size());
}

bool EmbeddedFile::isValid() const
{
    return m_embeddedFile->filespec->isOk();
}

}
