#include "game/core/physics.h"

#include "base/base.h"
#include "base/quat.h"
#include "base/vec2.h"

enum {
    // Maximum number of physics objects.
    MAX_PHYSICS_OBJS = 64,
};

void physics_init(struct sys_phys *restrict psys) {
    *psys = (struct sys_phys){
        .physics = mem_alloc(sizeof(*psys->physics) * MAX_PHYSICS_OBJS),
        .entities = mem_calloc(sizeof(*psys->entities) * ENTITY_COUNT),
    };
}

struct cp_phys *physics_get(struct sys_phys *restrict psys, ent_id ent);

struct cp_phys *physics_new(struct sys_phys *restrict psys, ent_id ent) {
    int index = psys->entities[ent.id];
    struct cp_phys *pp = &psys->physics[index];
    if (pp->ent.id != ent.id || index >= psys->count) {
        index = psys->count;
        if (index > MAX_PHYSICS_OBJS) {
            fatal_error("Too many physics objects");
        }
        psys->count = index + 1;
        pp = &psys->physics[index];
        psys->entities[ent.id] = index;
    }
    *pp = (struct cp_phys){
        .ent = ent,
        .orientation = quat_identity(),
    };
    return pp;
}

void physics_update(struct sys_phys *restrict psys, float dt) {
    struct cp_phys *restrict entities = psys->physics;
    for (struct cp_phys *cp = entities, *ce = cp + psys->count; cp != ce;
         cp++) {
        cp->pos = vec2_madd(cp->pos, cp->vel, dt);
        for (int i = 0; i < 2; i++) {
            const float wall = 2.0f;
            if (cp->pos.v[i] > wall) {
                cp->pos.v[i] = wall;
                cp->vel.v[i] = 0.0f;
            } else if (cp->pos.v[i] < -wall) {
                cp->pos.v[i] = -wall;
                cp->vel.v[i] = 0.0f;
            }
        }
    }
}
