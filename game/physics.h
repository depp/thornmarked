#pragma once

#include "base/vectypes.h"

// Physical object component. Used for entities with a physical presence in the
// game world.
struct cp_phys {
    vec2 pos;
    vec2 vel;
};

// Physics system.
struct sys_phys {
    struct cp_phys *entities;
    unsigned count;
};

// Initialize physics system.
void physics_init(struct sys_phys *restrict psys);

// Create new physics component.
struct cp_phys *physics_new(struct sys_phys *restrict psys);

// Update physics.
void physics_update(struct sys_phys *restrict psys, float dt);
