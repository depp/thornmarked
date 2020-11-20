// Copyright 2014-2020 Dietrich Epp.
#pragma once

// Floating-point vector math functions.

#include "base/vectypes.h"

#include <stdbool.h>

// =============================================================================
// vec2: 2D vector
// =============================================================================

// Test if two vectors are equal.
bool vec2_eq(vec2 x, vec2 y);
// Add two vectors.
vec2 vec2_add(vec2 x, vec2 y);
// Subtract a vector from another vector.
vec2 vec2_sub(vec2 x, vec2 y);
// Negate a vector.
vec2 vec2_neg(vec2 x);
// Mustiply two vectors componentwise.
vec2 vec2_mul(vec2 x, vec2 y);
// Multiply a vector by a scalar.
vec2 vec2_scale(vec2 x, float a);
// Get the componentwise minimum of two vectors.
vec2 vec2_min(vec2 x, vec2 y);
// Get the componentwise maximum of two vectors.
vec2 vec2_max(vec2 x, vec2 y);
// Clamp the components in a vector to the given range.
vec2 vec2_clamp(vec2 x, float min_val, float max_val);
// Clamp the components in a vector to the given range.
vec2 vec2_clampv(vec2 x, vec2 min_val, vec2 max_val);
// Get the length of a vector.
float vec2_length(vec2 x);
// Get the squared length of a vector.
float vec2_length2(vec2 x);
// Get the distance between two vectors.
float vec2_distance(vec2 x, vec2 y);
// Get the squared distance between two vectors.
float vec2_distance2(vec2 x, vec2 y);
// Get the dot product of two vectors.
float vec2_dot(vec2 x, vec2 y);
// Normalize a vector.
vec2 vec2_normalize(vec2 x);
// Interpolate between two vectors.
vec2 vec2_mix(vec2 x, vec2 y, float a);
