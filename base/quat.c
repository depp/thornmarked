#include "base/quat.h"

#include <math.h>

quat quat_identity(void);

quat quat_axis_angle(vec3 axis, float angle) {
    const float half = 0.5f * angle;
    const float a = 1.0f / vec3_length(axis);
    const float b = cosf(half);
    const float c = a * sinf(half);
    return (quat){{b, c * axis.v[0], c * axis.v[1], c * axis.v[2]}};
}

quat quat_angles(vec3 angles) {
    const quat rx = {{
        cosf(0.5f * angles.v[0]),
        sinf(0.5f * angles.v[0]),
        0.0f,
        0.0f,
    }};
    const quat ry = {{
        cosf(0.5f * angles.v[1]),
        0.0f,
        sinf(0.5f * angles.v[1]),
        0.0f,
    }};
    const quat rz = {{
        cosf(0.5f * angles.v[2]),
        0.0f,
        0.0f,
        sinf(0.5f * angles.v[2]),
    }};
    return quat_mul(quat_mul(rx, ry), rz);
}

quat quat_rotate_x(float angle) {
    return (quat){{
        cosf(0.5f * angle),
        sinf(0.5f * angle),
        0.0f,
        0.0f,
    }};
}

quat quat_rotate_y(float angle) {
    return (quat){{
        cosf(0.5f * angle),
        0.0f,
        sinf(0.5f * angle),
        0.0f,
    }};
}

quat quat_rotate_z(float angle) {
    return (quat){{
        cosf(0.5f * angle),
        0.0f,
        0.0f,
        sinf(0.5f * angle),
    }};
}

quat quat_mul(quat x, quat y) {
    const float *xf = x.v, *yf = y.v;
    return (quat){{
        xf[0] * yf[0] - xf[1] * yf[1] - xf[2] * yf[2] - xf[3] * yf[3],
        xf[0] * yf[1] + xf[1] * yf[0] + xf[2] * yf[3] - xf[3] * yf[2],
        xf[0] * yf[2] + xf[2] * yf[0] + xf[3] * yf[1] - xf[1] * yf[3],
        xf[0] * yf[3] + xf[3] * yf[0] + xf[1] * yf[2] - xf[2] * yf[1],
    }};
}

vec3 quat_transform(quat q, vec3 v) {
    const float w = q.v[0], x = q.v[1], y = q.v[2], z = q.v[3];
    return (vec3){{
        v.v[0] * (1 - 2 * y * y - 2 * z * z) +
            v.v[1] * (2 * x * y - 2 * w * z) + v.v[2] * (2 * z * x + 2 * w * y),
        v.v[0] * (2 * x * y + 2 * w * z) +
            v.v[1] * (1 - 2 * z * z - 2 * x * x) +
            v.v[2] * (2 * y * z - 2 * w * x),
        v.v[0] * (2 * z * x - 2 * w * y) + v.v[1] * (2 * y * z + 2 * w * x) +
            v.v[2] * (1 - 2 * x * x - 2 * y * y),
    }};
}
