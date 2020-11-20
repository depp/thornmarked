// Copyright 2014-2020 Dietrich Epp.
#pragma once

// Floating-point math functions.

// Note: The naming convention matches fminf() and fmaxf() which are provided by
// <math.h>.

// Clamp a scalar to the given range.
inline float fclampf(float x, float min_val, float max_val) {
    if (x < min_val)
        return min_val;
    if (x > max_val)
        return max_val;
    return x;
}

// Interpolate between two scalars.
inline float fmixf(float x, float y, float a) {
    return x + a * (y - x);
}
