#pragma once

#include "base/vectypes.h"

// Camera system.
struct sys_camera {
    vec3 pos;
    vec3 look_at;

    // Camera focal length, 1.0 = 90 degree FOV.
    float focal;

    // Unit vectors describing the camera's point of view. Calculated by the
    // camera system.
    vec3 right;
    vec3 up;
    vec3 forward;
};

// Initialize the camera system.
void camera_init(struct sys_camera *restrict csys);

// Update the camera system.
void camera_update(struct sys_camera *restrict csys);
