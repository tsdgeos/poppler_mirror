file(GLOB _files "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach (_dir ${_files})
  if (IS_DIRECTORY "${_dir}" AND EXISTS "${_dir}/CMakeLists.txt")
    add_subdirectory("${_dir}")
    get_filename_component(_lang ${_dir} NAME)
    list(APPEND _langs ${_lang})
  endif ()
endforeach()


macro(UPDATE_POT_FILE input)
  set(_potFile "${input}.pot")
  foreach(_lang ${_langs})
    set(_poFile "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}/${input}.po")
    list(APPEND _commands
      COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s ${_poFile} "${CMAKE_CURRENT_SOURCE_DIR}/${_potFile}")
  endforeach()
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${_potFile}
                    COMMAND xgettext --keyword=_ --language=c++ --package-name=pdfsig --output-dir=po --output=${_potFile} ${input}.cc
                    ${_commands}
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../${input}.cc
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
  add_custom_target(update_pot_${input}
                    ALL
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_potFile})
endmacro()

UPDATE_POT_FILE(pdfsig)
