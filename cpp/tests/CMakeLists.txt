include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/..
  ${CMAKE_SOURCE_DIR}/utils
)

macro(CPP_ADD_SIMPLETEST exe)
  string(REPLACE "-" "" test_name ${exe})
  set(${test_name}_SOURCES
    ${ARGN}
  )
  poppler_add_test(${exe} BUILD_CPP_TESTS ${${test_name}_SOURCES})
  target_link_libraries(${exe} poppler-cpp poppler)
endmacro(CPP_ADD_SIMPLETEST)

cpp_add_simpletest(poppler-dump poppler-dump.cpp ${CMAKE_SOURCE_DIR}/utils/parseargs.cc)
cpp_add_simpletest(poppler-render poppler-render.cpp ${CMAKE_SOURCE_DIR}/utils/parseargs.cc)

if(ENABLE_FUZZER)
  cpp_add_simpletest(doc_fuzzer ./fuzzing/doc_fuzzer.cc)
  cpp_add_simpletest(pdf_fuzzer ./fuzzing/pdf_fuzzer.cc)
  cpp_add_simpletest(pdf_file_fuzzer ./fuzzing/pdf_file_fuzzer.cc)
  cpp_add_simpletest(page_label_fuzzer ./fuzzing/page_label_fuzzer.cc)
  cpp_add_simpletest(page_search_fuzzer ./fuzzing/page_search_fuzzer.cc)
endif()
