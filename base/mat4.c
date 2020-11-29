#include "base/mat4.h"

// Set the top 3x3 submatrix to contain the given rotation and scale.
static void mat4_set_rotate_scale(mat4 *restrict out, quat rotation,
                                  float scale) {
    const float w = rotation.v[0];
    const float x = rotation.v[1];
    const float y = rotation.v[2];
    const float z = rotation.v[3];
    // This has 9 multiplies for the rotation and 4 additional multiplies for
    // scaling.
    float wx, wy, wz, xx, xy, xz, yy, yz, zz;
    if (scale == 1.0f) {
        // Fast path for no scaling.
        wx = wy = wz = w;
        xx = xy = xz = x;
        yy = yz = y;
        zz = z;
    } else {
        wx = wy = wz = scale * w;
        xx = xy = xz = scale * x;
        yy = yz = scale * y;
        zz = scale * z;
    }
    wx *= x;
    wy *= y;
    wz *= z;
    xx *= x;
    xy *= y;
    xz *= z;
    yy *= y;
    yz *= z;
    zz *= z;
    out->v[0] = scale - 2.0f * (yy + zz);
    out->v[1] = 2.0f * (xy + wz);
    out->v[2] = 2.0f * (xz - wy);
    out->v[4] = 2.0f * (xy - wz);
    out->v[5] = scale - 2.0f * (zz + xx);
    out->v[6] = 2.0f * (yz + wx);
    out->v[8] = 2.0f * (xz + wy);
    out->v[9] = 2.0f * (yz - wx);
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
