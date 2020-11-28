#pragma once

#include "base/vectypes.h"

#include <ultra64.h>

struct graphics;

// Camera system.
struct sys_camera {
    vec3 pos;
    vec3 look_at;
};

// Initialize the camera system.
void camera_init(struct sys_camera *restrict csys);

// Update the camera system.
void camera_update(struct sys_camera *restrict csys);

// Set the projection matrix for rendering from the camera's perspective.
Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl);
