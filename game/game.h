#pragma once

#include <ultra64.h>

#include "base/random.h"
#include "game/physics.h"

struct graphics;

struct game_state {
    struct rand rand;
    struct sys_phys physics;
};

void game_init(struct game_state *restrict gs);
void game_input(struct game_state *restrict gs, OSContPad *restrict pad);
void game_update(struct game_state *restrict gs, float dt);
void game_render(struct game_state *restrict gs, struct graphics *restrict gr);
