#include "game/walk.h"

#include "base/base.h"
#include "base/vec2.h"
#include "game/physics.h"

#include <math.h>

enum {
    // Maximum number of walking objects.
    MAX_WALK_OBJS = 16,
};

// Initailize walkers.
void walk_init(struct sys_walk *restrict wsys) {
    wsys->entities = mem_alloc(sizeof(*wsys->entities) * MAX_WALK_OBJS);
    wsys->count = 0;
}

// Create new walker.
struct cp_walk *walk_new(struct sys_walk *restrict wsys) {
    if (wsys->count >= MAX_WALK_OBJS) {
        fatal_error("Too many walk objects");
    }
    unsigned index = wsys->count;
    wsys->count++;
    return &wsys->entities[index];
}

// Update walkers.
void walk_update(struct sys_walk *restrict wsys, struct sys_phys *restrict psys,
                 float dt) {
    for (unsigned i = 0; i < wsys->count; i++) {
        struct cp_walk *wp = &wsys->entities[i];
        if (i >= psys->count) {
            continue;
        }
        struct cp_phys *pp = &psys->entities[i];
        float speed = 5.0f;
        float drive2 = vec2_length2(wp->drive);
        if (drive2 > 1.0f) {
            speed /= sqrtf(drive2);
        }
        vec2 target_vel = vec2_scale(wp->drive, speed);
        vec2 delta_vel = vec2_sub(target_vel, pp->vel);
        float dv2 = vec2_length2(delta_vel);
        const float accel_time = 0.05f;
        float accel = speed / accel_time;
        float max_dv = dt * accel;
        if (dv2 > max_dv * max_dv) {
            pp->vel = vec2_madd(pp->vel, delta_vel, max_dv / sqrtf(dv2));
        } else {
            pp->vel = target_vel;
        }
    }
}
