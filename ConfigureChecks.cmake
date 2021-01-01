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

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(sys/mman.h HAVE_SYS_MMAN_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(unistd.h HAVE_UNISTD_H)

check_function_exists(fseek64 HAVE_FSEEK64)
check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
check_function_exists(ftell64 HAVE_FTELL64)
check_function_exists(pread64 HAVE_PREAD64)
check_function_exists(lseek64 HAVE_LSEEK64)
check_function_exists(gmtime_r HAVE_GMTIME_R)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(popen HAVE_POPEN)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(strtok_r HAVE_STRTOK_R)

macro(CHECK_FOR_DIR include var)
  check_c_source_compiles(
    "#include <${include}>

int main(int argc, char *argv[])
{
  DIR* d = 0;
  return 0;
}
" ${var})
endmacro(CHECK_FOR_DIR)
check_for_dir("dirent.h" HAVE_DIRENT_H)
check_for_dir("ndir.h" HAVE_NDIR_H)
check_for_dir("sys/dir.h" HAVE_SYS_DIR_H)
check_for_dir("sys/ndir.h" HAVE_SYS_NDIR_H)

check_function_exists("nanosleep" HAVE_NANOSLEEP)
if(NOT HAVE_NANOSLEEP)
  check_library_exists("rt" "nanosleep" "" LIB_RT_HAS_NANOSLEEP)
endif(NOT HAVE_NANOSLEEP)

cmake_pop_check_state()
