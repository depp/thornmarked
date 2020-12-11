// Copyright 2014-2020 Dietrich Epp.
#include "base/vec3.h"

vec3 vec3_vec2(vec2 v, float z);
bool vec3_eq(vec3 x, vec3 y);
vec3 vec3_add(vec3 x, vec3 y);
vec3 vec3_sub(vec3 x, vec3 y);
vec3 vec3_neg(vec3 x);
vec3 vec3_mul(vec3 x, vec3 y);
vec3 vec3_scale(vec3 x, float a);
vec3 vec3_min(vec3 x, vec3 y);
vec3 vec3_max(vec3 x, vec3 y);
vec3 vec3_clamp(vec3 x, float min_val, float max_val);
vec3 vec3_clampv(vec3 x, vec3 min_val, vec3 max_val);
float vec3_length2(vec3 x);
float vec3_length(vec3 x);
float vec3_distance2(vec3 x, vec3 y);
float vec3_distance(vec3 x, vec3 y);
float vec3_dot(vec3 x, vec3 y);
vec3 vec3_normalize(vec3 x);
vec3 vec3_mix(vec3 x, vec3 y, float a);
vec3 vec3_madd(vec3 x, vec3 y, float a);

vec3 vec3_cross(vec3 x, vec3 y) {
    return (vec3){{
        x.v[1] * y.v[2] - x.v[2] * y.v[1],
        x.v[2] * y.v[0] - x.v[0] * y.v[2],
        x.v[0] * y.v[1] - x.v[1] * y.v[0],
    }};
}
