#include "base/base.h"

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *dptr = dest;
    const unsigned char *sptr = src;
    for (size_t i = 0; i < n; i++) {
        dptr[i] = sptr[i + 1];
    }
    return dest;
}
