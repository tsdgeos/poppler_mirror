//========================================================================
//
// Object.cc
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
// Copyright (C) 2008, 2010, 2012, 2017, 2019, 2024, 2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2020 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2025, 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <cstddef>
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Stream.h"
#include "XRef.h"
#include "goo/gmem.h"

//------------------------------------------------------------------------
// Object
//------------------------------------------------------------------------

static const char *objTypeNames[numObjTypes] = { "boolean", "integer", "real", "string", "name", "null", "array", "dictionary", "stream", "ref", "cmd", "error", "eof", "none", "integer64", "hexString", "dead" };

Object Object::copy() const
{
    CHECK_NOT_DEAD;

    return Object { type, data, PrivateTag {} };
}

Object Object::deepCopy() const
{
    CHECK_NOT_DEAD;

    switch (type) {
    case objArray:
        return Object { objArray, std::shared_ptr<Array>(std::get<std::shared_ptr<Array>>(data)->deepCopy()), PrivateTag {} };
    case objDict:
        return Object { objDict, std::shared_ptr<Dict>(std::get<std::shared_ptr<Dict>>(data)->deepCopy()), PrivateTag {} };
    default:
        return copy();
    }
}

Object Object::fetch(XRef *xref, int recursion) const
{
    CHECK_NOT_DEAD;

    return (type == objRef && xref) ? xref->fetch(std::get<Ref>(data), recursion) : copy();
}

const char *Object::getTypeName() const
{
    return objTypeNames[type];
}

void Object::print(FILE *f) const
{
    switch (type) {
    case objBool:
        fprintf(f, "%s", std::get<bool>(data) ? "true" : "false");
        break;
    case objInt:
        fprintf(f, "%d", std::get<int>(data));
        break;
    case objReal:
        fprintf(f, "%g", std::get<double>(data));
        break;
    case objString: {
        fprintf(f, "(");

        const auto &string = std::get<std::string>(data);
        fwrite(string.c_str(), 1, string.size(), f);
        fprintf(f, ")");
    } break;
    case objHexString: {
        fprintf(f, "<");
        const auto &string = std::get<std::string>(data);
        for (auto c : string) {
            fprintf(f, "%02x", c & 0xff);
        }
        fprintf(f, ">");
    } break;
    case objName:
        fprintf(f, "/%s", std::get<std::string>(data).c_str());
        break;
    case objNull:
        fprintf(f, "null");
        break;
    case objArray:
        fprintf(f, "[");
        for (int i = 0; i < arrayGetLength(); ++i) {
            if (i > 0) {
                fprintf(f, " ");
            }
            const Object &obj = arrayGetNF(i);
            obj.print(f);
        }
        fprintf(f, "]");
        break;
    case objDict:
        fprintf(f, "<<");
        for (int i = 0; i < dictGetLength(); ++i) {
            fprintf(f, " /%s ", dictGetKey(i));
            const Object &obj = dictGetValNF(i);
            obj.print(f);
        }
        fprintf(f, " >>");
        break;
    case objStream:
        fprintf(f, "<stream>");
        break;
    case objRef: {
        Ref ref = std::get<Ref>(data);
        fprintf(f, "%d %d R", ref.num, ref.gen);
    } break;
    case objCmd:
        fprintf(f, "%s", std::get<std::string>(data).c_str());
        break;
    case objError:
        fprintf(f, "<error>");
        break;
    case objEOF:
        fprintf(f, "<EOF>");
        break;
    case objNone:
        fprintf(f, "<none>");
        break;
    case objDead:
        fprintf(f, "<dead>");
        break;
    case objInt64:
        fprintf(f, "%lld", std::get<long long>(data));
        break;
    }
}
