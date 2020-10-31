#pragma once
#include <stdint.h>

// Random number generator state.
struct rand {
    uint32_t state;
    uint32_t inc;
};

// Initialize a random number generator.
void rand_init(struct rand *restrict r, uint32_t state, uint32_t seq);

// Return the next random number in the sequence. The result is a 32-bit
// integer.
uint32_t rand_next(struct rand *restrict r);

// Generate a random number within the given range, inclusive. Some values
// may be more likely than others.
int rand_range_fast(struct rand *restrict r, int min, int max);

// Generate a random floating-point number in the half-open range [0,1).
float rand_fnext(struct rand *restrict r);

// Generate a random floating-point number within the given half-open range.
float rand_frange(struct rand *restrict r, float min, float max);
