include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(poppler-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h @ONLY)

add_subdirectory(tests)

set(poppler_cpp_SRCS
  poppler-destination.cpp
  poppler-document.cpp
  poppler-embedded-file.cpp
  poppler-font.cpp
  poppler-global.cpp
  poppler-image.cpp
  poppler-page.cpp
  poppler-page-renderer.cpp
  poppler-page-transition.cpp
  poppler-private.cpp
  poppler-rectangle.cpp
  poppler-toc.cpp
  poppler-version.cpp
)

add_library(poppler-cpp ${poppler_cpp_SRCS})
generate_export_header(poppler-cpp BASE_NAME poppler-cpp EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/poppler_cpp_export.h")
set_target_properties(poppler-cpp PROPERTIES VERSION 3.0.0 SOVERSION 3)
if(MINGW AND BUILD_SHARED_LIBS)
    get_target_property(POPPLER_CPP_SOVERSION poppler-cpp SOVERSION)
    set_target_properties(poppler-cpp PROPERTIES SUFFIX "-${POPPLER_CPP_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler-cpp poppler Iconv::Iconv)
install(TARGETS poppler-cpp RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(poppler_cpp_all_install_headers
  poppler-destination.h
  poppler-document.h
  poppler-embedded-file.h
  poppler-font.h
  poppler-font-private.h
  poppler-global.h
  poppler-image.h
  poppler-page.h
  poppler-page-renderer.h
  poppler-page-transition.h
  poppler-rectangle.h
  poppler-toc.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler_cpp_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h
)

if (${CMAKE_VERSION} VERSION_LESS "3.23.0")
  install(FILES ${poppler_cpp_all_install_headers} DESTINATION include/poppler/cpp)
else()
  target_sources(poppler-cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}
    FILES
    ${poppler_cpp_all_install_headers}
  )
  install(TARGETS poppler-cpp FILE_SET HEADERS DESTINATION include/poppler/cpp)
endif()
