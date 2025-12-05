
set (perf_test_SRCS
  perf-test.cc
)
add_executable(perf-test ${perf_test_SRCS})
target_link_libraries(perf-test poppler)

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

if(ENABLE_NSS3)
  set(pdf_validate_signature_SRCS
    pdf-validate-signature.cc
  )
  add_executable(pdf-validate-signature ${pdf_validate_signature_SRCS})
  target_link_libraries(pdf-validate-signature poppler)

  set(VALIDATE_SIGNATURE_PATH ${EXECUTABLE_OUTPUT_PATH}/pdf-validate-signature)

  file(GLOB files-valid ${TESTDATADIR}/unittestcases/signature/valid/*.pdf)
  file(GLOB files-invalid ${TESTDATADIR}/unittestcases/signature/invalid/*.pdf)
  file(GLOB files-digest-mismatch ${TESTDATADIR}/unittestcases/signature/digest_mismatch/*.pdf)

  foreach(filename ${files-valid})
    file(RELATIVE_PATH testname ${TESTDATADIR}/unittestcases ${filename})
    add_test(
      NAME ${testname}
      COMMAND ${VALIDATE_SIGNATURE_PATH} ${filename} --valid
  )
  endforeach()

  foreach(filename ${files-invalid})
    file(RELATIVE_PATH testname ${TESTDATADIR}/unittestcases ${filename})
    add_test(
      NAME ${testname}
      COMMAND ${VALIDATE_SIGNATURE_PATH} ${filename} --invalid
  )
  endforeach()

  foreach(filename ${files-digest-mismatch})
    file(RELATIVE_PATH testname ${TESTDATADIR}/unittestcases ${filename})
    add_test(
      NAME ${testname}
      COMMAND ${VALIDATE_SIGNATURE_PATH} ${filename} --digest-mismatch
  )
  endforeach()

endif()

if(ENABLE_NSS3)
  set(pdf_signing_nss_SRCS
    pdf-signing-nss.cc
  )
  add_executable(pdf-signing-nss ${pdf_signing_nss_SRCS})
  target_include_directories(pdf-signing-nss SYSTEM PRIVATE ${NSS3_INCLUDE_DIRS})
  target_link_libraries(pdf-signing-nss poppler)

  set(NSS_SIGNING_PATH ${EXECUTABLE_OUTPUT_PATH}/pdf-signing-nss)

  set(NSS_SIGNING_INPUT_FILE ${TESTDATADIR}/unittestcases/WithActualText.pdf)
  set(NSS_SIGNING_WORKDIR ${TESTDATADIR}/unittestcases/signing_nss)
  set(NSSDIR_SIGN ${NSS_SIGNING_WORKDIR}/db_complete/)
  set(NSSDIR_VERIFY ${NSS_SIGNING_WORKDIR}/db_root_only/)

  add_test(
      NAME nss-sign-rsa-shortchain
      COMMAND ${NSS_SIGNING_PATH} --sign ${NSSDIR_SIGN} "RSA test key 01" ${NSS_SIGNING_INPUT_FILE} ${NSS_SIGNING_WORKDIR}/nss-rsa-shortchain.pdf
  )
  add_test(
      NAME nss-verify-rsa-shortchain
      COMMAND ${NSS_SIGNING_PATH} --verify ${NSSDIR_VERIFY} ${NSS_SIGNING_WORKDIR}/nss-rsa-shortchain.pdf
  )
  set_tests_properties(nss-sign-rsa-shortchain PROPERTIES FIXTURES_SETUP FIX_RSA01)
  set_tests_properties(nss-verify-rsa-shortchain PROPERTIES FIXTURES_REQUIRED FIX_RSA01)
  set_tests_properties(nss-sign-rsa-shortchain PROPERTIES RESOURCE_LOCK LOCK_NSSDDB_SIGN)
  set_tests_properties(nss-verify-rsa-shortchain PROPERTIES RESOURCE_LOCK LOCK_NSSDB_VERIFY)

  add_test(
      NAME nss-sign-rsa-moderatechain
      COMMAND ${NSS_SIGNING_PATH} --sign ${NSSDIR_SIGN} "RSA test key 02" ${NSS_SIGNING_INPUT_FILE} ${NSS_SIGNING_WORKDIR}/nss-rsa-moderatechain.pdf
  )
  add_test(
      NAME nss-verify-rsa-moderatechain
      COMMAND ${NSS_SIGNING_PATH} --verify ${NSSDIR_VERIFY} ${NSS_SIGNING_WORKDIR}/nss-rsa-moderatechain.pdf
  )
  set_tests_properties(nss-sign-rsa-moderatechain PROPERTIES FIXTURES_SETUP FIX_RSA02)
  set_tests_properties(nss-verify-rsa-moderatechain PROPERTIES FIXTURES_REQUIRED FIX_RSA02)
  set_tests_properties(nss-sign-rsa-moderatechain PROPERTIES RESOURCE_LOCK LOCK_NSSDDB_SIGN)
  set_tests_properties(nss-verify-rsa-moderatechain PROPERTIES RESOURCE_LOCK LOCK_NSSDB_VERIFY)

  add_test(
      NAME nss-sign-ecdsa-shortchain
      COMMAND ${NSS_SIGNING_PATH} --sign ${NSSDIR_SIGN} "ECDSA test key 01" ${NSS_SIGNING_INPUT_FILE} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-shortchain.pdf
  )
  add_test(
      NAME nss-verify-ecdsa-shortchain
      COMMAND ${NSS_SIGNING_PATH} --verify ${NSSDIR_VERIFY} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-shortchain.pdf
  )
  set_tests_properties(nss-sign-ecdsa-shortchain PROPERTIES FIXTURES_SETUP FIX_ECDSA01)
  set_tests_properties(nss-verify-ecdsa-shortchain PROPERTIES FIXTURES_REQUIRED FIX_ECDSA01)
  set_tests_properties(nss-sign-ecdsa-shortchain PROPERTIES RESOURCE_LOCK LOCK_NSSDDB_SIGN)
  set_tests_properties(nss-verify-ecdsa-shortchain PROPERTIES RESOURCE_LOCK LOCK_NSSDB_VERIFY)

  add_test(
      NAME nss-sign-ecdsa-moderatechain
      COMMAND ${NSS_SIGNING_PATH} --sign ${NSSDIR_SIGN} "ECDSA test key 02" ${NSS_SIGNING_INPUT_FILE} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-moderatechain.pdf
  )
  add_test(
      NAME nss-verify-ecdsa-moderatechain
      COMMAND ${NSS_SIGNING_PATH} --verify ${NSSDIR_VERIFY} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-moderatechain.pdf
  )
  set_tests_properties(nss-sign-ecdsa-moderatechain PROPERTIES FIXTURES_SETUP FIX_ECDSA02)
  set_tests_properties(nss-verify-ecdsa-moderatechain PROPERTIES FIXTURES_REQUIRED FIX_ECDSA02)
  set_tests_properties(nss-sign-ecdsa-moderatechain PROPERTIES RESOURCE_LOCK LOCK_NSSDDB_SIGN)
  set_tests_properties(nss-verify-ecdsa-moderatechain PROPERTIES RESOURCE_LOCK LOCK_NSSDB_VERIFY)

  add_test(
      NAME nss-sign-ecdsa-longchain
      COMMAND ${NSS_SIGNING_PATH} --sign ${NSSDIR_SIGN} "ECDSA test key 03" ${NSS_SIGNING_INPUT_FILE} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-longchain.pdf
  )
  add_test(
      NAME nss-verify-ecdsa-longchain
      COMMAND ${NSS_SIGNING_PATH} --verify ${NSSDIR_VERIFY} ${NSS_SIGNING_WORKDIR}/nss-ecdsa-longchain.pdf
  )
  set_tests_properties(nss-sign-ecdsa-longchain PROPERTIES FIXTURES_SETUP FIX_ECDSA03)
  set_tests_properties(nss-verify-ecdsa-longchain PROPERTIES FIXTURES_REQUIRED FIX_ECDSA03)
  set_tests_properties(nss-sign-ecdsa-longchain PROPERTIES RESOURCE_LOCK LOCK_NSSDDB_SIGN)
  set_tests_properties(nss-verify-ecdsa-longchain PROPERTIES RESOURCE_LOCK LOCK_NSSDB_VERIFY)

endif()
