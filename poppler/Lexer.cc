//========================================================================
//
// Lexer.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006-2010, 2012-2014, 2017-2019, 2024-2026 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright (C) 2010 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2012, 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2023, 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2023 Even Rouault <even.rouault@mines-paris.org>
// Copyright (C) 2023 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <climits>
#include <cctype>
#include "Lexer.h"
#include "Error.h"
#include "UTF.h"
#include "XRef.h"

//------------------------------------------------------------------------

// A '1' in this array means the character is white space.  A '1' or
// '2' means the character ends a name or command.
static const char specialChars[256] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, // 0x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1x
    1, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, // 2x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, // 3x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, // 5x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, // 7x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9x
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // ax
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // bx
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // cx
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // dx
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // ex
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // fx
};

static const int IntegerSafeLimit = (INT_MAX - 9) / 10;
static const long long LongLongSafeLimit = (LLONG_MAX - 9) / 10;

//------------------------------------------------------------------------
// Lexer
//------------------------------------------------------------------------

Lexer::Lexer(XRef *xrefA, std::unique_ptr<Stream> &&str)
{
    lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
    xref = xrefA;

    curStr = Object(std::move(str));
    streams = new Array(xref);
    streams->add(curStr.copy());
    strPtr = 0;
    freeArray = true;
    (void)curStr.streamRewind();
}

Lexer::Lexer(XRef *xrefA, Object *obj)
{
    lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
    xref = xrefA;

    if (obj->isStream()) {
        streams = new Array(xref);
        freeArray = true;
        streams->add(obj->copy());
    } else {
        streams = obj->getArray();
        freeArray = false;
    }
    strPtr = 0;
    if (streams->getLength() > 0) {
        curStr = streams->get(strPtr);
        if (curStr.isStream()) {
            (void)curStr.streamRewind();
        }
    }
}

Lexer::~Lexer()
{
    if (curStr.isStream()) {
        curStr.streamClose();
    }
    if (freeArray) {
        delete streams;
    }
}

int Lexer::getChar(bool comesFromLook)
{
    int c;

    if (LOOK_VALUE_NOT_CACHED != lookCharLastValueCached) {
        c = lookCharLastValueCached;
        lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
        return c;
    }

    c = EOF;
    while (curStr.isStream() && (c = curStr.streamGetChar()) == EOF) {
        if (comesFromLook) {
            return EOF;
        }
        curStr.streamClose();
        curStr = Object();
        ++strPtr;
        if (strPtr < streams->getLength()) {
            curStr = streams->get(strPtr);
            if (curStr.isStream()) {
                if (!curStr.streamRewind()) {
                    return EOF;
                }
            }
        }
    }
    return c;
}

int Lexer::lookChar()
{

    if (LOOK_VALUE_NOT_CACHED != lookCharLastValueCached) {
        return lookCharLastValueCached;
    }
    lookCharLastValueCached = getChar(true);
    if (lookCharLastValueCached == EOF) {
        lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
        return EOF;
    }
    return lookCharLastValueCached;
}

Object Lexer::getObj(int objNum)
{
    char tokBuf[tokBufSize]; // temporary token buffer
    char *p;
    int c, c2;
    bool comment, neg, done, overflownInteger, overflownLongLong;
    int numParen;
    int xi;
    long long xll = 0;
    double xf = 0, scale;
    int n, m;

    // skip whitespace and comments
    comment = false;
    while (true) {
        if ((c = getChar()) == EOF) {
            return Object::eof();
        }
        if (comment) {
            if (c == '\r' || c == '\n') {
                comment = false;
            }
        } else if (c == '%') {
            comment = true;
        } else if (specialChars[c] != 1) {
            break;
        }
    }

    // start reading token
    switch (c) {

    // number
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '+':
    case '-':
    case '.':
        overflownInteger = false;
        overflownLongLong = false;
        neg = false;
        xi = 0;
        if (c == '-') {
            neg = true;
        } else if (c == '.') {
            goto doReal;
        } else if (c != '+') {
            xi = c - '0';
        }
        while (true) {
            c = lookChar();
            if (isdigit(c)) {
                getChar();
                if (unlikely(overflownLongLong)) {
                    xf = xf * 10.0 + (c - '0');
                } else if (unlikely(overflownInteger)) {
                    if (unlikely(xll > LongLongSafeLimit) && (xll > (LLONG_MAX - (c - '0')) / 10)) {
                        overflownLongLong = true;
                        xf = xll * 10.0 + (c - '0');
                    } else {
                        xll = xll * 10 + (c - '0');
                    }
                } else {
                    if (unlikely(xi > IntegerSafeLimit) && (xi > (INT_MAX - (c - '0')) / 10.0)) {
                        overflownInteger = true;
                        xll = xi * 10LL + (c - '0');
                    } else {
                        xi = xi * 10 + (c - '0');
                    }
                }
            } else if (c == '.') {
                getChar();
                goto doReal;
            } else {
                break;
            }
        }
        if (neg) {
            xi = -xi;
            xll = -xll;
            xf = -xf;
        }
        if (unlikely(overflownInteger)) {
            if (overflownLongLong) {
                return Object(xf);
            }
            if (unlikely(xll == INT_MIN)) {
                return Object(INT_MIN);
            }
            return Object(xll);
        }
        return Object(xi);

        break;
    doReal:
        if (likely(!overflownInteger)) {
            xf = xi;
        } else if (!overflownLongLong) {
            xf = xll;
        }
        scale = 0.1;
        while (true) {
            c = lookChar();
            if (c == '-') {
                // ignore minus signs in the middle of numbers to match
                // Adobe's behavior
                error(errSyntaxWarning, getPos(), "Badly formatted number");
                getChar();
                continue;
            }
            if (!isdigit(c)) {
                break;
            }
            getChar();
            xf = xf + scale * (c - '0');
            scale *= 0.1;
        }
        if (neg) {
            xf = -xf;
        }
        return Object(xf);
        break;

    // string
    case '(': {
        p = tokBuf;
        n = 0;
        numParen = 1;
        done = false;
        std::string s;
        do {
            c2 = EOF;
            switch (c = getChar()) {
            // Should technically error on newlines too, but this breaks
            // some PDF files, e.g., ones from Photoshop.
            case EOF:
                error(errSyntaxError, getPos(), "Unterminated string");
                done = true;
                break;

            case '(':
                ++numParen;
                c2 = c;
                break;

            case ')':
                if (--numParen == 0) {
                    done = true;
                } else {
                    c2 = c;
                }
                break;

            case '\\':
                switch (c = getChar()) {
                case 'n':
                    c2 = '\n';
                    break;
                case 'r':
                    c2 = '\r';
                    break;
                case 't':
                    c2 = '\t';
                    break;
                case 'b':
                    c2 = '\b';
                    break;
                case 'f':
                    c2 = '\f';
                    break;
                case '\\':
                case '(':
                case ')':
                    c2 = c;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    c2 = c - '0';
                    c = lookChar();
                    if (c >= '0' && c <= '7') {
                        getChar();
                        c2 = (c2 << 3) + (c - '0');
                        c = lookChar();
                        if (c >= '0' && c <= '7') {
                            getChar();
                            c2 = (c2 << 3) + (c - '0');
                        }
                    }
                    break;
                case '\r':
                    c = lookChar();
                    if (c == '\n') {
                        getChar();
                    }
                    break;
                case '\n':
                    break;
                case EOF:
                    error(errSyntaxError, getPos(), "Unterminated string");
                    done = true;
                    break;
                default:
                    c2 = c;
                    break;
                }
                break;

            default:
                c2 = c;
                break;
            }

            if (c2 != EOF) {
                if (n == tokBufSize) {
                    s.append(tokBuf, tokBufSize);
                    p = tokBuf;
                    n = 0;

                    // we are growing see if the document is not malformed and we are growing too much
                    if (objNum > 0 && xref != nullptr) {
                        const int newObjNum = xref->getNumEntry(getPos());
                        if (newObjNum != objNum) {
                            error(errSyntaxError, getPos(), "Unterminated string");
                            done = true;
                            s.clear();
                            n = -2;
                        }
                    }
                }
                *p++ = (char)c2;
                ++n;
            }
        } while (!done);

        if (n >= 0) {
            s.append(tokBuf, n);
            // Check utf8
            if (isUtf8WithBom(s)) {
                s = utf8ToUtf16WithBom(s);
            }
            return Object(std::move(s));
        }
        return Object::eof();

        break;
    }
    // name
    case '/': {
        p = tokBuf;
        n = 0;
        std::string s;
        while ((c = lookChar()) != EOF && !specialChars[c]) {
            getChar();
            if (c == '#') {
                c2 = lookChar();
                if (c2 >= '0' && c2 <= '9') {
                    c = c2 - '0';
                } else if (c2 >= 'A' && c2 <= 'F') {
                    c = c2 - 'A' + 10;
                } else if (c2 >= 'a' && c2 <= 'f') {
                    c = c2 - 'a' + 10;
                } else {
                    goto notEscChar;
                }
                getChar();
                c <<= 4;
                c2 = getChar();
                if (c2 >= '0' && c2 <= '9') {
                    c += c2 - '0';
                } else if (c2 >= 'A' && c2 <= 'F') {
                    c += c2 - 'A' + 10;
                } else if (c2 >= 'a' && c2 <= 'f') {
                    c += c2 - 'a' + 10;
                } else {
                    error(errSyntaxError, getPos(), "Illegal digit in hex char in name");
                }
            }
        notEscChar:
            // the PDF spec claims that names are limited to 127 chars, but
            // Distiller 8 will produce longer names, and Acrobat 8 will
            // accept longer names
            ++n;
            if (n < tokBufSize) {
                *p++ = c;
            } else if (n == tokBufSize) {
                error(errSyntaxError, getPos(), "Warning: name token is longer than what the specification says it can be");
                *p = c;
                s = std::string(tokBuf, n);
            } else {
                // Somewhat arbitrary threshold
                if (unlikely(n == 1024 * 1024)) {
                    error(errSyntaxError, getPos(), "Error: name token is larger than 1 MB. Suspicion of hostile file. Stopping parsing");
                    return Object::eof();
                }
                s.push_back((char)c);
            }
        }
        if (n < tokBufSize) {
            return Object(objName, std::string_view(tokBuf, n));
        }
        Object obj(objName, s);
        return obj;
        break;
    }

    // array punctuation
    case '[':
        return Object(objCmd, std::string_view("[", 1));
    case ']':
        return Object(objCmd, std::string_view("]", 1));

    // hex string or dict punctuation
    case '<': {
        c = lookChar();

        // dict punctuation
        if (c == '<') {
            getChar();
            return Object(objCmd, std::string_view("<<", 2));

            // hex string
        }
        p = tokBuf;
        m = n = 0;
        c2 = 0;
        std::string s;
        while (true) {
            c = getChar();
            if (c == '>') {
                break;
            }
            if (c == EOF) {
                error(errSyntaxError, getPos(), "Unterminated hex string");
                break;
            }
            if (specialChars[c] != 1) {
                c2 = c2 << 4;
                if (c >= '0' && c <= '9') {
                    c2 += c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    c2 += c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') {
                    c2 += c - 'a' + 10;
                } else {
                    error(errSyntaxError, getPos(), "Illegal character <{0:02x}> in hex string", c);
                }
                if (++m == 2) {
                    if (n == tokBufSize) {
                        s.append(tokBuf, tokBufSize);
                        p = tokBuf;
                        n = 0;
                    }
                    *p++ = (char)c2;
                    ++n;
                    c2 = 0;
                    m = 0;
                }
            }
        }
        s.append(tokBuf, n);
        if (m == 1) {
            s.push_back((char)(c2 << 4));
        }
        if (isUtf8WithBom(s)) {
            s = utf8ToUtf16WithBom(s);
        }
        return Object(std::move(s));
    } break;

    // dict punctuation
    case '>':
        c = lookChar();
        if (c == '>') {
            getChar();
            return Object(objCmd, std::string_view(">>", 2));
        }
        error(errSyntaxError, getPos(), "Illegal character '>'");
        return Object::error();

        break;

    // error
    case ')':
    case '{':
    case '}':
        error(errSyntaxError, getPos(), "Illegal character '{0:c}'", c);
        return Object::error();
        break;

    // command
    default:
        p = tokBuf;
        *p++ = c;
        n = 1;
        while ((c = lookChar()) != EOF && !specialChars[c]) {
            getChar();
            *p++ = c;
            if (++n == tokBufSize) {
                error(errSyntaxError, getPos(), "Command token too long");
                break;
            }
        }
        std::string_view res(tokBuf, n);
        if (res == "true") {
            return Object(true);
        }
        if (res == "false") {
            return Object(false);
        }
        if (res == "null") {
            return Object::null();
        }
        return Object(objCmd, res);

        break;
    }

    return Object();
}

Object Lexer::getObj(std::string_view cmdA, int objNum)
{
    char tokBuf[tokBufSize]; // temporary token buffer
    char *p = tokBuf;
    int c;
    bool comment;
    int n;

    // skip whitespace and comments
    comment = false;
    const char *cmd1 = tokBuf;
    *tokBuf = 0;
    while (cmdA != std::string_view(cmd1, p) && (objNum < 0 || (xref && xref->getNumEntry(getPos()) == objNum))) {
        while (true) {
            if ((c = getChar()) == EOF) {
                return Object::eof();
            }
            if (comment) {
                if (c == '\r' || c == '\n') {
                    comment = false;
                }
            } else if (c == '%') {
                comment = true;
            } else if (specialChars[c] != 1) {
                break;
            }
        }
        p = tokBuf;
        *p++ = c;
        n = 1;
        while ((c = lookChar()) != EOF && specialChars[c] == 0) {
            getChar();
            if (++n == tokBufSize) {
                break;
            }
            *p++ = c;
        }
    }

    return Object(objCmd, std::string_view(tokBuf, p));
}

void Lexer::skipToNextLine()
{
    int c;

    while (true) {
        c = getChar();
        if (c == EOF || c == '\n') {
            return;
        }
        if (c == '\r') {
            if ((c = lookChar()) == '\n') {
                getChar();
            }
            return;
        }
    }
}

bool Lexer::isSpace(int c)
{
    return c >= 0 && c <= 0xff && specialChars[c] == 1;
}
