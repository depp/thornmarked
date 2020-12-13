#include "game/core/monster.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "base/base.h"
#include "base/vec3.h"
#include "game/core/game.h"
#include "game/core/physics.h"
#include "game/core/random.h"

enum {
    // Maximum number of monster components.
    MONSTER_COUNT = 8,
};

void monster_init(struct sys_monster *restrict msys) {
    *msys = (struct sys_monster){
        .monsters = mem_alloc(sizeof(*msys->monsters) * MONSTER_COUNT),
        .entities = mem_calloc(sizeof(*msys->entities) * ENTITY_COUNT),
    };
}

void monster_destroyall(struct sys_monster *restrict msys);

struct cp_monster *monster_get(struct sys_monster *restrict msys, ent_id ent);

struct cp_monster *monster_new(struct sys_monster *restrict msys, ent_id ent) {
    int index = msys->entities[ent.id];
    struct cp_monster *p = &msys->monsters[index];
    if (p->ent.id != ent.id || index >= msys->count) {
        index = msys->count;
        if (index >= MONSTER_COUNT) {
            fatal_error("Too many monsters");
        }
        msys->count = index + 1;
        p = &msys->monsters[index];
        msys->entities[ent.id] = index;
    }
    *p = (struct cp_monster){
        .ent = ent,
    };
    return p;
}

// Information about a monster type.
struct monster_info {
    pak_model model;
    pak_texture texture;
};

static const struct monster_info monster_info[] = {
    [MONSTER_BLUE] =
        {
            .model = MODEL_BLUEENEMY,
            .texture = IMG_BLUEENEMY,
        },
    [MONSTER_GREEN] =
        {
            .model = MODEL_GREENENEMY,
            .texture = IMG_GREENENEMY,
        },
};

void monster_spawn(struct game_state *restrict gs, monster_type type) {
    if (type <= 0 || (int)ARRAY_COUNT(monster_info) <= type) {
        fatal_error("spawn_monster: bad type: %d", (int)type);
    }
    ent_id ent = entity_newid(&gs->ent);
    if (ent.id == 0) {
        fatal_error("spawn_monster: no entity");
    }
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    const float radius = 0.75f, wall = 4.0f, r = wall - radius;
    pp->pos = (vec2){{
        rand_frange(&grand, -r, r),
        rand_frange(&grand, -r, r),
    }};
    pp->radius = radius;
    pp->team = TEAM_MONSTER;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    mp->model_id = monster_info[type].model;
    mp->material[0] = (struct material){
        .flags = MAT_ENABLED | MAT_CULL_BACK | MAT_VERTEX_COLOR,
        .texture_id = monster_info[type].texture,
    };
    monster_new(&gs->monster, ent);
}

void monster_update(struct sys_monster *restrict msys,
                    struct sys_phys *restrict psys,
                    struct sys_walk *restrict wsys, float dt) {
    (void)psys;
    struct cp_monster *mstart = msys->monsters, *mend = mstart + msys->count;
    for (struct cp_monster *mp = mstart; mp != mend; mp++) {
        if (mp->ent.id == 0) {
            do {
                mend--;
            } while (mp != mend && mend->ent.id == 0);
            if (mp == mend) {
                break;
            }
            *mp = *mend;
            msys->entities[mp->ent.id] = mp - mstart;
        }
        mp->timer -= dt;
        if (mp->timer <= 0.0f) {
            mp->timer = rand_frange(&grand, 1.0f, 2.0f);
            struct cp_walk *restrict wp = walk_get(wsys, mp->ent);
            if (wp != NULL) {
                wp->drive = (vec2){{rand_frange(&grand, -1.0f, 1.0f),
                                    rand_frange(&grand, -1.0f, 1.0f)}};
            }
        }
    }
    msys->count = mend - mstart;
}

void monster_damage(struct game_state *restrict gs, ent_id ent) {
    struct cp_phys *pp = physics_get(&gs->physics, ent);
    entity_destroy(gs, ent);
    if (pp != NULL) {
        particle_create(&gs->particle, vec3_vec2(pp->pos, pp->height), 2.0f,
                        (color){{255, 0, 0, 255}});
    }
}
