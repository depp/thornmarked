#pragma once

#include "base/vectypes.h"

// Create a matrix that scales, rotates, and then translates.
void mat4_translate_rotate_scale(mat4 *restrict out, vec3 translation,
                                 quat rotation, float scale);

// Create a perspective projection matrix. The focal length for the X and Y
// directions is given, where a focal length of 1.0 is defined to have a 90
// degree field of view. The scale is multiplied into the entire matrix.
void mat4_perspective(mat4 *restrict out, float focalx, float focaly,
                      float near, float far, float scale);
