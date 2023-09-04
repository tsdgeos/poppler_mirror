include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

include_directories(
  SYSTEM
  ${GLIB2_INCLUDE_DIRS}
  ${CAIRO_INCLUDE_DIRS}
)
add_definitions(
  -DG_LOG_DOMAIN=\"Poppler\"
  ${GLIB2_CFLAGS_OTHER}
  ${CAIRO_CFLAGS}
  ${POPPLER_GLIB_DISABLE_DEPRECATED}
  ${POPPLER_GLIB_DISABLE_SINGLE_INCLUDES}
)

configure_file(poppler-features.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/poppler-features.h @ONLY)

if (GTK_FOUND AND BUILD_GTK_TESTS AND NOT MSVC)
  add_subdirectory(demo)
  add_subdirectory(tests)
endif ()

set(poppler_glib_public_headers
  poppler-action.h
  poppler-date.h
  poppler-document.h
  poppler-page.h
  poppler-attachment.h
  poppler-form-field.h
  poppler-annot.h
  poppler-layer.h
  poppler-movie.h
  poppler-media.h
  poppler.h
  poppler-structure-element.h
)

find_program(GLIB2_MKENUMS glib-mkenums)
find_program(GLIB2_MKENUMS_PYTHON NAMES python3 python)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.h
  COMMAND ${GLIB2_MKENUMS_PYTHON}
  ARGS
    ${GLIB2_MKENUMS}
    --template poppler-enums.h.template
    ${poppler_glib_public_headers} > ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${poppler_glib_public_headers}
          ${CMAKE_CURRENT_SOURCE_DIR}/poppler-enums.h.template
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.c
  COMMAND ${GLIB2_MKENUMS_PYTHON}
  ARGS
    ${GLIB2_MKENUMS}
    --template poppler-enums.c.template
    ${poppler_glib_public_headers} > ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${poppler_glib_public_headers} ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.h
          ${CMAKE_CURRENT_SOURCE_DIR}/poppler-enums.c.template
)


set(poppler_glib_SRCS
  poppler-action.cc
  poppler-date.cc
  poppler-document.cc
  poppler-page.cc
  poppler-attachment.cc
  poppler-form-field.cc
  poppler-annot.cc
  poppler-layer.cc
  poppler-movie.cc
  poppler-media.cc
  poppler.cc
  poppler-cached-file-loader.cc
  poppler-input-stream.cc
  poppler-structure-element.cc
)
set(poppler_glib_generated_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.c
  ${CMAKE_SOURCE_DIR}/poppler/CairoFontEngine.cc
  ${CMAKE_SOURCE_DIR}/poppler/CairoOutputDev.cc
  ${CMAKE_SOURCE_DIR}/poppler/CairoRescaleBox.cc
)
add_library(poppler-glib ${poppler_glib_SRCS} ${poppler_glib_generated_SRCS})
generate_export_header(poppler-glib EXPORT_MACRO_NAME POPPLER_PUBLIC EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/poppler-macros.h")
set_target_properties(poppler-glib PROPERTIES VERSION 8.26.0 SOVERSION 8)
if(MINGW AND BUILD_SHARED_LIBS)
    get_target_property(POPPLER_GLIB_SOVERSION poppler-glib SOVERSION)
    set_target_properties(poppler-glib PROPERTIES SUFFIX "-${POPPLER_GLIB_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler-glib poppler PkgConfig::GLIB2 ${CAIRO_LIBRARIES} Freetype::Freetype)
target_include_directories(poppler-glib SYSTEM PRIVATE ${CAIRO_INCLUDE_DIRS})
install(TARGETS poppler-glib RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (ENABLE_NSS3)
    target_include_directories(poppler-glib SYSTEM PRIVATE ${NSS3_INCLUDE_DIRS})
endif()

install(FILES
  ${poppler_glib_public_headers}
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-features.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-macros.h
  DESTINATION include/poppler/glib)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.h ${CMAKE_CURRENT_BINARY_DIR}/poppler-enums.c"
)

# GObject Introspection
if (HAVE_INTROSPECTION AND BUILD_SHARED_LIBS)
  include(GObjectIntrospectionMacros)

  # General gir: Reset object-list for introspection & load tool args
  set(INTROSPECTION_GIRS)
  set(INTROSPECTION_SCANNER_ARGS "--add-include-path=${CMAKE_CURRENT_SOURCE_DIR}" "--warn-all")
  set(INTROSPECTION_COMPILER_ARGS ${INTROSPECTION_COMPILER_ARGS} "--includedir=${CMAKE_CURRENT_SOURCE_DIR}")

  # Poppler: Assign package to gir & export keys
  set(Poppler_0_18_gir "poppler-glib")
  set(Poppler_0_18_gir_EXPORT_PACKAGES "poppler-glib")
  # Then load library and header lists
  set(Poppler_0_18_gir_LIBS "poppler-glib" "poppler")
  set(Poppler_0_18_gir_INCLUDES "GObject-2.0" "Gio-2.0" "cairo-1.0")

  # Format list of include directories as compiler flags
  get_directory_property(_tmp_includes INCLUDE_DIRECTORIES)
  _gir_list_prefix(_includes _tmp_includes "-I")
  # And set flags for gir compiler and scanner
  set(Poppler_0_18_gir_CFLAGS ${_includes} -L${CMAKE_BINARY_DIR} -L${CMAKE_CURRENT_BINARY_DIR})
  set(Poppler_0_18_gir_SCANNERFLAGS "--c-include=poppler.h")

  # Load temporary source-file lists, including a few generated at build
  set(orig_introspect_srcs ${poppler_glib_SRCS} ${poppler_glib_public_headers})
  set(gen_introspect_srcs "poppler-enums.c" "poppler-enums.h" "poppler-features.h" "poppler-macros.h")
  # Prefix the files with their correct directories for full paths
  _gir_list_prefix(_orig_introspect_paths orig_introspect_srcs "${CMAKE_CURRENT_SOURCE_DIR}/")
  _gir_list_prefix(_gen_introspect_paths gen_introspect_srcs "${CMAKE_CURRENT_BINARY_DIR}/")
  # Now load them to the final file list
  set(Poppler_0_18_gir_FILES ${_orig_introspect_paths} ${_gen_introspect_paths})

  # Finally, load the list of objects for introspection & invoke macro
  list(APPEND INTROSPECTION_GIRS Poppler-0.18.gir)
  gir_add_introspections(INTROSPECTION_GIRS)
endif ()

if(ENABLE_GTK_DOC)
  add_subdirectory(reference)
endif()

check_function_exists(explicit_bzero HAVE_EXPLICIT_BZERO)
