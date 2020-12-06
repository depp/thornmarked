#pragma once

#include "base/vectypes.h"
#include "game/core/camera.h"
#include "game/core/model.h"
#include "game/core/monster.h"
#include "game/core/physics.h"
#include "game/core/walk.h"

#include <stdbool.h>

struct graphics;

struct game_state {
    struct sys_phys physics;
    struct sys_walk walk;
    struct sys_camera camera;
    struct sys_model model;
    struct sys_monster monster;

    unsigned button_state;
    unsigned prev_button_state;
    bool show_console;
};

struct controller_input;

void game_init(struct game_state *restrict gs);
void game_input(struct game_state *restrict gs,
                const struct controller_input *restrict input);
void game_update(struct game_state *restrict gs, float dt);
