#include "goo/gmem.h"
#include "UTF.h"

int UTF16toUCS4(const Unicode *utf16, int utf16Len, Unicode **ucs4)
{
  int i, n, len;
  Unicode *u;

  // count characters
  len = 0;
  for (i = 0; i < utf16Len; i++) {
    if (utf16[i] >= 0xd800 && utf16[i] < 0xdc00 && i + 1 < utf16Len &&
        utf16[i+1] >= 0xdc00 && utf16[i+1] < 0xe000) {
      i++; /* surrogate pair */
    }
    len++;
  }
  if (ucs4 == NULL)
    return len;

  u = (Unicode*)gmallocn(len, sizeof(Unicode));
  n = 0;
  // convert string
  for (i = 0; i < utf16Len; i++) {
    if (utf16[i] >= 0xd800 && utf16[i] < 0xdc00) { /* surrogate pair */
      if (i + 1 < utf16Len && utf16[i+1] >= 0xdc00 && utf16[i+1] < 0xe000) {
	/* next code is a low surrogate */
	u[n] = (((utf16[i] & 0x3ff) << 10) | (utf16[i+1] & 0x3ff)) + 0x10000;
	++i;
      } else {
	/* missing low surrogate
	   replace it with REPLACEMENT CHARACTER (U+FFFD) */
	u[n] = 0xfffd;
      }
    } else if (utf16[i] >= 0xdc00 && utf16[i] < 0xe000) {
      /* invalid low surrogate
	 replace it with REPLACEMENT CHARACTER (U+FFFD) */
      u[n] = 0xfffd;
    } else {
      u[n] = utf16[i];
    }
    n++;
  }
  *ucs4 = u;
  return len;
}

