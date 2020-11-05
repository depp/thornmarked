#include "base/base.h"

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = p1[i];
        unsigned char c2 = p2[i];
        if (c1 != c2) {
            return c1 - c2;
        }
    }
    return 0;
}
