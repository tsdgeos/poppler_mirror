# - try to find GTK libraries
# Once done this will define
#
#  GTK_FOUND - system has GTK
#  GTK3_CFLAGS - the GTK CFlags
#  GTK3_LIBRARIES - Link these to use GTK
#
# Copyright 2008-2010 Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# NOTE: As of cmake v3.18, built-in FindGTK is *only* valid for GTK1

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
    pkg_check_modules(GTK3 "gtk+-3.0>=${GTK_REQUIRED}" "gdk-pixbuf-2.0>=${GDK_PIXBUF_REQUIRED}")
else()
    pkg_check_modules(GTK3 IMPORTED_TARGET "gtk+-3.0>=${GTK_REQUIRED}" "gdk-pixbuf-2.0>=${GDK_PIXBUF_REQUIRED}")
endif()

find_package_handle_standard_args(GTK DEFAULT_MSG GTK3_LIBRARIES GTK3_CFLAGS)
