#include <stdint.h>
#include <poppler.h>
#include <cairo.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    gsize length;
    poppler_named_dest_from_bytestring(data, size);
    poppler_named_dest_to_bytestring(data, &length);
    /*poppler_named_dest_from_bytestring((const guint8*)data, size);*/
    /*poppler_named_dest_to_bytestring((const char*)data, &length);*/
    return 0;
}
