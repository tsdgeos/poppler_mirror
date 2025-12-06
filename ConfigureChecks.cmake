# Copyright 2008 Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CMakePushCheckState)

cmake_push_check_state()
# this is going to be defined via config.h, and impacts Android's stdio.h
if (_FILE_OFFSET_BITS)
  set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=${_FILE_OFFSET_BITS})
endif()

check_function_exists(fseek64 HAVE_FSEEK64)
check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
check_function_exists(pread64 HAVE_PREAD64)
check_function_exists(lseek64 HAVE_LSEEK64)
check_function_exists(gmtime_r HAVE_GMTIME_R)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(popen HAVE_POPEN)
check_function_exists(strtok_r HAVE_STRTOK_R)

cmake_pop_check_state()
