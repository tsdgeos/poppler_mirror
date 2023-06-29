add_definitions(-DTESTDATADIR=\"${TESTDATADIR}\")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../src
)

macro(QT5_ADD_SIMPLETEST exe source)
  string(REPLACE "-" "" test_name ${exe})
  set(${test_name}_SOURCES
    ${source}
  )
  poppler_add_test(${exe} BUILD_QT5_TESTS ${${test_name}_SOURCES})
  target_link_libraries(${exe} poppler-qt5 Qt5::Widgets)
endmacro(QT5_ADD_SIMPLETEST)

macro(QT5_ADD_QTEST exe source)
  if (Qt5Test_FOUND)
    string(REPLACE "-" "" test_name ${exe})
    set(${test_name}_SOURCES
      ${source}
    )
    poppler_add_test(${exe} BUILD_QT5_TESTS ${${test_name}_SOURCES})
    add_test(${exe} ${EXECUTABLE_OUTPUT_PATH}/${exe})
    target_link_libraries(${exe} poppler-qt5 Qt5::Widgets Qt5::Test Qt5::Core Qt5::Gui)
  endif ()
endmacro(QT5_ADD_QTEST)

macro(QT_ADD_FUZZER exe)
  string(REPLACE "-" "" test_name ${exe})
  set(${test_name}_SOURCES
    ${ARGN}
  )
  poppler_add_test(${exe} BUILD_QT5_TESTS ${${test_name}_SOURCES})
  target_link_libraries(${exe} poppler-qt5 Qt5::Widgets Qt5::Test Qt5::Core Qt5::Gui)
endmacro(QT_ADD_FUZZER)

qt5_add_simpletest(test-poppler-qt5 test-poppler-qt5.cpp)
qt5_add_simpletest(test-password-qt5 test-password-qt5.cpp)
qt5_add_simpletest(test-render-to-file-qt5 test-render-to-file.cpp)
qt5_add_simpletest(poppler-qt5-forms poppler-forms.cpp)
qt5_add_simpletest(poppler-qt5-fonts poppler-fonts.cpp)
qt5_add_simpletest(poppler-qt5-attachments poppler-attachments.cpp)
qt5_add_simpletest(stress-poppler-qt5 stress-poppler-qt5.cpp)
qt5_add_simpletest(stress-poppler-dir-qt5 stress-poppler-dir.cpp)
qt5_add_simpletest(stress-threads-qt5 stress-threads-qt5.cpp)
qt5_add_simpletest(poppler-qt5-texts poppler-texts.cpp)
qt5_add_simpletest(poppler-qt5-page-labels poppler-page-labels.cpp)

qt5_add_qtest(check_qt5_attachments check_attachments.cpp)
qt5_add_qtest(check_qt5_dateConversion check_dateConversion.cpp)
qt5_add_qtest(check_qt5_fonts check_fonts.cpp)
qt5_add_qtest(check_qt5_links check_links.cpp)
qt5_add_qtest(check_qt5_annotations check_annotations.cpp)
qt5_add_qtest(check_qt5_metadata check_metadata.cpp)
qt5_add_qtest(check_qt5_optcontent check_optcontent.cpp)
qt5_add_qtest(check_qt5_forms check_forms.cpp)
qt5_add_qtest(check_qt5_pagelayout check_pagelayout.cpp)
qt5_add_qtest(check_qt5_pagemode check_pagemode.cpp)
qt5_add_qtest(check_qt5_password check_password.cpp)
qt5_add_qtest(check_qt5_permissions check_permissions.cpp)
qt5_add_qtest(check_qt5_search check_search.cpp)
qt5_add_qtest(check_qt5_actualtext check_actualtext.cpp)
qt5_add_qtest(check_qt5_lexer check_lexer.cpp)
qt5_add_qtest(check_qt5_internal_outline check_internal_outline.cpp)
qt5_add_qtest(check_qt5_goostring check_goostring.cpp)
qt5_add_qtest(check_qt5_object check_object.cpp)
qt5_add_qtest(check_qt5_stroke_opacity check_stroke_opacity.cpp)
qt5_add_qtest(check_qt5_utf_conversion check_utf_conversion.cpp)
qt5_add_qtest(check_qt5_outline check_outline.cpp)
qt5_add_qtest(check_qt5_signature_basics check_signature_basics.cpp)
qt5_add_qtest(check_qt5_utf8document check_utf8document.cpp)
qt5_add_qtest(check_qt5_distinguished_name_parser check_distinguished_name_parser.cpp)
qt5_add_qtest(check_qt5_cidfontswidthsbuilder check_cidfontswidthsbuilder.cpp)
qt5_add_qtest(check_qt5_overprint check_overprint.cpp)
if (NOT WIN32)
  qt5_add_qtest(check_qt5_pagelabelinfo check_pagelabelinfo.cpp)
  qt5_add_qtest(check_qt5_strings check_strings.cpp)
endif ()

if(ENABLE_FUZZER)
    qt_add_fuzzer(qt_annot_fuzzer ./fuzzing/qt_annot_fuzzer.cc)
    qt_add_fuzzer(qt_pdf_fuzzer ./fuzzing/qt_pdf_fuzzer.cc)
    qt_add_fuzzer(qt_label_fuzzer ./fuzzing/qt_label_fuzzer.cc)
    qt_add_fuzzer(qt_search_fuzzer ./fuzzing/qt_search_fuzzer.cc)
    qt_add_fuzzer(qt_textbox_fuzzer ./fuzzing/qt_textbox_fuzzer.cc)
endif()
