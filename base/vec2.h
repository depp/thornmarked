// Copyright 2014-2020 Dietrich Epp.
#pragma once

// Floating-point vector math functions.

#include "base/float.h"
#include "base/vectypes.h"

#include <math.h>
#include <stdbool.h>

// =============================================================================
// vec2: 2D vector
// =============================================================================

// Convert a vec3 to a vec2, discarding the Z coordinate.
inline vec2 vec2_vec3(vec3 v) {
    return (vec2){{v.v[0], v.v[1]}};
}

// Test if two vectors are equal.
inline bool vec2_eq(vec2 x, vec2 y) {
    return x.v[0] == y.v[0] && x.v[1] == y.v[1];
}

// Add two vectors.
inline vec2 vec2_add(vec2 x, vec2 y) {
    return (vec2){{x.v[0] + y.v[0], x.v[1] + y.v[1]}};
}

// Subtract a vector from another vector.
inline vec2 vec2_sub(vec2 x, vec2 y) {
    return (vec2){{x.v[0] - y.v[0], x.v[1] - y.v[1]}};
}

// Negate a vector.
inline vec2 vec2_neg(vec2 x) {
    return (vec2){{-x.v[0], -x.v[1]}};
}

// Mustiply two vectors componentwise.
inline vec2 vec2_mul(vec2 x, vec2 y) {
    return (vec2){{x.v[0] * y.v[0], x.v[1] * y.v[1]}};
}

// Multiply a vector by a scalar.
inline vec2 vec2_scale(vec2 x, float a) {
    return (vec2){{x.v[0] * a, x.v[1] * a}};
}

// Get the componentwise minimum of two vectors.
inline vec2 vec2_min(vec2 x, vec2 y) {
    return (vec2){{
        x.v[0] < y.v[0] ? x.v[0] : y.v[0],
        x.v[1] < y.v[1] ? x.v[1] : y.v[1],
    }};
}

// Get the componentwise maximum of two vectors.
inline vec2 vec2_max(vec2 x, vec2 y) {
    return (vec2){{
        x.v[0] > y.v[0] ? x.v[0] : y.v[0],
        x.v[1] > y.v[1] ? x.v[1] : y.v[1],
    }};
}

// Clamp the components in a vector to the given range.
inline vec2 vec2_clamp(vec2 x, float min_val, float max_val) {
    return (vec2){{
        fclampf(x.v[0], min_val, max_val),
        fclampf(x.v[1], min_val, max_val),
    }};
}

// Clamp the components in a vector to the given range.
inline vec2 vec2_clampv(vec2 x, vec2 min_val, vec2 max_val) {
    return (vec2){{
        fclampf(x.v[0], min_val.v[0], max_val.v[0]),
        fclampf(x.v[1], min_val.v[1], max_val.v[1]),
    }};
}

// Get the squared length of a vector.
inline float vec2_length2(vec2 x) {
    return x.v[0] * x.v[0] + x.v[1] * x.v[1];
}

// Get the length of a vector.
inline float vec2_length(vec2 x) {
    return sqrtf(vec2_length2(x));
}

// Get the squared distance between two vectors.
inline float vec2_distance2(vec2 x, vec2 y) {
    return vec2_length2(vec2_sub(x, y));
}

// Get the distance between two vectors.
inline float vec2_distance(vec2 x, vec2 y) {
    return sqrtf(vec2_distance2(x, y));
}

// Get the dot product of two vectors.
inline float vec2_dot(vec2 x, vec2 y) {
    return x.v[0] * y.v[0] + x.v[1] * y.v[1];
}

// Normalize a vector.
inline vec2 vec2_normalize(vec2 x) {
    return vec2_scale(x, 1.0f / vec2_length(x));
}

// Interpolate between two vectors.
inline vec2 vec2_mix(vec2 x, vec2 y, float a) {
    return (vec2){{
        x.v[0] + a * (y.v[0] - x.v[0]),
        x.v[1] + a * (y.v[1] - x.v[1]),
    }};
}

// Compute x + a * y.
inline vec2 vec2_madd(vec2 x, vec2 y, float a) {
    return (vec2){{
        x.v[0] + a * y.v[0],
        x.v[1] + a * y.v[1],
    }};
}
