#pragma once

#include "game/core/entity.h"

#include <stdbool.h>

struct game_state;

// A player component.
struct cp_player {
    ent_id ent;
    bool active;
    int state;
    float state_time;
};

// The player system.
struct sys_player {
    struct cp_player players[PLAYER_COUNT];
};

// Initialize player system.
void player_init(struct sys_player *restrict psys);

// Create new player component attached to the given entity, using the given
// controller index. Overwrites any existing player component for that entity or
// for that input index.
struct cp_player *player_new(struct sys_player *restrict msys, int player_index,
                             ent_id ent);

// Update players.
void player_update(struct game_state *restrict gs, float dt);
