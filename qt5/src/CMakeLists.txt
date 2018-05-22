add_definitions(${QT5_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Qt5Core_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

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
  poppler-qiodeviceoutstream.cc
  poppler-sound.cc
  poppler-textbox.cc
  poppler-page-transition.cc
  poppler-media.cc
  ArthurOutputDev.cc
)
add_library(poppler-qt5 ${poppler_qt5_SRCS})
set_target_properties(poppler-qt5 PROPERTIES VERSION 1.15.0 SOVERSION 1)
if(MINGW)
    get_target_property(POPPLER_QT5_SOVERSION poppler-qt5 SOVERSION)
    set_target_properties(poppler-qt5 PROPERTIES SUFFIX "-${POPPLER_QT5_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler-qt5 poppler ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Xml_LIBRARIES})
if(MSVC)
target_link_libraries(poppler-qt5 poppler ${poppler_LIBS})
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
  DESTINATION include/poppler/qt5)

