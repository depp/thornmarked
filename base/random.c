// Random number generator.
//
// Uses the PCG algorithm: https://www.pcg-random.org/index.html
#include "base/random.h"

static void rand_advance(struct rand *restrict r) {
    r->state = r->state * 747796405u + r->inc;
}

void rand_init(struct rand *restrict r, uint32_t state, uint32_t seq) {
    r->state = 0;
    r->inc = (seq << 1) | 1;
    rand_advance(r);
    r->state += state;
    rand_advance(r);
}

uint32_t rand_next(struct rand *restrict r) {
    uint32_t x = r->state;
    rand_advance(r);
    x = ((x >> ((x >> 28) + 4)) ^ x) * 277803737u;
    return (x >> 22) ^ x;
}

int rand_range_fast(struct rand *restrict r, int min, int max) {
    if (max <= min) {
        return min;
    }
    uint32_t range = (max - min) + 1;
    uint32_t val = rand_next(r);
    uint32_t off = ((val >> 16) * range) >> 16;
    return min + off;
}
