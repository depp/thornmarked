#include "game/core/physics.h"

#include "base/base.h"
#include "base/quat.h"
#include "base/vec2.h"
#include "game/core/random.h"

#include <math.h>

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

void physics_destroyall(struct sys_phys *restrict psys);

noreturn void physics_missing(ent_id ent) {
    fatal_error("Missing physics component\nEntity: %d", ent.id);
}

struct cp_phys *physics_get(struct sys_phys *restrict psys, ent_id ent);

struct cp_phys *physics_require(struct sys_phys *restrict psys, ent_id ent);

struct cp_phys *physics_find(struct sys_phys *restrict psys, ent_id self,
                             vec2 pos, float radius) {
    float best_distance = radius;
    struct cp_phys *best = NULL;
    struct cp_phys *restrict parr = psys->physics;
    for (int i = 0; i < psys->count; i++) {
        if (parr[i].ent.id == self.id) {
            continue;
        }
        const float r = best_distance + parr[i].radius;
        vec2 v = parr[i].pos;
        if ((v.v[0] < pos.v[0] - r || pos.v[0] + r < v.v[0]) ||
            (v.v[1] < pos.v[1] - r || pos.v[1] + r < v.v[1])) {
            continue;
        }
        float distance2 = vec2_distance2(pos, parr[i].pos);
        if (distance2 < r * r) {
            best_distance = sqrt(distance2);
            best = &parr[i];
        }
    }
    return best;
}

struct cp_phys *physics_new(struct sys_phys *restrict psys, ent_id ent) {
    int index = psys->entities[ent.id];
    struct cp_phys *pp = &psys->physics[index];
    if (pp->ent.id != ent.id || index >= psys->count) {
        index = psys->count;
        if (index >= MAX_PHYSICS_OBJS) {
            fatal_error("Too many physics objects");
        }
        psys->count = index + 1;
        pp = &psys->physics[index];
        psys->entities[ent.id] = index;
    }
    *pp = (struct cp_phys){
        .ent = ent,
        .orientation = quat_identity(),
        .max_vel = 10.0f,
    };
    return pp;
}

// Update a pair of objects.
static void physics_update_pair(struct cp_phys *restrict cx,
                                struct cp_phys *restrict cy) {
    float radius = cx->radius + cy->radius;
    vec2 dpos = vec2_sub(cx->pos, cy->pos);
    if ((dpos.v[0] < -radius || radius < dpos.v[0]) ||
        (dpos.v[1] < -radius || radius < dpos.v[1])) {
        return;
    }
    float dist2 = vec2_length2(dpos);
    if (dist2 > radius * radius) {
        return;
    }
    float dist = sqrtf(dist2);
    float overlap = radius - dist;
    if (overlap <= 0.0f) {
        return;
    }
    float adj_amount;
    if (dist < 1e-3f) {
        // If two objects have the same position, push them apart in a random
        // direction (and don't divide by zero).
        float hc = atanf(1.0f);
        float a = rand_frange(&grand, -hc, hc);
        dpos = (vec2){{cosf(a), sinf(a)}};
        adj_amount = 0.5f * overlap;
    } else {
        adj_amount = 0.5f * overlap / dist;
    }
    cx->adj = vec2_madd(cx->adj, dpos, adj_amount);
    cy->adj = vec2_madd(cy->adj, dpos, -adj_amount);
    cx->collided = true;
    cy->collided = true;
    bool stable = cx->stable & cy->stable;
    cx->stable = stable;
    cy->stable = stable;
}

// Update an object with respect to static collisions.
static void physics_update_static(struct cp_phys *restrict cx) {
    const float wall = 4.0f - cx->radius;
    for (int i = 0; i < 2; i++) {
        if (cx->pos.v[i] > wall) {
            cx->pos.v[i] = wall;
            cx->vel.v[i] = 0.0f;
        } else if (cx->pos.v[i] < -wall) {
            cx->pos.v[i] = -wall;
            cx->vel.v[i] = 0.0f;
        }
    }
}

// Update the state of entities after collisions.
static void physics_post_collision(struct cp_phys *restrict cps, int count,
                                   float invdt) {
    for (int i = 0; i < count; i++) {
        struct cp_phys *restrict cx = &cps[i];
        if (cx->collided) {
            cx->pos = vec2_add(cx->pos, cx->adj);
            if (cx->stable) {
                // When an entity pops in, don't accumulate velocity changes.
                // Instead, just adjust the position.
                cx->vel = vec2_madd(cx->vel, cx->adj, invdt);
                float maxv2 = cx->max_vel * cx->max_vel;
                float v2 = vec2_length2(cx->vel);
                if (v2 > maxv2) {
                    cx->vel = vec2_scale(cx->vel, sqrtf(v2) / cx->max_vel);
                }
            }
            cx->collided = false;
        }
    }
}

void physics_update(struct sys_phys *restrict psys, float dt) {
    if (dt < 1e-4f) {
        return;
    }
    struct cp_phys *restrict cps = psys->physics;

    // Clean up destroyed entities.
    {
        struct cp_phys *cpend = cps + psys->count;
        for (struct cp_phys *cp = cps; cp != cpend; cp++) {
            if (cp->ent.id == 0) {
                do {
                    cpend--;
                } while (cp != cpend && cpend->ent.id == 0);
                if (cp == cpend) {
                    break;
                }
                *cp = *cpend;
                psys->entities[cp->ent.id] = cp - cps;
            }
        }
        psys->count = cpend - cps;
    }

    const float invdt = 1.0f / dt;

    // Move forwards.
    for (int i = 0; i < psys->count; i++) {
        cps[i].pos = vec2_madd(cps[i].pos, cps[i].vel, dt);
        cps[i].adj = (vec2){{0.0f, 0.0f}};
        cps[i].collided = false;
    }

    // Find collisions between entities and push entities out of collisions.
    for (int i = 0; i < psys->count; i++) {
        struct cp_phys *cx = &cps[i];
        for (int j = i + 1; j < psys->count; j++) {
            struct cp_phys *cy = &cps[j];
            physics_update_pair(cx, cy);
        }
    }

    // Update velocity after collision update.
    physics_post_collision(cps, psys->count, invdt);

    // Find collisions with walls and push entities out of collisions.
    for (int i = 0; i < psys->count; i++) {
        physics_update_static(&cps[i]);
    }

    // All entities are considered stable after a physics tick.
    for (int i = 0; i < psys->count; i++) {
        cps[i].stable = true;
    }
}
