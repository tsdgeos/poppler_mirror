include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../src
)

set(poppler_qt5viewer_SRCS
  abstractinfodock.cpp
  documentobserver.cpp
  embeddedfiles.cpp
  fonts.cpp
  info.cpp
  main_viewer.cpp
  metadata.cpp
  navigationtoolbar.cpp
  optcontent.cpp
  pageview.cpp
  permissions.cpp
  thumbnails.cpp
  toc.cpp
  viewer.cpp
)

poppler_add_test(poppler_qt5viewer BUILD_QT5_TESTS ${poppler_qt5viewer_SRCS})
target_link_libraries(poppler_qt5viewer poppler-qt5 Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml)
