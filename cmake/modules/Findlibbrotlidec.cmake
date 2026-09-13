# - try to find Brotli libraries
# Once done this will define
#
# PkgConfig::libbrotlidec
#
# Copyright 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(libbrotlidec IMPORTED_TARGET "libbrotlidec>=${libbrotlidec_FIND_VERSION}")

find_package_handle_standard_args(libbrotlidec DEFAULT_MSG libbrotlidec_LIBRARIES libbrotlidec_CFLAGS)

