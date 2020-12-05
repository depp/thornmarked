#include "game/core/game.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "game/core/input.h"
#include "game/core/random.h"

#include <stdbool.h>

void game_init(struct game_state *restrict gs) {
    rand_init(&grand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    physics_init(&gs->physics);
    walk_init(&gs->walk);
    camera_init(&gs->camera);
    model_init(&gs->model);
    for (int i = 0; i < 3; i++) {
        struct cp_phys *restrict phys = physics_new(&gs->physics, (ent_id){i});
        phys->pos = (vec2){{
            rand_frange(&grand, -1.0f, 1.0f),
            rand_frange(&grand, -1.0f, 1.0f),
        }};
        phys->vel = (vec2){{
            rand_frange(&grand, -1.0f, 1.0f),
            rand_frange(&grand, -1.0f, 1.0f),
        }};
    }
    walk_new(&gs->walk, (ent_id){0});
    gs->button_state = 0;
    gs->prev_button_state = 0;
    struct cp_model *restrict mp;
    mp = model_new(&gs->model, (ent_id){1});
    mp->model_id = MODEL_BLUEENEMY;
    mp = model_new(&gs->model, (ent_id){2});
    mp->model_id = MODEL_GREENENEMY;
    mp = model_new(&gs->model, (ent_id){0});
    mp->model_id = MODEL_FAIRY;
}

void game_input(struct game_state *restrict gs,
                const struct controller_input *restrict input) {
    for (int i = 0; i < gs->walk.count; i++) {
        gs->walk.components[i].drive = input->joystick;
    }
    gs->button_state = input->buttons;
}

void game_update(struct game_state *restrict gs, float dt) {
    walk_update(&gs->walk, &gs->physics, dt);
    physics_update(&gs->physics, dt);
    camera_update(&gs->camera);
    unsigned button = gs->button_state & ~gs->prev_button_state;
    gs->prev_button_state = gs->button_state;
    if ((button & BUTTON_A) != 0) {
        gs->show_console = !gs->show_console;
    }
}
