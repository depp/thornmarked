#pragma once

#include "game/core/entity.h"

struct sys_phys;
struct sys_walk;

// Monster behavior component.
struct cp_monster {
    ent_id ent;
    float timer;
};

// Monster behavior system.
struct sys_monster {
    struct cp_monster *monsters;
    unsigned short *entities;
    int count;
};

// Initialize monster system.
void monster_init(struct sys_monster *restrict msys);

// Get the monster component for an entity.
inline struct cp_monster *monster_get(struct sys_monster *restrict msys,
                                      ent_id ent) {
    int index = msys->entities[ent.id];
    struct cp_monster *p = &msys->monsters[index];
    return p->ent.id == ent.id && 0 < ent.id && index < msys->count ? p : 0;
}

// Create new monster component attached to the given entity. Overwrites any
// existing monster component for that entity.
struct cp_monster *monster_new(struct sys_monster *restrict msys, ent_id ent);

// Update monsters.
void monster_update(struct sys_monster *restrict msys,
                    struct sys_phys *restrict psys,
                    struct sys_walk *restrict wsys, float dt);
