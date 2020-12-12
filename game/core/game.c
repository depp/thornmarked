#include "game/core/game.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "base/base.h"
#include "game/core/input.h"
#include "game/core/random.h"

#include <stdbool.h>

static void spawn_player(struct game_state *restrict gs, int player_index) {
    ent_id ent = entity_newid(&gs->ent);
    if (ent.id == 0) {
        fatal_error("spawn_player: no entity");
    }
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    pp->radius = 0.75f;
    pp->height = -1.0f;
    pp->team = TEAM_PLAYER;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    pak_texture texture = player_index == 0 ? IMG_FAIRY1 : IMG_FAIRY2;
    mp->model_id = MODEL_FAIRY;
    mp->material[0] = (struct material){
        .flags = MAT_ENABLED | MAT_CULL_BACK | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    mp->material[1] = (struct material){
        .flags = MAT_ENABLED | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    mp->animation_id = 4;
    player_new(&gs->player, player_index, ent);
}

static void spawn_monster(struct game_state *restrict gs, pak_model model,
                          pak_texture texture) {
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
    mp->model_id = model;
    mp->material[0] = (struct material){
        .flags = MAT_ENABLED | MAT_CULL_BACK | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    monster_new(&gs->monster, ent);
}

void game_init(struct game_state *restrict gs) {
    rand_init(&grand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    entity_init(&gs->ent);
    physics_init(&gs->physics);
    walk_init(&gs->walk);
    camera_init(&gs->camera);
    model_init(&gs->model);
    monster_init(&gs->monster);
    player_init(&gs->player);
    particle_init(&gs->particle);
    menu_init(gs);

    for (int i = 0; i < gs->input.count; i++) {
        spawn_player(gs, i);
    }
}

enum {
    // Number of monsters to spawn.
    MONSTER_SPAWN_COUNT = 5,
};

void game_update(struct game_state *restrict gs, float dt) {
    menu_update(gs, dt);
    particle_update(&gs->particle, dt);
    player_update(gs, dt);
    monster_update(&gs->monster, &gs->physics, &gs->walk, dt);
    walk_update(&gs->walk, &gs->physics, dt);
    physics_update(&gs->physics, dt);
    camera_update(&gs->camera);
    model_update(&gs->model);

    if (gs->spawn_active) {
        gs->spawn_time -= dt;
        if (gs->spawn_time < 0.0f) {
            gs->spawn_active = false;
            gs->spawn_time = 0.0f;
            int n = MONSTER_SPAWN_COUNT - gs->monster.count;
            if (n > 0) {
                pak_texture texture =
                    gs->spawn_type ? IMG_BLUEENEMY : IMG_GREENENEMY;
                pak_model model =
                    gs->spawn_type ? MODEL_BLUEENEMY : MODEL_GREENENEMY;
                for (int i = 0; i < n; i++) {
                    spawn_monster(gs, model, texture);
                }
            }
        }
    } else if (gs->monster.count < MONSTER_SPAWN_COUNT) {
        gs->spawn_active = true;
        gs->spawn_time = 3.0f;
    }

    if (gs->input.count >= 1 &&
        (gs->input.input[0].button_press & BUTTON_START) != 0) {
        gs->show_console = !gs->show_console;
    }
}

void entity_destroy(struct game_state *restrict gs, ent_id ent) {
    entity_freeid(&gs->ent, ent);
    {
        struct cp_phys *pp = physics_get(&gs->physics, ent);
        if (pp != NULL) {
            pp->ent = ENTITY_DESTROY;
        }
    }
    {
        struct cp_walk *wp = walk_get(&gs->walk, ent);
        if (wp != NULL) {
            wp->ent = ENTITY_DESTROY;
        }
    }
    {
        struct cp_model *mp = model_get(&gs->model, ent);
        if (mp != NULL) {
            mp->ent = ENTITY_DESTROY;
        }
    }
    {
        struct cp_monster *mp = monster_get(&gs->monster, ent);
        if (mp != NULL) {
            mp->ent = ENTITY_DESTROY;
        }
    }
    {
        struct cp_player *pl = player_get(&gs->player, ent);
        if (pl != NULL) {
            pl->ent = ENTITY_DESTROY;
        }
    }
}
