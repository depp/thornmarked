#pragma once

#include "base/vectypes.h"

struct sys_phys;

// Walker component. Used for entities that can walk around.
struct cp_walk {
    vec2 drive;
    float face_angle;
};

struct sys_walk {
    struct cp_walk *entities;
    unsigned count;
};

// Initailize walkers.
void walk_init(struct sys_walk *restrict wsys);

// Create new walker.
struct cp_walk *walk_new(struct sys_walk *restrict wsys);

// Update walkers.
void walk_update(struct sys_walk *restrict wsys, struct sys_phys *restrict psys,
                 float dt);
