#pragma once

#include "game/core/game.h"

struct graphics;

void game_n64_init(struct game_state *restrict gs);

void game_n64_update(struct game_state *restrict gs, float dt);

void game_render(struct game_state *restrict gs, struct graphics *restrict gr);
