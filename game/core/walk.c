#include "game/core/walk.h"

#include "base/base.h"
#include "base/quat.h"
#include "base/vec2.h"
#include "game/core/physics.h"

#include <math.h>

enum {
    // Maximum number of walking objects.
    MAX_WALK_OBJS = 16,
};

// Initailize walkers.
void walk_init(struct sys_walk *restrict wsys) {
    *wsys = (struct sys_walk){
        .components = mem_alloc(sizeof(*wsys->components) * MAX_WALK_OBJS),
        .entities = mem_calloc(sizeof(*wsys->entities) * ENTITY_COUNT),
    };
}

struct cp_walk *walk_get(struct sys_walk *restrict wsys, ent_id ent);

struct cp_walk *walk_new(struct sys_walk *restrict wsys, ent_id ent) {
    int index = wsys->entities[ent.id];
    struct cp_walk *wp = &wsys->components[index];
    if (wp->ent.id != ent.id || index >= wsys->count) {
        index = wsys->count;
        if (index >= MAX_WALK_OBJS) {
            fatal_error("Too many walk objects");
        }
        wsys->count = index + 1;
        wp = &wsys->components[index];
        wsys->entities[ent.id] = index;
    }
    *wp = (struct cp_walk){
        .ent = ent,
        .face_angle = 0.0f,
    };
    return wp;
}

// Update walkers.
void walk_update(struct sys_walk *restrict wsys, struct sys_phys *restrict psys,
                 float dt) {
    for (int i = 0; i < wsys->count; i++) {
        struct cp_walk *wp = &wsys->components[i];
        struct cp_phys *pp = physics_get(psys, wp->ent);
        if (pp == NULL) {
            continue;
        }

        // Update velocity.
        float speed = 5.0f;
        float drive_mag = vec2_length(wp->drive);
        if (drive_mag > 1.0f) {
            speed /= drive_mag;
            drive_mag = 1.0f;
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

        // Update facing angle.
        if (drive_mag > 0.05f) {
            const float half_circle = 4.0f * atanf(1.0f);
            const float turn_speed = 3.0f * (2.0f * half_circle) * drive_mag;
            const float target_face = atan2f(wp->drive.v[1], wp->drive.v[0]);
            float delta_face = target_face - wp->face_angle;
            if (delta_face > half_circle) {
                delta_face -= 2.0f * half_circle;
            } else if (delta_face < -half_circle) {
                delta_face += 2.0f * half_circle;
            }
            const float max_dangle = turn_speed * dt;
            if (delta_face < -max_dangle) {
                wp->face_angle -= max_dangle;
            } else if (delta_face > max_dangle) {
                wp->face_angle += max_dangle;
            } else {
                wp->face_angle = target_face;
            }
        }

        // Update orientation.
        pp->orientation = quat_rotate_z(wp->face_angle);
    }
}
