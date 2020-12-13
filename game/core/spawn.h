// Entity spawning.
#pragma once

#include "base/pak/types.h"

struct game_state;

// Spawn a player entity.
void spawn_player(struct game_state *restrict gs, int player_index);

// Spawn a monster entity.
void spawn_monster(struct game_state *restrict gs, pak_model model,
                   pak_texture texture);
