#pragma once

#include "game/core/particle.h"

#include <ultra64.h>

struct graphics;
struct sys_camera;
struct sys_particle;

// Initialize model rendering.
void particle_render_init(void);

// Render all particles.
Gfx *particle_render(Gfx *dl, struct graphics *restrict gr,
                     struct sys_particle *restrict psys,
                     struct sys_camera *restrict csys);
