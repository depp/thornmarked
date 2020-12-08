// Audio tasks.
#pragma once

#include <ultra64.h>

struct game_system;
struct scheduler;

enum {
    // Audio sample rate, in Hz.
    AUDIO_SAMPLERATE = 32000,
};

// Audio task state.
struct audio_state {
    unsigned busy; // Mask for which resources are busy.
    unsigned wait; // Mask for which resources are needed for the next frame.
    int current_task;
    int current_buffer;
};

// Initialize the audio system.
void audio_init(struct game_system *restrict sys);

// Update the audio subsystem.
void audio_update(struct game_system *restrict sys);

// Render the next audio frame.
void audio_frame(struct game_system *restrict sys,
                 struct audio_state *restrict st, struct scheduler *sc,
                 OSMesgQueue *queue);
