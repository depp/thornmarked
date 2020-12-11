// Copyright 2014-2020 Dietrich Epp.
#pragma once

// Floating-point vector math functions.

#include "base/float.h"
#include "base/vectypes.h"

#include <math.h>
#include <stdbool.h>

// Convert a vec2 to a vec3.
inline vec3 vec3_vec2(vec2 v, float z) {
    return (vec3){{v.v[0], v.v[1], z}};
}

// Test if two vectors are equal.
inline bool vec3_eq(vec3 x, vec3 y) {
    return x.v[0] == y.v[0] && x.v[1] == y.v[1] && x.v[2] == y.v[2];
}

// Add two vectors.
inline vec3 vec3_add(vec3 x, vec3 y) {
    return (vec3){{x.v[0] + y.v[0], x.v[1] + y.v[1], x.v[2] + y.v[2]}};
}

// Subtract a vector from another vector.
inline vec3 vec3_sub(vec3 x, vec3 y) {
    return (vec3){{x.v[0] - y.v[0], x.v[1] - y.v[1], x.v[2] - y.v[2]}};
}

// Negate a vector.
inline vec3 vec3_neg(vec3 x) {
    return (vec3){{-x.v[0], -x.v[1], -x.v[2]}};
}

// Mustiply two vectors componentwise.
inline vec3 vec3_mul(vec3 x, vec3 y) {
    return (vec3){{x.v[0] * y.v[0], x.v[1] * y.v[1], x.v[2] * y.v[2]}};
}

// Multiply a vector by a scalar.
inline vec3 vec3_scale(vec3 x, float a) {
    return (vec3){{x.v[0] * a, x.v[1] * a, x.v[2] * a}};
}

// Get the componentwise minimum of two vectors.
inline vec3 vec3_min(vec3 x, vec3 y) {
    return (vec3){{
        x.v[0] < y.v[0] ? x.v[0] : y.v[0],
        x.v[1] < y.v[1] ? x.v[1] : y.v[1],
        x.v[2] < y.v[2] ? x.v[2] : y.v[2],
    }};
}

// Get the componentwise maximum of two vectors.
inline vec3 vec3_max(vec3 x, vec3 y) {
    return (vec3){{
        x.v[0] > y.v[0] ? x.v[0] : y.v[0],
        x.v[1] > y.v[1] ? x.v[1] : y.v[1],
        x.v[2] > y.v[2] ? x.v[2] : y.v[2],
    }};
}

// Clamp the components in a vector to the given range.
inline vec3 vec3_clamp(vec3 x, float min_val, float max_val) {
    return (vec3){{
        fclampf(x.v[0], min_val, max_val),
        fclampf(x.v[1], min_val, max_val),
        fclampf(x.v[2], min_val, max_val),
    }};
}

// Clamp the components in a vector to the given range.
inline vec3 vec3_clampv(vec3 x, vec3 min_val, vec3 max_val) {
    return (vec3){{
        fclampf(x.v[0], min_val.v[0], max_val.v[0]),
        fclampf(x.v[1], min_val.v[1], max_val.v[1]),
        fclampf(x.v[2], min_val.v[2], max_val.v[2]),
    }};
}

// Get the squared length of a vector.
inline float vec3_length2(vec3 x) {
    return x.v[0] * x.v[0] + x.v[1] * x.v[1] + x.v[2] * x.v[2];
}

// Get the length of a vector.
inline float vec3_length(vec3 x) {
    return sqrtf(vec3_length2(x));
}

// Get the squared distance between two vectors.
inline float vec3_distance2(vec3 x, vec3 y) {
    return vec3_length2(vec3_sub(x, y));
}

// Get the distance between two vectors.
inline float vec3_distance(vec3 x, vec3 y) {
    return sqrtf(vec3_distance2(x, y));
}

// Get the dot product of two vectors.
inline float vec3_dot(vec3 x, vec3 y) {
    return x.v[0] * y.v[0] + x.v[1] * y.v[1] + x.v[2] * y.v[2];
}

// Normalize a vector.
inline vec3 vec3_normalize(vec3 x) {
    return vec3_scale(x, 1.0f / vec3_length(x));
}

// Interpolate between two vectors.
inline vec3 vec3_mix(vec3 x, vec3 y, float a) {
    return (vec3){{
        x.v[0] + a * (y.v[0] - x.v[0]),
        x.v[1] + a * (y.v[1] - x.v[1]),
        x.v[2] + a * (y.v[2] - x.v[2]),
    }};
}

// Compute x + a * y.
inline vec3 vec3_madd(vec3 x, vec3 y, float a) {
    return (vec3){{
        x.v[0] + a * y.v[0],
        x.v[1] + a * y.v[1],
        x.v[2] + a * y.v[2],
    }};
}

// Compete the cross product of x and y.
vec3 vec3_cross(vec3 x, vec3 y);
