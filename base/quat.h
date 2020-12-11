// Copyright 2014-2020 Dietrich Epp.
#pragma once

// Quaternion math functions.

#include "base/vec3.h"
#include "base/vectypes.h"

// Get the identity quaternion.
inline quat quat_identity(void) {
    return (quat){{1.0f, 0.0f, 0.0f, 0.0f}};
}

// Create a quaternion for a rotation about the given axis. The axis does not
// need to be normalized.
quat quat_axis_angle(vec3 axis, float angle);

// Create a quaternion from roll, pitch, yaw vector.
quat quat_angles(vec3 angles);

// Create a quaternion to rotate about the X axis.
quat quat_rotate_x(float angle);

// Create a quaternion to rotate about the Y axis.
quat quat_rotate_y(float angle);

// Create a quaternion to rotate about the Z axis.
quat quat_rotate_z(float angle);

// Multiply two quaternions.
quat quat_mul(quat x, quat y);

// Transform a vector using a quaternion.
vec3 quat_transform(quat q, vec3 v);

// Get the vector (1,0,0) transformed by the quaternion.
vec3 quat_x(quat q);
