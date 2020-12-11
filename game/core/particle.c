#include "game/core/particle.h"

#include "base/base.h"

// Maximum number of particles.
enum {
    MAX_PARTICLE_COUNT = 32,
};

void particle_init(struct sys_particle *restrict psys) {
    *psys = (struct sys_particle){
        .particle = mem_alloc(sizeof(*psys->particle) * MAX_PARTICLE_COUNT),
    };
}

void particle_create(struct sys_particle *restrict psys, vec3 pos) {
    // Drop the particle if there are too many.
    if (psys->count >= MAX_PARTICLE_COUNT) {
        return;
    }
    int index = psys->count++;
    struct particle *restrict pp = &psys->particle[index];
    *pp = (struct particle){
        .pos = pos,
    };
}
