pkg_check_modules(POPPLER_QT6 REQUIRED poppler-qt6)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(poppler_qt6_test poppler_qt6_test.cpp)

target_include_directories(poppler_qt6_test PRIVATE ${POPPLER_QT6_INCLUDE_DIRS})
target_link_libraries(poppler_qt6_test PRIVATE Qt6::Core ${POPPLER_QT6_LIBRARIES})
