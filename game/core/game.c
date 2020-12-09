#include "game/core/game.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "base/base.h"
#include "game/core/input.h"
#include "game/core/random.h"

#include <stdbool.h>

static void spawn_player(struct game_state *restrict gs, ent_id ent) {
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    pp->radius = 0.25f;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    mp->model_id = MODEL_FAIRY;
    mp->texture_id = IMG_FAIRY2;
    mp->animation_id = 4;
}

static void spawn_monster(struct game_state *restrict gs, ent_id ent,
                          pak_model model, pak_texture texture) {
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    pp->pos = (vec2){{
        rand_frange(&grand, -2.0f, 2.0f),
        rand_frange(&grand, -2.0f, 2.0f),
    }};
    pp->radius = 0.25f;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    mp->model_id = model;
    mp->texture_id = texture;
    monster_new(&gs->monster, ent);
}

void game_init(struct game_state *restrict gs) {
    rand_init(&grand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    physics_init(&gs->physics);
    walk_init(&gs->walk);
    camera_init(&gs->camera);
    model_init(&gs->model);
    monster_init(&gs->monster);

    spawn_player(gs, (ent_id){0});
    for (int i = 0; i < 5; i++) {
        spawn_monster(gs, (ent_id){i + 1},
                      i & 1 ? MODEL_BLUEENEMY : MODEL_GREENENEMY,
                      i & 1 ? IMG_BLUEENEMY : IMG_GREENENEMY);
    }
}

void game_update(struct game_state *restrict gs, float dt) {
    struct cp_walk *restrict wp = walk_get(&gs->walk, (ent_id){0});
    if (wp != NULL && gs->input.count >= 1) {
        wp->drive = gs->input.input[0].joystick;
    }

    monster_update(&gs->monster, &gs->physics, &gs->walk, dt);
    walk_update(&gs->walk, &gs->physics, dt);
    physics_update(&gs->physics, dt);
    camera_update(&gs->camera);
    if (gs->input.count >= 1 &&
        (gs->input.input[0].button_press & BUTTON_A) != 0) {
        gs->show_console = !gs->show_console;
    }
}
