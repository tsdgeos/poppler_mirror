#include <stdint.h>
#include <poppler.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *tmp_ch;
    char *buf;
    gsize length;
    guint8 *tmp_uint;

    buf = (char *)malloc(size + 1);
    memcpy(buf, data, size);
    buf[size] = '\0';

    tmp_ch = poppler_named_dest_from_bytestring(buf, size);
    tmp_uint = poppler_named_dest_to_bytestring(buf, &length);

    g_free(tmp_ch);
    g_free(tmp_uint);
    free(buf);
    return 0;
}
