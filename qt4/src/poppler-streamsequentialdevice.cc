/* poppler-streamdevice.cc: Qt4 interface to poppler
 * Copyright (C) 2012, Guillermo A. Amaral B. <gamaral@kde.org>
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

#include "poppler-streamsequentialdevice-private.h"

#include "Object.h"
#include "Stream.h"

namespace Poppler {

StreamSequentialDevice::StreamSequentialDevice(Stream *stream, QObject *parent)
    : QIODevice(parent)
    , m_stream(stream)
{
	Q_ASSERT(m_stream && "Invalid stream assigned.");
	m_stream->incRef();
	m_stream->reset();
	open(QIODevice::ReadOnly);
}

StreamSequentialDevice::~StreamSequentialDevice()
{
	m_stream->decRef();
	m_stream = 0;
}

void
StreamSequentialDevice::close()
{
	m_stream->close();
	QIODevice::close();
}

qint64
StreamSequentialDevice::readData(char *data, qint64 maxSize)
{
	return m_stream->doGetChars(maxSize, reinterpret_cast<Guchar*>(data));
}

}

