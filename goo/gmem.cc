/*
 * gmem.c
 *
 * Memory routines with out-of-memory checking.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2007-2010, 2012 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2008 Jonathan Kew <jonathan_kew@sil.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "gmem.h"

inline static void *gmalloc(size_t size, bool checkoverflow) {
  void *p;

  if (size == 0) {
    return nullptr;
  }
  if (!(p = malloc(size))) {
    fprintf(stderr, "Out of memory\n");
    if (checkoverflow) return nullptr;
    else exit(1);
  }
  return p;
}

void *gmalloc(size_t size) {
  return gmalloc(size, false);
}

void *gmalloc_checkoverflow(size_t size) {
  return gmalloc(size, true);
}

inline static void *grealloc(void *p, size_t size, bool checkoverflow) {
  void *q;

  if (size == 0) {
    if (p) {
      free(p);
    }
    return nullptr;
  }
  if (p) {
    q = realloc(p, size);
  } else {
    q = malloc(size);
  }
  if (!q) {
    fprintf(stderr, "Out of memory\n");
    if (checkoverflow) return nullptr;
    else exit(1);
  }
  return q;
}

void *grealloc(void *p, size_t size) {
  return grealloc(p, size, false);
}

void *grealloc_checkoverflow(void *p, size_t size) {
  return grealloc(p, size, true);
}

inline static void *gmallocn(int nObjs, int objSize, bool checkoverflow) {
  if (nObjs == 0) {
    return nullptr;
  }
  if (objSize <= 0 || nObjs < 0 || nObjs >= INT_MAX / objSize) {
    fprintf(stderr, "Bogus memory allocation size\n");
    if (checkoverflow) return nullptr;
    else exit(1);
  }
  const int n = nObjs * objSize;
  return gmalloc(n, checkoverflow);
}

void *gmallocn(int nObjs, int objSize) {
  return gmallocn(nObjs, objSize, false);
}

void *gmallocn_checkoverflow(int nObjs, int objSize) {
  return gmallocn(nObjs, objSize, true);
}

inline static void *gmallocn3(int a, int b, int c, bool checkoverflow) {
  int n = a * b;
  if (b <= 0 || a < 0 || a >= INT_MAX / b) {
    fprintf(stderr, "Bogus memory allocation size\n");
    if (checkoverflow) return nullptr;
    else exit(1);
  }
  return gmallocn(n, c, checkoverflow);
}

void *gmallocn3(int a, int b, int c) {
  return gmallocn3(a, b, c, false);
}

void *gmallocn3_checkoverflow(int a, int b, int c) {
  return gmallocn3(a, b, c, true);
}

inline static void *greallocn(void *p, int nObjs, int objSize, bool checkoverflow) {
  if (nObjs == 0) {
    if (p) {
      gfree(p);
    }
    return nullptr;
  }
  if (objSize <= 0 || nObjs < 0 || nObjs >= INT_MAX / objSize) {
    fprintf(stderr, "Bogus memory allocation size\n");
    if (checkoverflow) {
      gfree(p);
      return nullptr;
    } else {
      exit(1);
    }
  }
  const int n = nObjs * objSize;
  return grealloc(p, n, checkoverflow);
}

void *greallocn(void *p, int nObjs, int objSize) {
  return greallocn(p, nObjs, objSize, false);
}

void *greallocn_checkoverflow(void *p, int nObjs, int objSize) {
  return greallocn(p, nObjs, objSize, true);
}

void gfree(void *p) {
  if (p) {
    free(p);
  }
}

char *copyString(const char *s) {
  char *s1;

  s1 = (char *)gmalloc(strlen(s) + 1);
  strcpy(s1, s);
  return s1;
}

char *gstrndup(const char *s, size_t n) {
  char *s1 = (char*)gmalloc(n + 1); /* cannot return NULL for size > 0 */
  s1[n] = '\0';
  memcpy(s1, s, n);
  return s1;
}
