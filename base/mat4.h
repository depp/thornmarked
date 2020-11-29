#pragma once

#include "base/vectypes.h"

// Create a matrix that scales, rotates, and then translates.
void mat4_translate_rotate_scale(mat4 *restrict out, vec3 translation,
                                 quat rotation, float scale);
