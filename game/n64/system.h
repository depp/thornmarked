#pragma once

#include "game/core/game.h"

#include <stdint.h>

struct graphics;
struct scheduler;

// The entire game state, including platform-specific details.
struct game_system {
    // Platform-agnostic game state.
    struct game_state state;

    // Frame index being rendered, or rendered next, and the corresponding
    // sample.
    unsigned current_frame;
    unsigned current_frame_sample;

    // Sample index being rendered, or rendered next.
    unsigned current_sample;

    // Timestamp of the last game update.
    uint64_t update_time;

    // Scheduler timestamps.
    unsigned time_frame;
    unsigned time_sample;

    // Track position.
    unsigned track_start; // Sample position when the track or loop started.
    int track_loop;       // Current loop the track is on, or 0 for lead-in.
    int track_pos;        // Current position within the loop or lead-in.

    // Number of audio samples per video frame, according to the clock settings.
    int samples_per_frame;
};

void game_system_init(struct game_system *restrict sys);

void game_system_update(struct game_system *restrict sys, struct scheduler *sc);

void game_system_render(struct game_system *restrict sys,
                        struct graphics *restrict gr);
