#ifndef CAIRO_RESCALE_BOX_H
#define CAIRO_RESCALE_BOX_H

#include "goo/gtypes.h"
#include <cairo.h>

class CairoRescaleBox {
public:

  CairoRescaleBox() {};
  virtual ~CairoRescaleBox() {};

  virtual GBool downScaleImage(unsigned orig_width, unsigned orig_height,
                               signed scaled_width, signed scaled_height,
                               unsigned short int start_column, unsigned short int start_row,
                               unsigned short int width, unsigned short int height,
                               cairo_surface_t *dest_surface);

  virtual void getRow(int row_num, uint32_t *row_data) = 0;

};

#endif /* CAIRO_RESCALE_BOX_H */
