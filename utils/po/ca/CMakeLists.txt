get_filename_component(_lang ${CMAKE_CURRENT_SOURCE_DIR} NAME)
gettext_process_po_files(${_lang} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_LOCALEDIR} PO_FILES
  pdfsig.po
)
