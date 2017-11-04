prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_LIBDIR@
includedir=@CMAKE_INSTALL_INCLUDEDIR@

Name: poppler
Description: PDF rendering library
Version: @POPPLER_VERSION@

Libs: -L${libdir} -lpoppler
Cflags: -I${includedir}/poppler
