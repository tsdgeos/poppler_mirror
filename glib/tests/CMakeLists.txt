macro(POPPLER_ADD_TESTCASE exe arg1)
  add_test(${exe}-${arg1} ${EXE} ${EXECUTABLE_OUTPUT_PATH}/poppler-check-bb ${TESTDATADIR}/unittestcases/${arg1} ${ARGN})
endmacro(POPPLER_ADD_TESTCASE)

add_definitions(-DTESTDATADIR=\"${TESTDATADIR}\")

set(poppler_check_text_SRCS
  check_text.c
)
poppler_add_test(poppler-check-text BUILD_GTK_TESTS ${poppler_check_text_SRCS})
add_test(poppler-check-text ${EXECUTABLE_OUTPUT_PATH}/poppler-check-text)

set(poppler_check_bb_SRCS
  check_bb.c
)
poppler_add_test(poppler-check-bb BUILD_GTK_TESTS ${poppler_check_bb_SRCS})

target_link_libraries(poppler-check-text poppler-glib PkgConfig::GTK3)
target_link_libraries(poppler-check-bb poppler-glib PkgConfig::GTK3)

poppler_add_testcase(poppler-check-bb shapes+attachments.pdf 42.5 42.5 557.5 557.5)
poppler_add_testcase(poppler-check-bb orientation.pdf 34 34 83.74 49 793 34 808 97.19 488.02 793 561 808 34 503.61 49 561)
poppler_add_testcase(poppler-check-bb xr01.pdf 148.71 126.35 308.11 704.57)
poppler_add_testcase(poppler-check-bb xr02.pdf 133.77 124.81 308.11 704.57 133.77 124.80 308.11 704.57)
poppler_add_testcase(poppler-check-bb russian.pdf 71.5 76.81 197.69 131.09)
poppler_add_testcase(poppler-check-bb vis_policy_test.pdf 90 77.93 312.01 265.13)
poppler_add_testcase(poppler-check-bb searchAcrossLines.pdf 107.15 105.23 523.85 691 85.04 94 538.59 762.19)
poppler_add_testcase(poppler-check-bb deseret.pdf 56.8 57.15 109.5 72.8)
poppler_add_testcase(poppler-check-bb fieldWithUtf16Names.pdf 56.65 56.65 264.55 83.05)
poppler_add_testcase(poppler-check-bb bug7063.pdf 56.8 57.46 244.29 118.79)
poppler_add_testcase(poppler-check-bb WithActualText.pdf 100 90.72 331.01 102.35)
poppler_add_testcase(poppler-check-bb Issue637.pdf 70.87 53 293 105.37)
poppler_add_testcase(poppler-check-bb truetype.pdf 17.5 17.5 577.5 225.62)
poppler_add_testcase(poppler-check-bb form_set_icon.pdf 0 0 362.835 272.126)
poppler_add_testcase(poppler-check-bb imageretrieve+attachment.pdf 0 0 610.56 792)
poppler_add_testcase(poppler-check-bb checkbox_issue_159.pdf 2.84 14.17 553.18 840.87)
poppler_add_testcase(poppler-check-bb NestedLayers.pdf 0 191 612 792)
poppler_add_testcase(poppler-check-bb A6EmbeddedFiles.pdf 18 18 558.36 751.92)
poppler_add_testcase(poppler-check-bb latex-hyperref-checkbox-issue-655.pdf 148.71 123.81 308.11 704.57)
poppler_add_testcase(poppler-check-bb utf16le-annot.pdf 55.47 54.78 98.74 96.12)
poppler_add_testcase(poppler-check-bb type3.pdf -p 10 125.80 130 509.30 695 125.80 132 538.03 693)

add_executable(pdfdrawbb pdfdrawbb.c)
target_link_libraries(pdfdrawbb poppler-glib)

macro(GLIB_ADD_FUZZER exe)
  string(REPLACE "-" "" test_name ${exe})
  set(${test_name}_SOURCES
    ${ARGN}
  )
  poppler_add_test(${exe} BUILD_GTK_TESTS ${${test_name}_SOURCES})
  target_link_libraries(${exe} poppler-glib PkgConfig::GTK3)
endmacro(GLIB_ADD_FUZZER)

if(ENABLE_FUZZER)
    glib_add_fuzzer(annot_fuzzer ./fuzzing/annot_fuzzer.cc)
    glib_add_fuzzer(doc_attr_fuzzer ./fuzzing/doc_attr_fuzzer.cc)
    glib_add_fuzzer(find_text_fuzzer ./fuzzing/find_text_fuzzer.cc)
    glib_add_fuzzer(util_fuzzer ./fuzzing/util_fuzzer.cc)
    glib_add_fuzzer(label_fuzzer ./fuzzing/label_fuzzer.cc)
    glib_add_fuzzer(pdf_draw_fuzzer ./fuzzing/pdf_draw_fuzzer.cc)
endif()
