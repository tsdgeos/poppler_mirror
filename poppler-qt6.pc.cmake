prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: poppler-qt6
Description: Qt6 bindings for poppler
Version: @POPPLER_VERSION@
Requires.private: freetype2 >= @FREETYPE_VERSION@ \
                  poppler = @POPPLER_VERSION@

Libs: -L${libdir} -lpoppler-qt6
Libs.private: -lQt6Gui -lQt6Core
Cflags: -I${includedir}/poppler/qt6
