/*
 * Copyright (C) 2009-2010, Pino Toscano <pino@kde.org>
 * Copyright (C) 2010, Hib Eris <hib@hiberis.nl>
 * Copyright (C) 2014, 2015 Hans-Peter Deifel <hpdeifel@gmx.de>
 * Copyright (C) 2015, Tamas Szekeres <szekerest@gmail.com>
 * Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
 * Copyright (C) 2018, 2020-2022, Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2018 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
 * Copyright (C) 2018, 2020, Adam Reichold <adam.reichold@t-online.de>
 * Copyright (C) 2022, Oliver Sander <oliver.sander@tu-dresden.de>
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

/**
 \file poppler-global.h
 */
#include "poppler-global.h"

#include "poppler-private.h"
#include "poppler-document-private.h"

#include "DateInfo.h"

#include <algorithm>

#include <cerrno>
#include <cstring>
#include <ios>
#include <iostream>

#include <iconv.h>

#include "config.h"

namespace {

struct MiniIconv
{
    MiniIconv(const char *to_code, const char *from_code) : i_(iconv_open(to_code, from_code)) { }
    ~MiniIconv()
    {
        if (is_valid()) {
            iconv_close(i_);
        }
    }
    MiniIconv(const MiniIconv &) = delete;
    MiniIconv &operator=(const MiniIconv &) = delete;
    bool is_valid() const { return i_ != (iconv_t)-1; }
    explicit operator iconv_t() const { return i_; }
    iconv_t i_;
};

}

using namespace poppler;

/**
 \namespace poppler

 Single namespace containing all the classes and functions of poppler-cpp.
 */

/**
 \class poppler::noncopyable

 A class that cannot be copied.
 */

/**
 \enum poppler::rotation_enum

 The case sensitivity.
*/
/**
 \var poppler::rotation_enum poppler::rotate_0

 A rotation of 0 degrees clockwise.
*/
/**
 \var poppler::rotation_enum poppler::rotate_90

 A rotation of 90 degrees clockwise.
*/
/**
 \var poppler::rotation_enum poppler::rotate_180

 A rotation of 180 degrees clockwise.
*/
/**
 \var poppler::rotation_enum poppler::rotate_270

 A rotation of 270 degrees clockwise.
*/

/**
 \enum poppler::page_box_enum

 A possible box of a page in a PDF %document.
*/
/**
 \var poppler::page_box_enum poppler::media_box

 The "media" box.
*/
/**
 \var poppler::page_box_enum poppler::crop_box

 The "crop" box.
*/
/**
 \var poppler::page_box_enum poppler::bleed_box

 The "bleed" box.
*/
/**
 \var poppler::page_box_enum poppler::trim_box

 The "trim" box.
*/
/**
 \var poppler::page_box_enum poppler::art_box

 The "art" box.
*/

/**
 \enum poppler::permission_enum

 A possible permission in a PDF %document.
*/
/**
 \var poppler::permission_enum poppler::perm_print

 The permission to allow the print of a %document.
*/
/**
 \var poppler::permission_enum poppler::perm_change

 The permission to change a %document.

 This is a generic "change" permission, so other permissions could affect
 some types of changes.
*/
/**
 \var poppler::permission_enum poppler::perm_copy

 The permission to allow the copy or extraction of the text in a %document.
*/
/**
 \var poppler::permission_enum poppler::perm_add_notes

 The permission to allow the addition or editing of annotations,
 and the filling of interactive form fields (including signature fields).
*/
/**
 \var poppler::permission_enum poppler::perm_fill_forms

 The permission to allow the filling of interactive form fields
 (including signature fields).

 \note this permission can be set even when the \ref poppler::perm_add_notes "perm_add_notes"
       is not: this means that only the filling of forms is allowed.
*/
/**
 \var poppler::permission_enum poppler::perm_accessibility

 The permission to allow the extracting of content (for example, text) for
 accessibility usage (e.g. for a screen reader).
*/
/**
 \var poppler::permission_enum poppler::perm_assemble

 The permission to allow to "assemble" a %document.

 This implies operations such as the insertion, the rotation and the deletion
 of pages; the creation of bookmarks and thumbnail images.

 \note this permission can be set even when the \ref poppler::perm_change "perm_change"
       is not
*/
/**
 \var poppler::permission_enum poppler::perm_print_high_resolution

 The permission to allow the high resolution print of a %document.
*/

/**
 \enum poppler::case_sensitivity_enum

 The case sensitivity.
*/

noncopyable::noncopyable() { }

noncopyable::~noncopyable() { }

noncopyable &noncopyable::operator=(noncopyable &&other) noexcept = default;

ustring::ustring() { }

ustring::ustring(size_type len, value_type ch) : std::basic_string<value_type>(len, ch) { }

ustring::~ustring() { }

byte_array ustring::to_utf8() const
{
    if (!size()) {
        return byte_array();
    }

#ifdef WORDS_BIGENDIAN
    MiniIconv ic("UTF-8", "UTF-16BE");
#else
    MiniIconv ic("UTF-8", "UTF-16LE");
#endif
    if (!ic.is_valid()) {
        return byte_array();
    }
    const value_type *me_data = data();
    byte_array str(size() * sizeof(value_type));
    char *str_data = &str[0];
    size_t me_len_char = size() * sizeof(value_type);
    size_t str_len_left = str.size();
    size_t ir = iconv(static_cast<iconv_t>(ic), (ICONV_CONST char **)&me_data, &me_len_char, &str_data, &str_len_left);
    if ((ir == (size_t)-1) && (errno == E2BIG)) {
        const size_t delta = str_data - &str[0];
        str_len_left += str.size();
        str.resize(str.size() * 2);
        str_data = &str[delta];
        ir = iconv(static_cast<iconv_t>(ic), (ICONV_CONST char **)&me_data, &me_len_char, &str_data, &str_len_left);
        if (ir == (size_t)-1) {
            return byte_array();
        }
    }
    str.resize(str.size() - str_len_left);
    return str;
}

std::string ustring::to_latin1() const
{
    if (!size()) {
        return std::string();
    }

    const size_type mylength = size();
    std::string ret(mylength, '\0');
    const value_type *me = data();
    for (size_type i = 0; i < mylength; ++i) {
        ret[i] = (char)*me++;
    }
    return ret;
}

ustring ustring::from_utf8(const char *str, int len)
{
    if (len <= 0) {
        len = std::strlen(str);
        if (len <= 0) {
            return ustring();
        }
    }

#ifdef WORDS_BIGENDIAN
    MiniIconv ic("UTF-16BE", "UTF-8");
#else
    MiniIconv ic("UTF-16LE", "UTF-8");
#endif
    if (!ic.is_valid()) {
        return ustring();
    }

    // +1, because iconv inserts byte order marks
    ustring ret(len + 1, 0);
    char *ret_data = reinterpret_cast<char *>(&ret[0]);
    char *str_data = const_cast<char *>(str);
    size_t str_len_char = len;
    size_t ret_len_left = ret.size() * sizeof(ustring::value_type);
    size_t ir = iconv(static_cast<iconv_t>(ic), (ICONV_CONST char **)&str_data, &str_len_char, &ret_data, &ret_len_left);
    if ((ir == (size_t)-1) && (errno == E2BIG)) {
        const size_t delta = ret_data - reinterpret_cast<char *>(&ret[0]);
        ret_len_left += ret.size() * sizeof(ustring::value_type);
        ret.resize(ret.size() * 2);
        ret_data = reinterpret_cast<char *>(&ret[0]) + delta;
        ir = iconv(static_cast<iconv_t>(ic), (ICONV_CONST char **)&str_data, &str_len_char, &ret_data, &ret_len_left);
        if (ir == (size_t)-1) {
            return ustring();
        }
    }
    ret.resize(ret.size() - ret_len_left / sizeof(ustring::value_type));

    return ret;
}

ustring ustring::from_latin1(const std::string &str)
{
    const size_type l = str.size();
    if (!l) {
        return ustring();
    }
    const char *c = str.data();
    ustring ret(l, 0);
    for (size_type i = 0; i < l; ++i) {
        ret[i] = static_cast<unsigned char>(*c);
        c++;
    }
    return ret;
}

/**
 Converts a string representing a PDF date to a value compatible with time_type.
 */
time_type poppler::convert_date(const std::string &date)
{
    GooString gooDateStr(date.c_str());
    return static_cast<time_type>(dateStringToTime(&gooDateStr));
}

/**
 Converts a string representing a PDF date to a value compatible with time_t.
 */
time_t poppler::convert_date_t(const std::string &date)
{
    GooString gooDateStr(date.c_str());
    return dateStringToTime(&gooDateStr);
}

std::ostream &poppler::operator<<(std::ostream &stream, const byte_array &array)
{
    stream << "[";
    const std::ios_base::fmtflags f = stream.flags();
    std::hex(stream);
    const char *data = &array[0];
    const byte_array::size_type out_len = std::min<byte_array::size_type>(array.size(), 50);
    for (byte_array::size_type i = 0; i < out_len; ++i) {
        if (i != 0) {
            stream << " ";
        }
        stream << ((data[i] & 0xf0) >> 4) << (data[i] & 0xf);
    }
    stream.flags(f);
    if (out_len < array.size()) {
        stream << " ...";
    }
    stream << "]";
    return stream;
}

/**
 * Sets a custom data directory for initialization of global parameters
 *
 * If no instances of \see document currently exist, this will save the
 * given path as a custom data directory to be used when the first instance
 * of the \see document is constructed.
 *
 * \returns true on success, false on failure
 *
 * \since 0.73.0
 */
bool poppler::set_data_dir(const std::string &new_data_dir)
{
    return GlobalParamsIniter::setCustomDataDir(new_data_dir);
}

/**
 \typedef poppler::debug_func

 Debug/error function.

 This function type is used for debugging & error output;
 the first parameter is the actual message, the second is the unaltered
 closure argument which was passed to the set_debug_error_function() call.

 \since 0.30.0
 */

/**
 Set a new debug/error output function.

 If not set, by default error and debug messages will be sent to stderr.

 \param debug_function the new debug function
 \param closure user data which will be passed as-is to the debug function

 \since 0.30.0
 */
void poppler::set_debug_error_function(debug_func debug_function, void *closure)
{
    poppler::detail::user_debug_function = debug_function;
    poppler::detail::debug_closure = closure;
}
