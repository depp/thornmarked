#pragma once

#include "base/vectypes.h"

// Camera system.
struct sys_camera {
    vec3 pos;
    vec3 look_at;

    // Camera focal length, 1.0 = 90 degree FOV.
    float focal;
};

// Initialize the camera system.
void camera_init(struct sys_camera *restrict csys);

// Update the camera system.
void camera_update(struct sys_camera *restrict csys);
