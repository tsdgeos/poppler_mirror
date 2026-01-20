/* poppler-input-stream.cc: glib interface to poppler
 *
 * Copyright (C) 2012 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
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
#include "poppler-input-stream.h"

PopplerInputStream::PopplerInputStream(GInputStream *inputStreamA, GCancellable *cancellableA, Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA) : BaseSeekInputStream(startA, limitedA, lengthA, std::move(dictA))
{
    inputStream = (GInputStream *)g_object_ref(inputStreamA);
    cancellable = cancellableA ? (GCancellable *)g_object_ref(cancellableA) : nullptr;
}

PopplerInputStream::~PopplerInputStream()
{
    close();
    g_object_unref(inputStream);
    if (cancellable) {
        g_object_unref(cancellable);
    }
}

std::unique_ptr<BaseStream> PopplerInputStream::copy()
{
    return std::make_unique<PopplerInputStream>(inputStream, cancellable, start, limited, length, dict.copy());
}

std::unique_ptr<Stream> PopplerInputStream::makeSubStream(Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA)
{
    return std::make_unique<PopplerInputStream>(inputStream, cancellable, startA, limitedA, lengthA, std::move(dictA));
}

Goffset PopplerInputStream::currentPos() const
{
    GSeekable *seekable = G_SEEKABLE(inputStream);
    return g_seekable_tell(seekable);
}

void PopplerInputStream::setCurrentPos(Goffset offset)
{
    GSeekable *seekable = G_SEEKABLE(inputStream);
    g_seekable_seek(seekable, offset, G_SEEK_SET, cancellable, nullptr);
}

Goffset PopplerInputStream::read(char *buffer, Goffset count)
{
    return g_input_stream_read(inputStream, buffer, count, cancellable, nullptr);
}
