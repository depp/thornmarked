#pragma once

#include "base/vectypes.h"
#include "game/core/entity.h"

struct sys_phys;

// Walker component. Used for entities that can walk around.
struct cp_walk {
    ent_id ent;
    vec2 drive;
    float face_angle;
};

struct sys_walk {
    struct cp_walk *components;
    unsigned short *entities;
    int count;
};

// Initailize walkers.
void walk_init(struct sys_walk *restrict wsys);

// Get the walker component for the given entity.
inline struct cp_walk *walk_get(struct sys_walk *restrict wsys, ent_id ent) {
    int index = wsys->entities[ent.id];
    struct cp_walk *wp = &wsys->components[index];
    return wp->ent.id == ent.id && index < wsys->count ? wp : 0;
}

// Create new walker.
struct cp_walk *walk_new(struct sys_walk *restrict wsys, ent_id ent);

// Update walkers.
void walk_update(struct sys_walk *restrict wsys, struct sys_phys *restrict psys,
                 float dt);
