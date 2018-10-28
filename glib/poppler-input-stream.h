/* poppler-input-stream.h: glib interface to poppler
 *
 * Copyright (C) 2012 Carlos Garcia Campos <carlosgc@gnome.org>
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
#include <Object.h>
#include <Stream.h>

#define inputStreamBufSize 1024

class PopplerInputStream: public BaseStream {
public:

  PopplerInputStream(GInputStream *inputStream, GCancellable *cancellableA,
                     Goffset startA, bool limitedA, Goffset lengthA, Object &&dictA);
  ~PopplerInputStream();
  BaseStream *copy() override;
  Stream *makeSubStream(Goffset start, bool limited,
                        Goffset lengthA, Object &&dictA) override;
  StreamKind getKind() override { return strWeird; }
  void reset() override;
  void close() override;
  int getChar() override
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  int lookChar() override
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  Goffset getPos() override { return bufPos + (bufPtr - buf); }
  void setPos(Goffset pos, int dir = 0) override;
  Goffset getStart() override { return start; }
  void moveStart(Goffset delta) override;

  int getUnfilteredChar() override { return getChar(); }
  void unfilteredReset() override { reset(); }

private:

  bool fillBuf();

  bool hasGetChars() override { return true; }
  int getChars(int nChars, Guchar *buffer) override;

  GInputStream *inputStream;
  GCancellable *cancellable;
  Goffset start;
  bool limited;
  char buf[inputStreamBufSize];
  char *bufPtr;
  char *bufEnd;
  Goffset bufPos;
  int savePos;
  bool saved;
};

#endif /* __GI_SCANNER__ */

#endif /* __POPPLER_INPUT_STREAM_H__ */
