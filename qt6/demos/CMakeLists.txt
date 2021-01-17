include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../src
)

set(poppler_qt6viewer_SRCS
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

poppler_add_test(poppler_qt6viewer BUILD_QT6_TESTS ${poppler_qt6viewer_SRCS})
target_link_libraries(poppler_qt6viewer poppler-qt6 Qt6::Widgets)
