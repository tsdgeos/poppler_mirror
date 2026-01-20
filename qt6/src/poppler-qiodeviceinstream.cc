/* poppler-qiodeviceinstream.cc: Qt6 interface to poppler
 * Copyright (C) 2019 Alexander Volkov <a.volkov@rusbitech.ru>
 * Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
 * Copyright (C) 2025 Albert Astals Cid <aacid@kde.org>
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

#include "poppler-qiodeviceinstream-private.h"

#include <QtCore/QIODevice>

namespace Poppler {

QIODeviceInStream::QIODeviceInStream(QIODevice *device, Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA) : BaseSeekInputStream(startA, limitedA, lengthA, std::move(dictA)), m_device(device) { }

QIODeviceInStream::~QIODeviceInStream()
{
    close();
}

std::unique_ptr<BaseStream> QIODeviceInStream::copy()
{
    return std::make_unique<QIODeviceInStream>(m_device, start, limited, length, dict.copy());
}

std::unique_ptr<Stream> QIODeviceInStream::makeSubStream(Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA)
{
    return std::make_unique<QIODeviceInStream>(m_device, startA, limitedA, lengthA, std::move(dictA));
}

Goffset QIODeviceInStream::currentPos() const
{
    return m_device->pos();
}

void QIODeviceInStream::setCurrentPos(Goffset offset)
{
    m_device->seek(offset);
}

Goffset QIODeviceInStream::read(char *buffer, Goffset count)
{
    return m_device->read(buffer, count);
}

}
