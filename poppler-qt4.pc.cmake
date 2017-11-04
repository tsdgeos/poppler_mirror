prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_LIBDIR@
includedir=@CMAKE_INSTALL_INCLUDEDIR@

Name: poppler-qt4
Description: Qt4 bindings for poppler
Version: @POPPLER_VERSION@
Requires: @PC_REQUIRES@
@PC_REQUIRES_PRIVATE@

Libs: -L${libdir} -lpoppler-qt4
Cflags: -I${includedir}/poppler/qt4
