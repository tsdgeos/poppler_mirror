/* poppler-streamdevice-private.h: Qt4 interface to poppler
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

#ifndef POPPLER_STREAMSEQUENTIALDEVICE_PRIVATE_H
#define POPPLER_STREAMSEQUENTIALDEVICE_PRIVATE_H

#include <QtCore/QIODevice>

class Stream;

namespace Poppler {

class StreamSequentialDevice : public QIODevice
{
  public:
    StreamSequentialDevice(Stream *stream, QObject *parent = 0);
    virtual ~StreamSequentialDevice();

    virtual void close();

    virtual bool isSequential() const
      { return true; }

  protected:
    virtual qint64 readData(char *data, qint64 maxSize);
    inline virtual qint64 writeData(const char *, qint64)
      { return 0; }

  private:
    Q_DISABLE_COPY(StreamSequentialDevice);
    Stream *m_stream;
};

}

#endif
