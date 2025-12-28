# Copyright 2008 Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(POPPLER_ADD_TEST exe build_flag)
  set(build_test ${${build_flag}})

  # Omit the disabled test binaries from the "all" target
  if(NOT build_test)
    set(_add_executable_param ${_add_executable_param} EXCLUDE_FROM_ALL)
  endif(NOT build_test)

  add_executable(${exe} ${_add_executable_param} ${ARGN})

  # if the tests are EXCLUDE_FROM_ALL, add a target "buildtests" to build all tests
  # Don't try to use custom targets if building with Visual Studio
  if(NOT build_test AND NOT MSVC_IDE)
    get_property(_buildtestsAdded GLOBAL PROPERTY BUILDTESTS_ADDED)
    if(NOT _buildtestsAdded)
      add_custom_target(buildtests)
      set_property(GLOBAL PROPERTY BUILDTESTS_ADDED TRUE)
    endif(NOT _buildtestsAdded)
    add_dependencies(buildtests ${exe})
  endif(NOT build_test AND NOT MSVC_IDE)
endmacro(POPPLER_ADD_TEST)

macro(POPPLER_CREATE_INSTALL_PKGCONFIG generated_file install_location)
  configure_file(${generated_file}.cmake ${CMAKE_CURRENT_BINARY_DIR}/${generated_file} @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${generated_file} DESTINATION ${install_location})
endmacro(POPPLER_CREATE_INSTALL_PKGCONFIG)

macro(SHOW_END_MESSAGE what value)
  string(LENGTH ${what} length_what)
  math(EXPR left_char "20 - ${length_what}")
  set(blanks)
  foreach(_i RANGE 1 ${left_char})
    set(blanks "${blanks} ")
  endforeach(_i)

  message("  ${what}:${blanks} ${value}")
endmacro(SHOW_END_MESSAGE)

macro(SHOW_END_MESSAGE_YESNO what enabled)
  if(${enabled})
    set(enabled_string "yes")
  else(${enabled})
    set(enabled_string "no")
  endif(${enabled})

  show_end_message("${what}" "${enabled_string}")
endmacro(SHOW_END_MESSAGE_YESNO)

macro(POPPLER_CHECK_LINK_FLAG flag var)
   set(_save_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
   include(CheckCXXSourceCompiles)
   set(CMAKE_REQUIRED_LIBRARIES "${flag}")
   check_cxx_source_compiles("int main() { return 0; }" ${var})
   set(CMAKE_REQUIRED_LIBRARIES "${_save_CMAKE_REQUIRED_LIBRARIES}")
endmacro(POPPLER_CHECK_LINK_FLAG)


set(CMAKE_SYSTEM_INCLUDE_PATH ${CMAKE_SYSTEM_INCLUDE_PATH}
                              "${CMAKE_INSTALL_PREFIX}/include" )

set(CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
                              "${CMAKE_INSTALL_PREFIX}/bin" )

set(CMAKE_SYSTEM_LIBRARY_PATH ${CMAKE_SYSTEM_LIBRARY_PATH}
                              "${CMAKE_INSTALL_PREFIX}/lib" )

# under Windows dlls may be also installed in bin/
if(WIN32)
  set(CMAKE_SYSTEM_LIBRARY_PATH ${CMAKE_SYSTEM_LIBRARY_PATH}
                                "${CMAKE_INSTALL_PREFIX}/bin" )
endif(WIN32)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

string(TOUPPER "${CMAKE_BUILD_TYPE}" _CMAKE_BUILD_TYPE_UPPER)
set(_known_build_types RELWITHDEBINFO;RELEASE;DEBUG;DEBUGFULL;PROFILE)
# We override CMAKE_CXX_FLAGS_${_CMAKE_BUILD_TYPE_UPPER} below. If the user
# selects a CMAKE_BUILD_TYPE that is not handled by the logic below, we will
# end up dropping the previous flags (e.g. those set in a cross-compilation
# CMake toolchain file). To avoid surprising compilation errors, we emit an
# error in that case, so that the user can handle the  passed CMAKE_BUILD_TYPE
# in the compiler flags logic below.
if (NOT "${_CMAKE_BUILD_TYPE_UPPER}" IN_LIST _known_build_types)
  message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()
set(_save_cflags "${CMAKE_C_FLAGS}")
set(_save_cxxflags "${CMAKE_CXX_FLAGS}")

if(CMAKE_COMPILER_IS_GNUCXX)
  # set the default compile warnings
  set(_warn "-Wall -Wextra -Wpedantic")
  set(_warn "${_warn} -Wcast-align")
  set(_warn "${_warn} -Wformat-security")
  set(_warn "${_warn} -Wframe-larger-than=65536")
  set(_warn "${_warn} -Wlogical-op")
  set(_warn "${_warn} -Wmissing-format-attribute")
  set(_warn "${_warn} -Wnon-virtual-dtor")
  set(_warn "${_warn} -Woverloaded-virtual")
  set(_warn "${_warn} -Wmissing-declarations")
  set(_warn "${_warn} -Wundef")
  set(_warn "${_warn} -Wzero-as-null-pointer-constant")
  set(_warn "${_warn} -Wshadow")
  set(_warn "${_warn} -Wsuggest-override")

  # set extra warnings
  set(_warnx "${_warnx} -Wconversion")
  set(_warnx "${_warnx} -Wuseless-cast")

  set(DEFAULT_COMPILE_WARNINGS "${_warn}")
  set(DEFAULT_COMPILE_WARNINGS_EXTRA "${_warn} ${_warnx}")

  set(CMAKE_CXX_FLAGS                "-fno-exceptions -fno-check-new -fno-common -fno-operator-names -D_DEFAULT_SOURCE")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
  set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG          "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
  set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
  set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs")
  set(CMAKE_C_FLAGS                  "-std=c99 -D_DEFAULT_SOURCE")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
  set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
  set(CMAKE_C_FLAGS_DEBUG            "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
  set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline")
  set(CMAKE_C_FLAGS_PROFILE          "-g3 -fno-inline -ftest-coverage -fprofile-arcs")

  poppler_check_link_flag("-Wl,--as-needed" GCC_HAS_AS_NEEDED)
  if(GCC_HAS_AS_NEEDED)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
  endif(GCC_HAS_AS_NEEDED)
  set(_compiler_flags_changed 1)
endif (CMAKE_COMPILER_IS_GNUCXX)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
# set the default compile warnings
  set(_warn "-Wall -Wextra -Wpedantic")
  set(_warn "${_warn} -Wcast-align")
  set(_warn "${_warn} -Wformat-security")
  set(_warn "${_warn} -Wframe-larger-than=65536")
  set(_warn "${_warn} -Wmissing-format-attribute")
  set(_warn "${_warn} -Wnon-virtual-dtor")
  set(_warn "${_warn} -Woverloaded-virtual")
  set(_warn "${_warn} -Wmissing-declarations")
  set(_warn "${_warn} -Wundef")
  set(_warn "${_warn} -Wzero-as-null-pointer-constant")
  set(_warn "${_warn} -Wshadow")
  set(_warn "${_warn} -Wweak-vtables")

  # set extra warnings
  set(_warnx "${_warnx} -Wconversion")

  set(DEFAULT_COMPILE_WARNINGS "${_warn}")
  set(DEFAULT_COMPILE_WARNINGS_EXTRA "${_warn} ${_warnx}")

  set(CMAKE_CXX_FLAGS                "-fno-exceptions -fno-check-new -fno-common -D_DEFAULT_SOURCE")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
  set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
  # clang does not support -fno-reorder-blocks -fno-schedule-insns, so do not use -O2
  set(CMAKE_CXX_FLAGS_DEBUG          "-g")
  set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
  set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs")
  set(CMAKE_C_FLAGS                  "-std=c99 -D_DEFAULT_SOURCE")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
  set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
  # clang does not support -fno-reorder-blocks -fno-schedule-insns, so do not use -O2
  set(CMAKE_C_FLAGS_DEBUG            "-g")
  set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline")
  set(CMAKE_C_FLAGS_PROFILE          "-g3 -fno-inline -ftest-coverage -fprofile-arcs")
  set(_compiler_flags_changed 1)
endif()

if(CMAKE_C_COMPILER MATCHES "icc")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
  set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG          "-O2 -g -0b0 -noalign")
  set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g -Ob0 -noalign")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
  set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
  set(CMAKE_C_FLAGS_DEBUG            "-O2 -g -Ob0 -noalign")
  set(CMAKE_C_FLAGS_DEBUGFULL        "-g -Ob0 -noalign")
  set(_compiler_flags_changed 1)
endif(CMAKE_C_COMPILER MATCHES "icc")

if(_compiler_flags_changed)
  # Ensure that the previous CMAKE_{C,CXX}_FLAGS are included in the current configuration flags.
  foreach(_build_type ${_known_build_types})
    set(CMAKE_CXX_FLAGS_${_build_type} "${CMAKE_CXX_FLAGS_${_build_type}} ${_save_cxxflags}")
    set(CMAKE_C_FLAGS_${_build_type} "${CMAKE_C_FLAGS_${_build_type}} ${_save_cflags}")
  endforeach()
endif()
