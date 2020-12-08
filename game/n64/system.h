#pragma once

#include "game/core/game.h"

#include <stdint.h>

struct graphics;
struct scheduler;

// The entire game state, including platform-specific details.
struct game_system {
    // Platform-agnostic game state.
    struct game_state state;

    // Frame index being rendered, or rendered next.
    unsigned current_frame;

    // Timestamp of the last game update.
    uint64_t update_time;
};

void game_system_init(struct game_system *restrict sys);

void game_system_update(struct game_system *restrict sys);

void game_system_render(struct game_system *restrict sys,
                        struct graphics *restrict gr);
