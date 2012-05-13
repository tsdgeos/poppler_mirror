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
                     Guint startA, GBool limitedA, Guint lengthA, Object *dictA);
  virtual ~PopplerInputStream();
  virtual Stream *makeSubStream(Guint start, GBool limited,
                                Guint lengthA, Object *dictA);
  virtual StreamKind getKind() { return strWeird; }
  virtual void reset();
  virtual void close();
  virtual int getChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr++ & 0xff); }
  virtual int lookChar()
    { return (bufPtr >= bufEnd && !fillBuf()) ? EOF : (*bufPtr & 0xff); }
  virtual int getPos() { return bufPos + (bufPtr - buf); }
  virtual void setPos(Guint pos, int dir = 0);
  virtual Guint getStart() { return start; }
  virtual void moveStart(int delta);

  virtual int getUnfilteredChar() { return getChar(); }
  virtual void unfilteredReset() { reset(); }

private:

  GBool fillBuf();

  virtual GBool hasGetChars() { return true; }
  virtual int getChars(int nChars, Guchar *buffer);

  GInputStream *inputStream;
  GCancellable *cancellable;
  Guint start;
  GBool limited;
  char buf[inputStreamBufSize];
  char *bufPtr;
  char *bufEnd;
  Guint bufPos;
  int savePos;
  GBool saved;
};

#endif /* __GI_SCANNER__ */

#endif /* __POPPLER_INPUT_STREAM_H__ */
