#pragma once

#include "base/random.h"
#include "game/camera.h"
#include "game/model.h"
#include "game/physics.h"
#include "game/walk.h"

#include <ultra64.h>

#include <stdbool.h>

struct graphics;

struct game_state {
    struct rand rand;
    struct sys_phys physics;
    struct sys_walk walk;
    struct sys_camera camera;
    struct sys_model model;

    unsigned button_state;
    unsigned prev_button_state;
    bool show_console;
};

void game_init(struct game_state *restrict gs);
void game_input(struct game_state *restrict gs, OSContPad *restrict pad);
void game_update(struct game_state *restrict gs, float dt);
void game_render(struct game_state *restrict gs, struct graphics *restrict gr);
