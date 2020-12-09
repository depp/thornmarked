#include "game/n64/time.h"

#include "base/base.h"
#include "base/n64/scheduler.h"
#include "game/core/time.h"
#include "game/n64/audio.h"
#include "game/n64/graphics.h"

#include <ultra64.h>

enum {
    // Maximum time to advance during a frame.
    MAX_DELTA_TIME = OS_CPU_COUNTER / 10,
};

// Timestamp of the last game update.
static uint64_t last_update_time;

// Audio sample index of the last game update.
static unsigned last_update_sample;

void time_init(void) {
    last_update_time = osGetTime();
}

float time_update(struct game_time *restrict tm, struct scheduler *sc) {
    int delta_time;
    // Calculate delta.
    {
        OSTime last_time = last_update_time;
        OSTime cur_time = osGetTime();
        unsigned udelta_time = cur_time - last_time;
        // Clamp delta time, in case something gets out of hand.
        if (udelta_time > MAX_DELTA_TIME) {
            udelta_time = MAX_DELTA_TIME;
        }
        last_update_time = last_time;
        delta_time = udelta_time;
    }

    // Calculate the audio position.
    unsigned frame_sample;
    {
        struct scheduler_frame fr = scheduler_getframe(sc);

        // Extrapolate from the reference frame.
        unsigned frame_delta = graphics_current_frame - fr.frame;
        if (frame_delta > 2) {
            // Ignore impossible values.
            frame_delta = 2;
        }

        // Sample index corresponding to the current frame.
        frame_sample = fr.sample + frame_delta * audio_samples_per_frame +
                       (audio_samples_per_frame >> 1);
    }

    // Calculate the audio track position.
    {
        unsigned track_offset = frame_sample - audio_trackstart;
        if (track_offset > 60 * 60 * AUDIO_SAMPLERATE) {
            // Paranoia? If sample goes backwards for some reason?
            fatal_error("Audio desynchronized");
        }
        unsigned end = tm->track_loop == 0 ? audio_trackinfo.lead_in
                                           : audio_trackinfo.loop_length;
        if (track_offset >= end) {
            audio_trackstart += end;
            tm->track_loop++;
        }
        tm->track_pos = track_offset;
    }

    int delta_sample = frame_sample - last_update_sample;
    last_update_sample = frame_sample;

    (void)delta_time;
    return (float)delta_sample * (1.0f / AUDIO_SAMPLERATE);
    // (float)(int)delta_time * (1.0f / (float)OS_CPU_COUNTER);
}
