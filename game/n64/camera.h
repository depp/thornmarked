#pragma once

#include "game/camera.h"

#include <ultra64.h>

struct sys_camera;
struct graphics;

// Set the projection matrix for rendering from the camera's perspective.
Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl);
