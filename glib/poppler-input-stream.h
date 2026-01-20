/* poppler-input-stream.h: glib interface to poppler
 *
 * Copyright (C) 2012 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2019 Albert Astals Cid <aacid@kde.org>
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

#ifndef __POPPLER_INPUT_STREAM_H__
#define __POPPLER_INPUT_STREAM_H__

#include <gio/gio.h>
#ifndef __GI_SCANNER__
#    include <Object.h>
#    include <Stream.h>

class PopplerInputStream : public BaseSeekInputStream
{
public:
    PopplerInputStream(GInputStream *inputStream, GCancellable *cancellableA, Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA);
    ~PopplerInputStream() override;
    std::unique_ptr<BaseStream> copy() override;
    std::unique_ptr<Stream> makeSubStream(Goffset start, bool limited, Goffset lengthA, Object &&dictA) override;

private:
    Goffset currentPos() const override;
    void setCurrentPos(Goffset offset) override;
    Goffset read(char *buffer, Goffset count) override;

    GInputStream *inputStream;
    GCancellable *cancellable;
};

#endif /* __GI_SCANNER__ */

#endif /* __POPPLER_INPUT_STREAM_H__ */
