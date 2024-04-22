# - try to find NSS3 libraries
# Once done this will define
#
#  NSS3_FOUND - system has NSS3
#  PkgConfig::NSS3 - Use this in target_link_libraries to bring both includes and link libraries
#
# Copyright 2015 André Guerreiro, <aguerreiro1985@gmail.com>
# Copyright 2022 Albert Astals Cid, <aacid@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(NSS3 IMPORTED_TARGET "nss>=3.68")

find_package_handle_standard_args(NSS3 DEFAULT_MSG NSS3_LIBRARIES NSS3_CFLAGS)
