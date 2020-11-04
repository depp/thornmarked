#include "corelib.h"

extern void *memset(void *s, int c, size_t n) {
    unsigned char *restrict ptr = s;
    for (size_t i = 0; i < n; i++) {
        ptr[i] = c;
    }
    return ptr;
}
