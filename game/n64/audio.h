// Audio tasks.
#pragma once

#include <ultra64.h>

#include <stdbool.h>

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

// Length and looping information about a track.
struct audio_trackinfo {
    // ASSET DATA, do not modify without updating asset import programs.
    unsigned lead_in;
    unsigned loop_length;
};

// Number of audio samples per video frame, according to the clock settings.
extern int audio_samples_per_frame;

// Sample position when the track or loop started.
extern unsigned audio_trackstart;

// Metadata for the currently playing track.
extern struct audio_trackinfo audio_trackinfo;

// Initialize the audio system.
void audio_init(void);

// Render the next audio frame.
void audio_frame(struct audio_state *restrict st, struct scheduler *sc,
                 OSMesgQueue *queue);
