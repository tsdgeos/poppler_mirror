# - Try to find the libopenjpeg2 library
# Once done this will define
#
#  LIBOPENJPEG2_FOUND - system has libopenjpeg
#  LIBOPENJPEG2_INCLUDE_DIRS - the libopenjpeg include directories
#  LIBOPENJPEG2_LIBRARIES - Link these to use libopenjpeg

# Copyright (c) 2008, Albert Astals Cid, <aacid@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBOPENJPEG2_LIBRARIES AND LIBOPENJPEG2_INCLUDE_DIR)

  # in cache already
  set(LIBOPENJPEG2_FOUND TRUE)

else ()

  set(LIBOPENJPEG2_FOUND FALSE)
  set(LIBOPENJPEG2_INCLUDE_DIRS)
  set(LIBOPENJPEG2_LIBRARIES)

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBOPENJPEG2 libopenjp2)
  if (LIBOPENJPEG2_FOUND)
    add_definitions(-DUSE_OPENJPEG2)
  endif ()
endif ()
