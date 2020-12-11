#pragma once

#include "base/vectypes.h"

// Maximum number of particles.
enum {
    MAX_PARTICLE_COUNT = 32,
};

// A simple particle. This is not a component and cannot be attached to an
// entity.
struct particle {
    vec3 pos;
    float size;
    color color;
};

// Particle system.
struct sys_particle {
    struct particle *particle;
    int count;
};

// Initialize the particle system.
void particle_init(struct sys_particle *restrict psys);

// Create a new particle at the given position.
void particle_create(struct sys_particle *restrict psys, vec3 pos, float size,
                     color pcolor);

// Update the particle system.
void particle_update(struct sys_particle *restrict psys, float dt);
