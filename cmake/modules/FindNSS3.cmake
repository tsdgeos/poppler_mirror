# - try to find NSS3 libraries
# Once done this will define
#
#  NSS_FOUND - system has NSS3
#  NSS3_CFLAGS - the NSS CFlags
#  NSS3_LIBRARIES - Link these to use NSS
#
# Copyright 2015 André Guerreiro, <aguerreiro1985@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

if (NOT MSVC)
  find_package(PkgConfig REQUIRED)

  if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
    pkg_check_modules(NSS3 "nss>=3.19")
  else ()
    pkg_check_modules(NSS3 IMPORTED_TARGET "nss>=3.19")
  endif()

  find_package_handle_standard_args(NSS3 DEFAULT_MSG NSS3_LIBRARIES NSS3_CFLAGS)

endif(NOT MSVC)
