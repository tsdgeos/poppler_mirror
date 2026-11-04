prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: poppler-glib
Description: GLib wrapper for poppler
Version: @POPPLER_VERSION@
Requires: glib-2.0 >= @GLIB_REQUIRED_VERSION@ gobject-2.0 >= @GLIB_REQUIRED_VERSION@ cairo >= @CAIRO_REQUIRED_VERSION@
Requires.private: gio-2.0 >= @GLIB_REQUIRED_VERSION@ \
                  freetype2 >= @FREETYPE_REQUIRED_VERSION@ \
                  poppler = @POPPLER_VERSION@

Libs: -L${libdir} -lpoppler-glib
Cflags: -I${includedir}/poppler/glib
