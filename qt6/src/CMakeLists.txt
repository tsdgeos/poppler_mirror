add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

configure_file(poppler-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h @ONLY)

set(poppler_qt6_SRCS
  poppler-annotation.cc
  poppler-document.cc
  poppler-embeddedfile.cc
  poppler-fontinfo.cc
  poppler-form.cc
  poppler-link.cc
  poppler-link-extractor.cc
  poppler-movie.cc
  poppler-optcontent.cc
  poppler-page.cc
  poppler-base-converter.cc
  poppler-pdf-converter.cc
  poppler-private.cc
  poppler-ps-converter.cc
  poppler-qiodeviceinstream.cc
  poppler-qiodeviceoutstream.cc
  poppler-sound.cc
  poppler-textbox.cc
  poppler-page-transition.cc
  poppler-media.cc
  poppler-outline.cc
  ArthurOutputDev.cc
  poppler-version.cpp
)
add_library(poppler-qt6 ${poppler_qt6_SRCS})
set_target_properties(poppler-qt6 PROPERTIES VERSION 1.0.0 SOVERSION 1)
if(MINGW AND BUILD_SHARED_LIBS)
    get_target_property(POPPLER_QT6_SOVERSION poppler-qt6 SOVERSION)
    set_target_properties(poppler-qt6 PROPERTIES SUFFIX "-${POPPLER_QT6_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler-qt6 poppler Qt6::Core Qt6::Gui Qt6::Xml ${FREETYPE_LIBRARIES})
if(MSVC)
target_link_libraries(poppler-qt6 poppler ${poppler_LIBS})
endif()
if (ENABLE_NSS3)
    target_include_directories(poppler-qt6 SYSTEM PRIVATE ${NSS3_INCLUDE_DIRS})
endif()
if(USE_CMS)
    target_link_libraries(poppler-qt6 poppler ${LCMS2_LIBRARIES})
endif()
install(TARGETS poppler-qt6 RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES
  poppler-qt6.h
  poppler-link.h
  poppler-annotation.h
  poppler-form.h
  poppler-optcontent.h
  poppler-export.h
  poppler-page-transition.h
  poppler-media.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h
  DESTINATION include/poppler/qt6)
