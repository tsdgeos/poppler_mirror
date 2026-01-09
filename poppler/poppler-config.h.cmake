//================================================= -*- mode: c++ -*- ====
//
// poppler-config.h
//
// Copyright 1996-2011, 2022 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2014 Bogdan Cristea <cristeab@gmail.com>
// Copyright (C) 2014 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2016 Tor Lillqvist <tml@collabora.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2018 Stefan Brüns <stefan.bruens@rwth-aachen.de>
// Copyright (C) 2020, 2025 Albert Astals Cid <aacid@kde.org>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef POPPLER_CONFIG_H
#define POPPLER_CONFIG_H

/* Defines the poppler version. */
#define POPPLER_VERSION "${POPPLER_VERSION}"

/* Use single precision arithmetic in the Splash backend */
#cmakedefine01 USE_FLOAT

/* Support for curl is compiled in. */
#cmakedefine01 POPPLER_HAS_CURL_SUPPORT

/* Use libjpeg instead of builtin jpeg decoder. */
#cmakedefine01 ENABLE_LIBJPEG

/* Build against libtiff. */
#cmakedefine01 ENABLE_LIBTIFF

/* Build against libpng. */
#cmakedefine01 ENABLE_LIBPNG

/* Use zlib instead of builtin zlib decoder to uncompress flate streams. */
#cmakedefine01 ENABLE_ZLIB_UNCOMPRESS

/* Defines if use cms */
#cmakedefine01 USE_CMS

/* Use header-only classes from Boost in the Splash backend */
#cmakedefine01 USE_BOOST_HEADERS

//------------------------------------------------------------------------
// version
//------------------------------------------------------------------------

// copyright notice
#define popplerCopyright "Copyright 2005-2026 The Poppler Developers - http://poppler.freedesktop.org"
#define xpdfCopyright "Copyright 1996-2011, 2022 Glyph & Cog, LLC"

//------------------------------------------------------------------------
// Win32 stuff
//------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_MSC_VER)
#    include <windef.h>
#else
#    define CDECL
#endif

//------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------

#ifdef __GNUC__
#    ifdef __MINGW32__
#        define GCC_PRINTF_FORMAT(fmt_index, va_index) __attribute__((__format__(gnu_printf, fmt_index, va_index)))
#    else
#        define GCC_PRINTF_FORMAT(fmt_index, va_index) __attribute__((__format__(printf, fmt_index, va_index)))
#    endif
#else
#    define GCC_PRINTF_FORMAT(fmt_index, va_index)
#endif

#endif /* POPPLER_CONFIG_H */
