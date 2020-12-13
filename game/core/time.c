#include "game/core/time.h"

#include "base/console.h"

enum {
    AUDIO_SAMPLERATE = 32000,
};

void time_update2(struct game_time *restrict tm) {
    const float beat_duty_cycle = 0.4f;
    if (tm->track_loop > 0) {
        const float to_beats = 138.0f / (60.0f * AUDIO_SAMPLERATE);
        const float fbeat = to_beats * tm->track_pos;
        int beat = fbeat;
        const float subbeat = fbeat - beat;
        int measure = (beat >> 2) + 1;
        beat = (beat & 3) + 1;
        cprintf("%d:%02d:%d:%04.2f\n", tm->track_loop, measure, beat,
                (double)subbeat);
        tm->measure = measure;
        tm->beat = beat;
        tm->subbeat = subbeat;
        tm->on_beat = subbeat < beat_duty_cycle * 0.5f ||
                      1.0f - beat_duty_cycle * 0.5f < subbeat;
    } else {
        tm->measure = 0;
        tm->beat = 0;
        tm->subbeat = 0.0f;
        tm->on_beat = false;
    }
}
