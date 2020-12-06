#pragma once

#include "base/vectypes.h"
#include "game/core/entity.h"

#include <stdbool.h>

// Physical object component. Used for entities with a physical presence in the
// game world.
struct cp_phys {
    ent_id ent;
    vec2 pos;         // Position: updated by physics.
    vec2 vel;         // Velocity: must be set as an input.
    float max_vel;    // Maximum velocity after collision response.
    quat orientation; // Orientation: must be set as an input.
    float radius;     // Radius for collisions.

    // Private.
    vec2 adj;      // Collision adjustment to position.
    bool collided; // Has collision adjustment.
    bool stable;   // True if component has stable position.
};

// Physics system.
struct sys_phys {
    struct cp_phys *physics;
    unsigned short *entities;
    int count;
};

// Initialize physics system.
void physics_init(struct sys_phys *restrict psys);

// Get the physics component for the given entity.
inline struct cp_phys *physics_get(struct sys_phys *restrict psys, ent_id ent) {
    int index = psys->entities[ent.id];
    struct cp_phys *pp = &psys->physics[index];
    return pp->ent.id == ent.id && index < psys->count ? pp : 0;
}

// Create new physics component. Overwrite any existing physics component.
struct cp_phys *physics_new(struct sys_phys *restrict psys, ent_id ent);

// Update physics.
void physics_update(struct sys_phys *restrict psys, float dt);
