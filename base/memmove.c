#include "base/base.h"

#pragma GCC diagnostic ignored "-Wsign-compare"

void *memmove(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *dptr = dest;
    const unsigned char *sptr = src;
    if (dptr - sptr >= n) {
        for (size_t i = 0; i < n; i++) {
            dptr[i] = sptr[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            dptr[i - 1] = sptr[i - 1];
        }
    }
    return dest;
}
