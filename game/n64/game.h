#pragma once

#include "game/game.h"

struct graphics;

void game_render(struct game_state *restrict gs, struct graphics *restrict gr);
