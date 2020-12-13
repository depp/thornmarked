#pragma once

#include <stdbool.h>

// Game timing information.
struct game_time {
    // Updated by the platform.
    int track_loop; // Current loop the track is on, or 0 for lead-in.
    int track_pos;  // Current position within the loop or lead-in.

    // Updated by time_update.
    int measure;
    int beat;
    float subbeat;

    // Whether we are currently on the beat.
    bool on_beat;
};

void time_update2(struct game_time *restrict tm);
