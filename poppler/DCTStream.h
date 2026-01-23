//========================================================================
//
// DCTStream.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2005 Jeff Muizelaar <jeff@infidigm.net>
// Copyright 2005 Martin Kretzschmar <martink@gnome.org>
// Copyright 2005-2007, 2009-2011, 2017, 2019, 2021, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright 2010 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright 2011 Daiki Ueno <ueno@unixuser.org>
// Copyright 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright 2020 Lluís Batlle i Rossell <viric@viric.name>
// Copyright 2025 Nelson Benítez León <nbenitezl@gmail.com>
// Copyright 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
//========================================================================

#ifndef DCTSTREAM_H
#define DCTSTREAM_H

#include <csetjmp>
#include "Object.h"
#include "Stream.h"

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

struct str_src_mgr
{
    struct jpeg_source_mgr pub;
    JOCTET buffer;
    Stream *str;
    int index;
};

struct str_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
    int width;
    int height;
};

class DCTStream : public OwnedFilterStream
{
public:
    DCTStream(std::unique_ptr<Stream> strA, int colorXformA, Dict *dict, int recursion);
    ~DCTStream() override;
    StreamKind getKind() const override { return strDCT; }
    [[nodiscard]] bool rewind() override;
    int getChar() override;
    int lookChar() override;
    std::optional<std::string> getPSFilter(int psLevel, const char *indent) override;
    bool isBinary(bool last = true) const override;

private:
    void init();

    bool hasGetChars() override { return true; }
    bool readLine();
    int getChars(int nChars, unsigned char *buffer) override;

    int colorXform;
    JSAMPLE *current;
    JSAMPLE *limit;
    struct jpeg_decompress_struct cinfo;
    struct str_error_mgr err;
    struct str_src_mgr src;
    JSAMPARRAY row_buffer;
};

#endif
