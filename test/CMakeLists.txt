
if (HAVE_NANOSLEEP OR LIB_RT_HAS_NANOSLEEP)
  set (perf_test_SRCS
    perf-test.cc
    perf-test-preview-dummy.cc
  )
  add_executable(perf-test ${perf_test_SRCS})
  target_link_libraries(perf-test poppler)
  if (LIB_RT_HAS_NANOSLEEP)
    target_link_libraries(perf-test rt)
  endif ()
endif ()

if (GTK_FOUND)

  include_directories(
    SYSTEM
    ${GTK3_INCLUDE_DIRS}
  )

  include_directories(
    ${CMAKE_SOURCE_DIR}/glib
    ${CMAKE_BINARY_DIR}/glib
  )

  set (gtk_splash_test_SRCS
    gtk-test.cc
  )
  poppler_add_test(gtk-test BUILD_GTK_TESTS ${gtk_splash_test_SRCS})
  target_link_libraries(gtk-test ${CAIRO_LIBRARIES} poppler-glib ${GTK3_LIBRARIES})

  if (HAVE_CAIRO)

    set (pdf_inspector_SRCS
      pdf-inspector.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoFontEngine.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoOutputDev.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoRescaleBox.cc
    )
    poppler_add_test(pdf-inspector BUILD_GTK_TESTS ${pdf_inspector_SRCS})
    target_link_libraries(pdf-inspector ${CAIRO_LIBRARIES} ${FREETYPE_LIBRARIES} ${common_libs} ${GTK3_LIBRARIES} poppler)
    target_compile_definitions(pdf-inspector PRIVATE -DSRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(CMAKE_USE_PTHREADS_INIT)
       target_link_libraries(pdf-inspector Threads::Threads)
    endif()

  endif ()

endif ()

set (pdf_fullrewrite_SRCS
  pdf-fullrewrite.cc
  ../utils/parseargs.cc
)
add_executable(pdf-fullrewrite ${pdf_fullrewrite_SRCS})
target_link_libraries(pdf-fullrewrite poppler)


