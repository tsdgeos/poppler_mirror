
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
    ${CMAKE_SOURCE_DIR}/glib
    ${CMAKE_BINARY_DIR}/glib
  )

  set (gtk_splash_test_SRCS
    gtk-test.cc
  )
  poppler_add_test(gtk-test BUILD_GTK_TESTS ${gtk_splash_test_SRCS})
  target_link_libraries(gtk-test ${CAIRO_LIBRARIES} poppler-glib PkgConfig::GTK3)
  target_include_directories(gtk-test SYSTEM PRIVATE ${CAIRO_INCLUDE_DIRS})

  if (HAVE_CAIRO)

    set (pdf_inspector_SRCS
      pdf-inspector.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoFontEngine.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoOutputDev.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoRescaleBox.cc
    )
    poppler_add_test(pdf-inspector BUILD_GTK_TESTS ${pdf_inspector_SRCS})
    target_link_libraries(pdf-inspector ${CAIRO_LIBRARIES} Freetype::Freetype ${common_libs} PkgConfig::GTK3 poppler)
    target_include_directories(pdf-inspector SYSTEM PRIVATE ${CAIRO_INCLUDE_DIRS})
    target_compile_definitions(pdf-inspector PRIVATE -DSRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  endif ()

endif ()

if (HAVE_CAIRO)
  include(CheckCXXSymbolExists)
  set (CMAKE_REQUIRED_INCLUDES ${CAIRO_INCLUDE_DIRS})
  check_cxx_symbol_exists(CAIRO_HAS_PNG_FUNCTIONS "cairo.h" HAVE_CAIRO_PNG)
  check_cxx_symbol_exists(CAIRO_HAS_PDF_SURFACE "cairo.h" HAVE_CAIRO_PDF)
  check_cxx_symbol_exists(CAIRO_HAS_PS_SURFACE "cairo.h" HAVE_CAIRO_PS)
  check_cxx_symbol_exists(CAIRO_HAS_SVG_SURFACE "cairo.h" HAVE_CAIRO_SVG)

  if (HAVE_CAIRO_PNG AND HAVE_CAIRO_PDF AND HAVE_CAIRO_PS AND HAVE_CAIRO_SVG)
    find_package(Threads)
    set(cairo_thread_test_SRCS
      cairo-thread-test.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoFontEngine.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoOutputDev.cc
      ${CMAKE_SOURCE_DIR}/poppler/CairoRescaleBox.cc
      )
    add_executable(cairo-thread-test ${cairo_thread_test_SRCS})
    target_link_libraries(cairo-thread-test ${CAIRO_LIBRARIES} Freetype::Freetype Threads::Threads poppler)
    target_include_directories(cairo-thread-test SYSTEM PRIVATE ${CAIRO_INCLUDE_DIRS})
  endif ()
endif ()

set (pdf_fullrewrite_SRCS
  pdf-fullrewrite.cc
  ../utils/parseargs.cc
)
add_executable(pdf-fullrewrite ${pdf_fullrewrite_SRCS})
target_link_libraries(pdf-fullrewrite poppler)

# Tests for the image embedding API.
if(ENABLE_LIBPNG OR ENABLE_LIBJPEG)
  set(image_embedding_SRCS
    image-embedding.cc
    ../utils/parseargs.cc
  )
  add_executable(image-embedding ${image_embedding_SRCS})
  target_link_libraries(image-embedding poppler)

  set(INPUT_PDF ${TESTDATADIR}/unittestcases/xr01.pdf)
  set(IMG_DIR ${TESTDATADIR}/unittestcases/images)
  set(IMAGE_EMBEDDING_PATH ${EXECUTABLE_OUTPUT_PATH}/image-embedding)

  if (ENABLE_LIBPNG)
    add_test(
      NAME embed-png-g1
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-g1.png -depth 8 -colorspace DeviceGray
    )
    add_test(
      NAME embed-png-g2
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-g2.png -depth 8 -colorspace DeviceGray
    )
    add_test(
      NAME embed-png-g4
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-g4.png -depth 8 -colorspace DeviceGray
    )
    add_test(
      NAME embed-png-g8
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-g8.png -depth 8 -colorspace DeviceGray
    )
    add_test(
      NAME embed-png-g16
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-g16.png -depth 16 -colorspace DeviceGray
    )
    add_test(
      NAME embed-png-ga8
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-ga8.png -depth 8 -colorspace DeviceGray -smask
    )
    add_test(
      NAME embed-png-ga16
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-ga16.png -depth 16 -colorspace DeviceGray -smask
    )
    add_test(
      NAME embed-png-palette
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-palette.png -depth 8 -colorspace DeviceRGB
    )
    add_test(
      NAME embed-png-rgb8
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-rgb8.png -depth 8 -colorspace DeviceRGB
    )
    add_test(
      NAME embed-png-rgb16
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-rgb16.png -depth 16 -colorspace DeviceRGB
    )
    add_test(
      NAME embed-png-rgba8
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-rgba8.png -depth 8 -colorspace DeviceRGB -smask
    )
    add_test(
      NAME embed-png-rgba16
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/png-rgba16.png -depth 16 -colorspace DeviceRGB -smask
    )
    add_test(
      NAME embed-malformed-png
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/malformed.png -fail
    )
  endif()

  if(ENABLE_LIBJPEG)
    add_test(
      NAME embed-jpeg
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/jpeg.jpg -depth 8 -colorspace DeviceRGB -filter DCTDecode
    )
    add_test(
      NAME embed-malformed-jpeg
      COMMAND ${IMAGE_EMBEDDING_PATH} ${INPUT_PDF} ${IMG_DIR}/malformed.jpg -fail
    )
  endif()

  unset(IMAGE_EMBEDDING_PATH)
  unset(IMG_DIR)
  unset(INPUT_PDF)
endif()
