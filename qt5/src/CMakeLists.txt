add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

configure_file(poppler-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h @ONLY)

set(poppler_qt5_SRCS
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
  QPainterOutputDev.cc
  poppler-version.cpp
)
add_library(poppler-qt5 ${poppler_qt5_SRCS})
set_target_properties(poppler-qt5 PROPERTIES VERSION 1.26.0 SOVERSION 1)
if(MINGW AND BUILD_SHARED_LIBS)
    get_target_property(POPPLER_QT5_SOVERSION poppler-qt5 SOVERSION)
    set_target_properties(poppler-qt5 PROPERTIES SUFFIX "-${POPPLER_QT5_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler-qt5 poppler Qt5::Core Qt5::Gui Qt5::Xml ${FREETYPE_LIBRARIES})
if(MSVC)
target_link_libraries(poppler-qt5 poppler ${poppler_LIBS})
endif()
if (ENABLE_NSS3)
    target_include_directories(poppler-qt5 SYSTEM PRIVATE ${NSS3_INCLUDE_DIRS})
endif()
if(USE_CMS)
    target_link_libraries(poppler-qt5 poppler ${LCMS2_LIBRARIES})
endif()
install(TARGETS poppler-qt5 RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES
  poppler-qt5.h
  poppler-link.h
  poppler-annotation.h
  poppler-form.h
  poppler-optcontent.h
  poppler-export.h
  poppler-page-transition.h
  poppler-media.h
  ${CMAKE_CURRENT_BINARY_DIR}/poppler-version.h
  DESTINATION include/poppler/qt5)

