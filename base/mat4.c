#include "base/mat4.h"

// Set the top 3x3 submatrix to contain the given rotation and scale.
static void mat4_set_rotate_scale(mat4 *restrict out, quat rotation,
                                  float scale) {
    const float w = rotation.v[0];
    const float x = rotation.v[1];
    const float y = rotation.v[2];
    const float z = rotation.v[3];
    // This has 9 multiplies for the rotation and 3 additional multiplies for
    // scaling.
    float xw, xx, xy, xz, yw, yy, yz, zw, zz;
    if (scale == 1.0f) {
        // Fast path for no scaling.
        xw = xx = xy = xz = x;
        yw = yy = yz = y;
        zw = zz = z;
    } else {
        xw = xx = xy = xz = scale * x;
        yw = yy = yz = scale * y;
        zw = zz = scale * z;
    }
    xw *= w;
    xx *= x;
    xy *= y;
    xz *= z;
    yw *= w;
    yy *= y;
    yz *= z;
    zw *= w;
    zz *= z;
    out->v[0] = scale - 2.0f * (yy + zz);
    out->v[1] = 2.0f * (xy + zw);
    out->v[2] = 2.0f * (xz - yw);
    out->v[4] = 2.0f * (xy - zw);
    out->v[5] = scale - 2.0f * (zz + xx);
    out->v[6] = 2.0f * (yz + xw);
    out->v[8] = 2.0f * (xz + yw);
    out->v[9] = 2.0f * (yz - xw);
    out->v[10] = scale - 2.0f * (xx + yy);
}

void mat4_translate_rotate_scale(mat4 *restrict out, vec3 translation,
                                 quat rotation, float scale) {
    mat4_set_rotate_scale(out, rotation, scale);
    out->v[3] = 0.0f;
    out->v[7] = 0.0f;
    out->v[11] = 0.0f;
    out->v[12] = translation.v[0];
    out->v[13] = translation.v[1];
    out->v[14] = translation.v[2];
    out->v[15] = 1.0f;
}

void mat4_perspective(mat4 *restrict out, float focalx, float focaly,
                      float near, float far, float scale) {
    *out = (mat4){{
        [0] = scale * focalx,
        [5] = scale * focaly,
        [10] = scale * (near + far) / (near - far),
        [11] = -scale,
        [14] = scale * 2.0f * near * far / (near - far),
    }};
}
