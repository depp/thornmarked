#pragma once

#include <stdint.h>

struct game_system;
struct scheduler;

// Time system.
struct sys_time {
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

// Initialize the timing subsystem.
void time_init(struct sys_time *restrict tm);

// Update the timing system. Returns the time delta.
float time_update(struct sys_time *restrict tm, struct scheduler *sc);
