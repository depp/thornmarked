#include "game/core/particle.h"

#include "base/base.h"

void particle_init(struct sys_particle *restrict psys) {
    *psys = (struct sys_particle){
        .particle = mem_alloc(sizeof(*psys->particle) * MAX_PARTICLE_COUNT),
    };
}

void particle_create(struct sys_particle *restrict psys, vec3 pos, float size,
                     color pcolor) {
    // Drop the particle if there are too many.
    if (psys->count >= MAX_PARTICLE_COUNT) {
        return;
    }
    int index = psys->count++;
    struct particle *restrict pp = &psys->particle[index];
    *pp = (struct particle){
        .pos = pos,
        .size = size,
        .color = pcolor,
    };
}

void particle_update(struct sys_particle *restrict psys, float dt) {
    for (int i = 0; i < psys->count;) {
        struct particle *restrict pp = &psys->particle[i];
        pp->size -= dt;
        if (pp->size < 0.01f) {
            psys->count--;
            if (i >= psys->count) {
                break;
            }
            *pp = psys->particle[psys->count];
        } else {
            i++;
        }
    }
}
