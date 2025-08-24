cmake_minimum_required(VERSION 3.22)
project(pkgconfig_test C CXX)

find_package(PkgConfig REQUIRED)

# ======= POPPLER CPP =======
pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

add_executable(poppler_cpp_test poppler_cpp_test.cpp)

target_include_directories(poppler_cpp_test PRIVATE ${POPPLER_CPP_INCLUDE_DIRS})
target_link_libraries(poppler_cpp_test PRIVATE ${POPPLER_CPP_LIBRARIES})

# ======= POPPLER GLIB =======
pkg_check_modules(POPPLER_GLIB REQUIRED poppler-glib)

add_executable(poppler_glib_test poppler_glib_test.c)
set_target_properties(poppler_glib_test PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(poppler_glib_test PRIVATE ${POPPLER_GLIB_INCLUDE_DIRS})
target_link_libraries(poppler_glib_test PRIVATE ${POPPLER_GLIB_LIBRARIES} stdc++)

# Since both Qt5 and Qt6 define Qt::Core target, we put them into separate subdirectories
# ======= POPPLER QT5 =======
add_subdirectory(poppler_qt5_test)

# ======= POPPLER QT6 =======
add_subdirectory(poppler_qt6_test)
