prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_LIBDIR@
includedir=@CMAKE_INSTALL_INCLUDEDIR@

Name: poppler-cairo
Description: Cairo backend for Poppler PDF rendering library
Version: @POPPLER_VERSION@
Requires: poppler = @POPPLER_VERSION@ cairo >= @CAIRO_VERSION@
