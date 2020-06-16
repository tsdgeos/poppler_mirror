set(CMAKE_AUTOMOC ON)

set(ENABLE_QT_STRICT_ITERATORS ON CACHE BOOL "Select whether to compile with QT_STRICT_ITERATORS. Leave it ON, unless your Qt lacks support, or your compiler can't do SRA optimization.")
if(ENABLE_QT_STRICT_ITERATORS)
  add_definitions(-DQT_STRICT_ITERATORS)
endif()

add_subdirectory(src)

add_subdirectory(tests)
add_subdirectory(demos)
