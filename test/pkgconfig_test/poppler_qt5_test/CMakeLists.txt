pkg_check_modules(POPPLER_QT5 REQUIRED poppler-qt5)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(poppler_qt5_test poppler_qt5_test.cpp)

target_include_directories(poppler_qt5_test PRIVATE ${POPPLER_QT5_INCLUDE_DIRS})
target_link_libraries(poppler_qt5_test PRIVATE Qt5::Core ${POPPLER_QT5_LIBRARIES})
