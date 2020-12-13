// Game stage system, also known as levels.
#pragma once

#include <stdbool.h>

struct game_state;

struct sys_stage {
    bool active;

    bool spawn_active; // True if spawn_time is counting down.
    float spawn_time;  // Time remaining until spawn.
    int spawn_type;    // Type of monsters to spawn.
};

// Initialize stage state.
void stage_init(struct sys_stage *restrict ssys);

// Start a new stage -- destroy all entities and start playing.
void stage_start(struct game_state *restrict gs, int player_count);

// Update the stage state.
void stage_update(struct game_state *restrict gs, float dt);
