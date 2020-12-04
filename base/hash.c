#include "base/hash.h"

// Murmur 3 hash.

static uint32_t hash_update(uint32_t state, uint32_t data) {
    uint32_t k = data;
    k *= 0xcc9e2d51u;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593u;
    state = state ^ k;
    state = (state << 13) | (state >> 19);
    state = (state * 5) + 0xe6546b64u;
    return state;
}

static uint32_t hash_finalize(uint32_t state, uint32_t len) {
    uint32_t hash = state;
    hash ^= len;
    hash ^= hash >> 16;
    hash *= 0x85ebca6bu;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35u;
    hash ^= hash >> 16;
    return hash;
}

uint32_t hash32(uint32_t x) {
    return hash_finalize(hash_update(0, x), 4);
}
