#pragma once

// Integer vector math functions.

#include "base/vectypes.h"

// Convert a vec3 to an ivec3
inline ivec3 ivec3_vec3(vec3 v) {
    return (ivec3){{(int)v.v[0], (int)v.v[1], (int)v.v[2]}};
}

// Add two vectors.
inline ivec3 ivec3_add(ivec3 x, ivec3 y) {
    return (ivec3){{x.v[0] + y.v[0], x.v[1] + y.v[1], x.v[2] + y.v[2]}};
}

// Subtract a vector from another vector.
inline ivec3 ivec3_sub(ivec3 x, ivec3 y) {
    return (ivec3){{x.v[0] - y.v[0], x.v[1] - y.v[1], x.v[2] - y.v[2]}};
}

// Negate a vector.
inline ivec3 ivec3_neg(ivec3 x) {
    return (ivec3){{-x.v[0], -x.v[1], -x.v[2]}};
}
