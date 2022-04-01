/* poppler-cached-file-loader.h: glib interface to poppler
 *
 * Copyright (C) 2012 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2022 Albert Astals Cid <aacid@kde.org>
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
#include "poppler-cached-file-loader.h"

PopplerCachedFileLoader::PopplerCachedFileLoader(GInputStream *inputStreamA, GCancellable *cancellableA, goffset lengthA)
{
    inputStream = (GInputStream *)g_object_ref(inputStreamA);
    cancellable = cancellableA ? (GCancellable *)g_object_ref(cancellableA) : nullptr;
    length = lengthA;
    cachedFile = nullptr;
}

PopplerCachedFileLoader::~PopplerCachedFileLoader()
{
    g_object_unref(inputStream);
    if (cancellable) {
        g_object_unref(cancellable);
    }
}

size_t PopplerCachedFileLoader::init(CachedFile *cachedFileA)
{
    size_t size;
    gssize bytesRead;
    char buf[CachedFileChunkSize];

    cachedFile = cachedFileA;

    if (length != (goffset)-1) {
        return length;
    }

    if (G_IS_FILE_INPUT_STREAM(inputStream)) {
        GFileInfo *info;

        info = g_file_input_stream_query_info(G_FILE_INPUT_STREAM(inputStream), G_FILE_ATTRIBUTE_STANDARD_SIZE, cancellable, nullptr);
        if (!info) {
            error(errInternal, -1, "Failed to get size.");
            return (size_t)-1;
        }

        length = g_file_info_get_size(info);
        g_object_unref(info);

        return length;
    }

    // Unknown stream length, read the whole stream and return the size.
    CachedFileWriter writer = CachedFileWriter(cachedFile, nullptr);
    size = 0;
    do {
        bytesRead = g_input_stream_read(inputStream, buf, CachedFileChunkSize, cancellable, nullptr);
        if (bytesRead == -1) {
            break;
        }

        writer.write(buf, bytesRead);
        size += bytesRead;
    } while (bytesRead > 0);

    return size;
}

int PopplerCachedFileLoader::load(const std::vector<ByteRange> &ranges, CachedFileWriter *writer)
{
    char buf[CachedFileChunkSize];
    gssize bytesRead;
    size_t rangeBytesRead, bytesToRead;

    if (length == (goffset)-1) {
        return 0;
    }

    for (const ByteRange &range : ranges) {
        bytesToRead = MIN(CachedFileChunkSize, range.length);
        rangeBytesRead = 0;
        g_seekable_seek(G_SEEKABLE(inputStream), range.offset, G_SEEK_SET, cancellable, nullptr);
        do {
            bytesRead = g_input_stream_read(inputStream, buf, bytesToRead, cancellable, nullptr);
            if (bytesRead == -1) {
                return -1;
            }

            writer->write(buf, bytesRead);
            rangeBytesRead += bytesRead;
            bytesToRead = range.length - rangeBytesRead;
        } while (bytesRead > 0 && bytesToRead > 0);
    }

    return 0;
}
