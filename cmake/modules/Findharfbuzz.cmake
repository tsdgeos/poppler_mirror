# - try to find Harfbuzz libraries
# Once done this will define
#
# harfbuzz::harfbuzz
# harfbuzz::subset
#
# Copyright 2026 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(harfbuzz IMPORTED_TARGET harfbuzz)

find_package_handle_standard_args(harfbuzz DEFAULT_MSG harfbuzz_LINK_LIBRARIES harfbuzz_CFLAGS)

pkg_check_modules(harfbuzz-subset IMPORTED_TARGET harfbuzz-subset)

find_package_handle_standard_args(harfbuzz-subset DEFAULT_MSG harfbuzz-subset_LINK_LIBRARIES harfbuzz-subset_CFLAGS)

add_library(harfbuzz::harfbuzz ALIAS PkgConfig::harfbuzz)
add_library(harfbuzz::subset ALIAS PkgConfig::harfbuzz-subset)
if (NOT harfbuzz-subset_FOUND)
set (harfbuzz_FOUND false)
endif()



