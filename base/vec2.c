// Copyright 2014-2020 Dietrich Epp.
#include "base/vec2.h"

#include "base/float.h"

#include <math.h>

bool vec2_eq(vec2 x, vec2 y) {
    return x.v[0] == y.v[0] && x.v[1] == y.v[1];
}

vec2 vec2_add(vec2 x, vec2 y) {
    vec2 r;
    r.v[0] = x.v[0] + y.v[0];
    r.v[1] = x.v[1] + y.v[1];
    return r;
}

vec2 vec2_sub(vec2 x, vec2 y) {
    vec2 r;
    r.v[0] = x.v[0] - y.v[0];
    r.v[1] = x.v[1] - y.v[1];
    return r;
}

vec2 vec2_neg(vec2 x) {
    vec2 r;
    r.v[0] = -x.v[0];
    r.v[1] = -x.v[1];
    return r;
}

vec2 vec2_mul(vec2 x, vec2 y) {
    vec2 r;
    r.v[0] = x.v[0] * y.v[0];
    r.v[1] = x.v[1] * y.v[1];
    return r;
}

vec2 vec2_scale(vec2 x, float a) {
    vec2 r;
    r.v[0] = x.v[0] * a;
    r.v[1] = x.v[1] * a;
    return r;
}

vec2 vec2_min(vec2 x, vec2 y) {
    vec2 r;
    r.v[0] = fminf(x.v[0], y.v[0]);
    r.v[1] = fminf(x.v[1], y.v[1]);
    return r;
}

vec2 vec2_max(vec2 x, vec2 y) {
    vec2 r;
    r.v[0] = fmaxf(x.v[0], y.v[0]);
    r.v[1] = fmaxf(x.v[1], y.v[1]);
    return r;
}

vec2 vec2_clamp(vec2 x, float min_val, float max_val) {
    vec2 r;
    r.v[0] = fclampf(x.v[0], min_val, max_val);
    r.v[1] = fclampf(x.v[1], min_val, max_val);
    return r;
}

vec2 vec2_clampv(vec2 x, vec2 min_val, vec2 max_val) {
    vec2 r;
    r.v[0] = fclampf(x.v[0], min_val.v[0], max_val.v[0]);
    r.v[1] = fclampf(x.v[1], min_val.v[1], max_val.v[1]);
    return r;
}

float vec2_length(vec2 x) {
    return sqrtf(vec2_length2(x));
}

float vec2_length2(vec2 x) {
    return x.v[0] * x.v[0] + x.v[1] * x.v[1];
}

float vec2_distance(vec2 x, vec2 y) {
    return sqrtf(vec2_distance2(x, y));
}

float vec2_distance2(vec2 x, vec2 y) {
    return vec2_length2(vec2_sub(x, y));
}

float vec2_dot(vec2 x, vec2 y) {
    return x.v[0] * y.v[0] + x.v[1] * y.v[1];
}

vec2 vec2_normalize(vec2 x) {
    return vec2_scale(x, 1.0f / vec2_length(x));
}

vec2 vec2_mix(vec2 x, vec2 y, float a) {
    vec2 r;
    r.v[0] = x.v[0] + a * (y.v[0] - x.v[0]);
    r.v[1] = x.v[1] + a * (y.v[1] - x.v[1]);
    return r;
}
