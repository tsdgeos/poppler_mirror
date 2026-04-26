//========================================================================
//
// Object.h
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
// Copyright (C) 2007 Julien Rebetez <julienr@svn.gnome.org>
// Copyright (C) 2008 Kees Cook <kees@outflux.net>
// Copyright (C) 2008, 2010, 2017-2021, 2023-2025 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2009 Jakub Wilk <jwilk@jwilk.net>
// Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013, 2017, 2018 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Adrian Perez de Castro <aperez@igalia.com>
// Copyright (C) 2016, 2020 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by Technische Universität Dresden
// Copyright (C) 2023 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2024-2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
// Copyright (C) 2025 Jonathan Hähne <jonathan.haehne@hotmail.com>
// Copyright (C) 2025 Arnav V <arnav0872@gmail.com>
// Copyright (C) 2026 Adam Sampson <ats@offog.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef OBJECT_H
#define OBJECT_H

#include <cassert>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <variant>
#include <cstring>
#include "goo/GooString.h"
#include "goo/GooLikely.h"
#include "Error.h"
#include "poppler_private_export.h"

#define OBJECT_TYPE_CHECK(wanted_type)                                                                                                                                                                                                         \
    if (unlikely(type != (wanted_type))) {                                                                                                                                                                                                     \
        ::error(errInternal, 0,                                                                                                                                                                                                                \
                "Call to Object where the object was type {0:d}, "                                                                                                                                                                             \
                "not the expected type {1:d}",                                                                                                                                                                                                 \
                type, wanted_type);                                                                                                                                                                                                            \
        abort();                                                                                                                                                                                                                               \
    }

#define OBJECT_2TYPES_CHECK(wanted_type1, wanted_type2)                                                                                                                                                                                        \
    if (unlikely(type != (wanted_type1)) && unlikely(type != (wanted_type2))) {                                                                                                                                                                \
        ::error(errInternal, 0,                                                                                                                                                                                                                \
                "Call to Object where the object was type {0:d}, "                                                                                                                                                                             \
                "not the expected type {1:d} or {2:d}",                                                                                                                                                                                        \
                type, wanted_type1, wanted_type2);                                                                                                                                                                                             \
        abort();                                                                                                                                                                                                                               \
    }

#define OBJECT_3TYPES_CHECK(wanted_type1, wanted_type2, wanted_type3)                                                                                                                                                                          \
    if (unlikely(type != (wanted_type1)) && unlikely(type != (wanted_type2)) && unlikely(type != (wanted_type3))) {                                                                                                                            \
        ::error(errInternal, 0,                                                                                                                                                                                                                \
                "Call to Object where the object was type {0:d}, "                                                                                                                                                                             \
                "not the expected type {1:d}, {2:d} or {3:d}",                                                                                                                                                                                 \
                type, wanted_type1, wanted_type2, wanted_type3);                                                                                                                                                                               \
        abort();                                                                                                                                                                                                                               \
    }

#define CHECK_NOT_DEAD                                                                                                                                                                                                                         \
    if (unlikely(type == objDead)) {                                                                                                                                                                                                           \
        ::error(errInternal, 0, "Call to dead object");                                                                                                                                                                                        \
        abort();                                                                                                                                                                                                                               \
    }

class XRef;
class Array;
class Dict;
class Stream;

//------------------------------------------------------------------------
// Ref
//------------------------------------------------------------------------

struct Ref
{
    int num; // object number
    int gen; // generation number

    static constexpr Ref INVALID() { return { .num = -1, .gen = -1 }; };
};

inline bool operator==(const Ref lhs, const Ref rhs) noexcept
{
    return lhs.num == rhs.num && lhs.gen == rhs.gen;
}

inline bool operator!=(const Ref lhs, const Ref rhs) noexcept
{
    return lhs.num != rhs.num || lhs.gen != rhs.gen;
}

inline bool operator<(const Ref lhs, const Ref rhs) noexcept
{
    if (lhs.num != rhs.num) {
        return lhs.num < rhs.num;
    }
    return lhs.gen < rhs.gen;
}

struct RefRecursionChecker
{
    RefRecursionChecker() = default;

    RefRecursionChecker(const RefRecursionChecker &) = delete;
    RefRecursionChecker &operator=(const RefRecursionChecker &) = delete;

    bool insert(Ref ref)
    {
        if (ref == Ref::INVALID()) {
            return true;
        }

        // insert returns std::pair<iterator,bool>
        // where the bool is whether the insert succeeded
        return alreadySeenRefs.insert(ref.num).second;
    }

    void remove(Ref ref) { alreadySeenRefs.erase(ref.num); }

private:
    std::set<int> alreadySeenRefs;
};

struct RefRecursionCheckerRemover
{
    // Removes ref from c when this object is removed
    RefRecursionCheckerRemover(RefRecursionChecker &c, Ref r) : checker(c), ref(r) { }
    ~RefRecursionCheckerRemover() { checker.remove(ref); }

    RefRecursionCheckerRemover(const RefRecursionCheckerRemover &) = delete;
    RefRecursionCheckerRemover &operator=(const RefRecursionCheckerRemover &) = delete;

private:
    RefRecursionChecker &checker;
    Ref ref;
};

namespace std {

template<>
struct hash<Ref>
{
    using argument_type = Ref;
    using result_type = size_t;

    result_type operator()(const argument_type ref) const noexcept { return std::hash<int> {}(ref.num) ^ (std::hash<int> {}(ref.gen) << 1); }
};

}

//------------------------------------------------------------------------
// object types
//------------------------------------------------------------------------

enum ObjType
{
    // simple objects
    objBool, // boolean
    objInt, // integer
    objReal, // real
    objString, // string
    objName, // name
    objNull, // null

    // complex objects
    objArray, // array
    objDict, // dictionary
    objStream, // stream
    objRef, // indirect reference

    // special objects
    objCmd, // command name
    objError, // error return from Lexer
    objEOF, // end of file return from Lexer
    objNone, // uninitialized object

    // poppler-only objects
    objInt64, // integer with at least 64-bits
    objHexString, // hex string
    objDead // and object after shallowCopy
};

constexpr int numObjTypes = 17; // total number of object types

//------------------------------------------------------------------------
// Object
//------------------------------------------------------------------------

class POPPLER_PRIVATE_EXPORT Object
{
public:
    static Object null() { return Object(objNull); }
    static Object eof() { return Object(objEOF); }
    static Object error() { return Object(objError); }
    Object() : type(objNone) { }
    ~Object() = default;

    explicit Object(bool boolnA) : type { objBool }, data { boolnA } { }

    explicit Object(int intgA) : type { objInt }, data { intgA } { }

    explicit Object(double realA) : type { objReal }, data { realA } { }

    explicit Object(std::unique_ptr<GooString> stringA) : type { objString }, data { std::move(stringA->toNonConstStr()) } { assert(stringA); }

    explicit Object(std::string &&stringA) : type { objString }, data { std::move(stringA) } { }

    Object(ObjType typeA, std::string &&stringA) : type { typeA }, data { std::move(stringA) } { assert(typeA == objHexString); }

    Object(ObjType typeA, const char *v) : Object(typeA, std::string_view(v)) { }
    Object(ObjType typeA, std::string_view v) : type { typeA }, data { std::string { v } } { assert(typeA == objName || typeA == objCmd); }

    explicit Object(long long int64gA) : type { objInt64 }, data { int64gA } { }

    explicit Object(std::unique_ptr<Array> arrayA) : type { objArray }, data { std::shared_ptr<Array>(std::move(arrayA)) } { }

    explicit Object(std::unique_ptr<Dict> &&dictA) : type { objDict }, data { std::shared_ptr<Dict>(std::move(dictA)) } { }

    template<typename StreamType>
        requires(std::is_base_of_v<Stream, StreamType>)
    explicit Object(std::unique_ptr<StreamType> &&streamA) : type { objStream }, data { std::shared_ptr<Stream>(std::move(streamA)) }
    {
    }
    explicit Object(Ref r) : type { objRef }, data { r } { }

    template<typename T>
    Object(T) = delete;

    Object(Object &&other) noexcept : type { other.type }, data { std::move(other.data) } { other.type = objDead; }

    Object &operator=(Object &&other) noexcept
    {
        data = std::move(other.data);
        type = other.type;
        other.type = objDead;

        return *this;
    }

    Object &operator=(const Object &other) = delete;
    Object(const Object &other) = delete;

    // Set object to null.
    void setToNull()
    {
        data = std::monostate {};
        type = objNull;
    }

    // Copies all object types except
    // objArray, objDict, objStream whose refcount is increased by 1
    Object copy() const;

    // Deep copies all object types (recursively)
    // except objStream whose refcount is increased by 1
    Object deepCopy() const;

    // If object is a Ref, fetch and return the referenced object.
    // Otherwise, return a copy of the object.
    Object fetch(XRef *xref, int recursion = 0) const;

    // Type checking.
    ObjType getType() const
    {
        CHECK_NOT_DEAD;
        return type;
    }
    bool isBool() const
    {
        CHECK_NOT_DEAD;
        return type == objBool;
    }
    bool isInt() const
    {
        CHECK_NOT_DEAD;
        return type == objInt;
    }
    bool isReal() const
    {
        CHECK_NOT_DEAD;
        return type == objReal;
    }
    bool isNum() const
    {
        CHECK_NOT_DEAD;
        return type == objInt || type == objReal || type == objInt64;
    }
    bool isString() const
    {
        CHECK_NOT_DEAD;
        return type == objString;
    }
    bool isHexString() const
    {
        CHECK_NOT_DEAD;
        return type == objHexString;
    }
    bool isName() const
    {
        CHECK_NOT_DEAD;
        return type == objName;
    }
    bool isNull() const
    {
        CHECK_NOT_DEAD;
        return type == objNull;
    }
    bool isArray() const
    {
        CHECK_NOT_DEAD;
        return type == objArray;
    }
    bool isDict() const
    {
        CHECK_NOT_DEAD;
        return type == objDict;
    }
    bool isStream() const
    {
        CHECK_NOT_DEAD;
        return type == objStream;
    }
    bool isRef() const
    {
        CHECK_NOT_DEAD;
        return type == objRef;
    }
    bool isCmd() const
    {
        CHECK_NOT_DEAD;
        return type == objCmd;
    }
    bool isError() const
    {
        CHECK_NOT_DEAD;
        return type == objError;
    }
    bool isEOF() const
    {
        CHECK_NOT_DEAD;
        return type == objEOF;
    }
    bool isNone() const
    {
        CHECK_NOT_DEAD;
        return type == objNone;
    }
    bool isInt64() const
    {
        CHECK_NOT_DEAD;
        return type == objInt64;
    }
    bool isIntOrInt64() const
    {
        CHECK_NOT_DEAD;
        return type == objInt || type == objInt64;
    }

    // Special type checking.
    bool isName(std::string_view nameA) const { return type == objName && getName() == nameA; }
    bool isArrayOfLength(int length) const;
    bool isArrayOfLengthAtLeast(int length) const;
    bool isDict(std::string_view dictType) const;
    bool isCmd(std::string_view cmdA) const { return type == objCmd && std::get<std::string>(data) == cmdA; }

    // Accessors.
    bool getBool() const
    {
        OBJECT_TYPE_CHECK(objBool);
        return std::get<bool>(data);
    }
    int getInt() const
    {
        OBJECT_TYPE_CHECK(objInt);
        return std::get<int>(data);
    }
    double getReal() const
    {
        OBJECT_TYPE_CHECK(objReal);
        return std::get<double>(data);
    }

    // Note: integers larger than 2^53 can not be exactly represented by a double.
    // Where the exact value of integers up to 2^63 is required, use isInt64()/getInt64().
    double getNum() const
    {
        OBJECT_3TYPES_CHECK(objInt, objInt64, objReal);
        return type == objInt ? static_cast<double>(std::get<int>(data)) : type == objInt64 ? static_cast<double>(std::get<long long>(data)) : std::get<double>(data);
    }
    double getNum(bool *ok) const
    {
        if (unlikely(type != objInt && type != objInt64 && type != objReal)) {
            *ok = false;
            return 0.;
        }
        return type == objInt ? static_cast<double>(std::get<int>(data)) : type == objInt64 ? static_cast<double>(std::get<long long>(data)) : std::get<double>(data);
    }
    const std::string &getString() const
    {
        OBJECT_TYPE_CHECK(objString);
        return std::get<std::string>(data);
    }

    std::unique_ptr<GooString> takeString()
    {
        OBJECT_TYPE_CHECK(objString);
        std::unique_ptr<GooString> str = std::make_unique<GooString>(std::get<std::string>(std::move(data)));
        data = std::monostate {};
        type = objNull;
        return str;
    }

    const std::string &getHexString() const
    {
        OBJECT_TYPE_CHECK(objHexString);
        return std::get<std::string>(data);
    }
    const char *getName() const
    {
        OBJECT_TYPE_CHECK(objName);
        return std::get<std::string>(data).c_str();
    }
    const std::string &getNameString() const
    {
        OBJECT_TYPE_CHECK(objName);
        return std::get<std::string>(data);
    }
    Array *getArray() const
    {
        OBJECT_TYPE_CHECK(objArray);
        return std::get<std::shared_ptr<Array>>(data).get();
    }
    Dict *getDict() const
    {
        OBJECT_TYPE_CHECK(objDict);
        return std::get<std::shared_ptr<Dict>>(data).get();
    }
    Stream *getStream() const
    {
        OBJECT_TYPE_CHECK(objStream);
        return std::get<std::shared_ptr<Stream>>(data).get();
    }
    Ref getRef() const
    {
        OBJECT_TYPE_CHECK(objRef);
        return std::get<Ref>(data);
    }
    int getRefNum() const
    {
        OBJECT_TYPE_CHECK(objRef);
        return std::get<Ref>(data).num;
    }
    int getRefGen() const
    {
        OBJECT_TYPE_CHECK(objRef);
        return std::get<Ref>(data).gen;
    }
    const char *getCmd() const
    {
        OBJECT_TYPE_CHECK(objCmd);
        return std::get<std::string>(data).c_str();
    }
    long long getInt64() const
    {
        OBJECT_TYPE_CHECK(objInt64);
        return std::get<long long>(data);
    }
    long long getIntOrInt64() const
    {
        OBJECT_2TYPES_CHECK(objInt, objInt64);
        return type == objInt ? std::get<int>(data) : std::get<long long>(data);
    }

    // Array accessors.
    int arrayGetLength() const;
    void arrayAdd(Object &&elem);
    void arrayRemove(int i);
    Object arrayGet(int i, int recursion) const;
    const Object &arrayGetNF(int i) const;

    // Dict accessors.
    int dictGetLength() const;
    const char *dictGetKey(int i) const;
    void dictAdd(std::string_view key, Object &&val);
    void dictSet(std::string_view key, Object &&val);
    void dictRemove(std::string_view key);
    bool dictIs(std::string_view dictType) const;
    Object dictLookup(std::string_view key, int recursion = 0) const;
    const Object &dictLookupNF(std::string_view key) const;
    Object dictGetVal(int i) const;
    const Object &dictGetValNF(int i) const;

    // Stream accessors.
    [[nodiscard]] bool streamRewind();
    void streamClose();
    int streamGetChar();
    int streamGetChars(int nChars, unsigned char *buffer);
    void streamSetPos(Goffset pos, int dir = 0);
    Dict *streamGetDict() const;

    // Output.
    const char *getTypeName() const;
    void print(FILE *f = stdout) const;

    double getNumWithDefaultValue(double defaultValue) const
    {
        if (unlikely(type != objInt && type != objInt64 && type != objReal)) {
            return defaultValue;
        }
        return type == objInt ? static_cast<double>(std::get<int>(data)) : type == objInt64 ? static_cast<double>(std::get<long long>(data)) : std::get<double>(data);
    }

    bool getBoolWithDefaultValue(bool defaultValue) const { return (type == objBool) ? std::get<bool>(data) : defaultValue; }

private:
    class PrivateTag
    {
    };
    explicit Object(ObjType typeA) { type = typeA; }
    explicit Object(ObjType typeA, std::variant<std::monostate, bool, int, long long, double, std::string, std::shared_ptr<Array>, std::shared_ptr<Dict>, std::shared_ptr<Stream>, Ref> dataA, PrivateTag /*unnamed*/)
        : type { typeA }, data { std::move(dataA) }
    {
    }

    ObjType type; // object type
    std::variant<std::monostate, bool, int, long long, double, std::string, std::shared_ptr<Array>, std::shared_ptr<Dict>, std::shared_ptr<Stream>, Ref> data;
};

//------------------------------------------------------------------------
// Array accessors.
//------------------------------------------------------------------------

#include "Array.h"

//------------------------------------------------------------------------
// Dict accessors.
//------------------------------------------------------------------------

#include "Dict.h"

//------------------------------------------------------------------------
// Stream accessors.
//------------------------------------------------------------------------

#include "Stream.h"

#endif
