set(CMAKE_AUTOMOC ON)

set(ENABLE_QT_STRICT_ITERATORS ON CACHE BOOL "Select whether to compile with QT_STRICT_ITERATORS. Leave it ON, unless your Qt lacks support, or your compiler can't do SRA optimization.")
if(ENABLE_QT_STRICT_ITERATORS)
  add_definitions(-DQT_STRICT_ITERATORS)
endif()

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050C00)
add_definitions(-DQT_NO_DEPRECATED_WARNINGS)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(demos)
