// Copyright 2014-2020 Dietrich Epp.
#include "base/vec2.h"

bool vec2_eq(vec2 x, vec2 y);
vec2 vec2_add(vec2 x, vec2 y);
vec2 vec2_sub(vec2 x, vec2 y);
vec2 vec2_neg(vec2 x);
vec2 vec2_mul(vec2 x, vec2 y);
vec2 vec2_scale(vec2 x, float a);
vec2 vec2_min(vec2 x, vec2 y);
vec2 vec2_max(vec2 x, vec2 y);
vec2 vec2_clamp(vec2 x, float min_val, float max_val);
vec2 vec2_clampv(vec2 x, vec2 min_val, vec2 max_val);
float vec2_length2(vec2 x);
float vec2_length(vec2 x);
float vec2_distance2(vec2 x, vec2 y);
float vec2_distance(vec2 x, vec2 y);
float vec2_dot(vec2 x, vec2 y);
vec2 vec2_normalize(vec2 x);
vec2 vec2_mix(vec2 x, vec2 y, float a);
vec2 vec2_madd(vec2 x, vec2 y, float a);
