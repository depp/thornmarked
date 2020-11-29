#include "game/physics.h"

#include "base/base.h"
#include "base/quat.h"
#include "base/vec2.h"

enum {
    // Maximum number of physics objects.
    MAX_PHYSICS_OBJS = 64,
};

void physics_init(struct sys_phys *restrict psys) {
    psys->entities = mem_alloc(sizeof(*psys->entities) * MAX_PHYSICS_OBJS);
    psys->count = 0;
}

struct cp_phys *physics_new(struct sys_phys *restrict psys) {
    if (psys->count >= MAX_PHYSICS_OBJS) {
        fatal_error("Too many physics objects");
    }
    unsigned index = psys->count;
    psys->count++;
    struct cp_phys *pp = &psys->entities[index];
    pp->pos = (vec2){{0.0f, 0.0f}};
    pp->vel = (vec2){{0.0f, 0.0f}};
    pp->orientation = quat_identity();
    return pp;
}

void physics_update(struct sys_phys *restrict psys, float dt) {
    struct cp_phys *restrict entities = psys->entities;
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
