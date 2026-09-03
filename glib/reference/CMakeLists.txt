# The docs generation scans the glib build dir while g-ir-scanner may still be
# writing its temporary introspection files there, racing the docs build.
# Serialize the two targets by depending on the gir output when introspection
# is enabled.
set(_poppler_glib_docs_deps poppler-glib)
if (HAVE_INTROSPECTION AND BUILD_SHARED_LIBS)
  list(APPEND _poppler_glib_docs_deps ${CMAKE_BINARY_DIR}/glib/Poppler-0.18.gir)
endif ()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/glib-docs-build.stamp
  DEPENDS ${_poppler_glib_docs_deps}
  COMMAND ${CMAKE_SOURCE_DIR}/make-glib-api-docs --src-dir=${CMAKE_SOURCE_DIR} --build-dir=${CMAKE_BINARY_DIR}
  COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/glib-docs-build.stamp
)

add_custom_target(glib-docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/glib-docs-build.stamp)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION "${CMAKE_INSTALL_DATADIR}/gtk-doc/html/poppler"
)
