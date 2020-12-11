#pragma once

#include "game/core/game.h"
#include "game/n64/time.h"

#include <stdint.h>

struct graphics;
struct scheduler;

void game_system_init(struct game_state *restrict gs);

void game_system_update(struct game_state *restrict gs, struct scheduler *sc);

void game_system_render(struct game_state *restrict gs,
                        struct graphics *restrict gr);
