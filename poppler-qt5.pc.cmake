prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: poppler-qt5
Description: Qt5 bindings for poppler
Version: @POPPLER_VERSION@
Requires.private: freetype2 >= @FREETYPE_VERSION@ \
                  poppler = @POPPLER_VERSION@

Libs: -L${libdir} -lpoppler-qt5
Libs.private: -lQt5Gui -lQt5Xml -lQt5Core
Cflags: -I${includedir}/poppler/qt5
