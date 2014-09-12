#include <cairo.h>
#include "goo/gmem.h"
#include "goo/gtypes.h"
#include "goo/gtypes_p.h"
#include "goo/GooString.h"


#ifdef CAIRO_HAS_WIN32_SURFACE

#include <cairo-win32.h>

void win32GetFitToPageTransform(cairo_matrix_t *m);
cairo_surface_t *win32BeginDocument(GooString *inputFileName, GooString *outputFileName,
				    double w, double h,
				    GooString *printer,
				    GooString *printOpt,
				    GBool duplex);
void win32BeginPage(double w, double h);
void win32EndPage(GooString *imageFileName);
void win32EndDocument();

#endif // CAIRO_HAS_WIN32_SURFACE
