// Entity spawning.
#pragma once

#include "base/pak/types.h"

struct game_state;

// Spawn a player entity.
void spawn_player(struct game_state *restrict gs, int player_index);
