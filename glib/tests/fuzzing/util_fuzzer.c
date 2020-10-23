#include <stdint.h>
#include <poppler.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    gsize length;
    poppler_named_dest_from_bytestring(data, size);
    poppler_named_dest_to_bytestring(data, &length);
    return 0;
}
