#include "game/n64/time.h"

#include "base/n64/scheduler.h"

#include <ultra64.h>

enum {
    // Maximum time to advance during a frame.
    MAX_DELTA_TIME = OS_CPU_COUNTER / 10,
};

void time_init(struct sys_time *restrict tm) {
    tm->current_frame = 1;
    tm->update_time = osGetTime();
}

float time_update(struct sys_time *restrict tm, struct scheduler *sc) {
    struct scheduler_frame fr = scheduler_getframe(sc);
    tm->time_frame = fr.frame;
    tm->time_sample = fr.sample;

    // Extrapolate from the reference frame.
    unsigned frame_delta = tm->current_frame - tm->time_frame;
    if (frame_delta > 2) {
        // Ignore impossible values.
        frame_delta = 2;
    }

    tm->current_frame_sample = tm->time_sample +
                               frame_delta * tm->samples_per_frame +
                               (tm->samples_per_frame >> 1);

    OSTime last_time = tm->update_time;
    tm->update_time = osGetTime();
    uint32_t delta_time = tm->update_time - last_time;
    // Clamp delta time, in case something gets out of hand.
    if (delta_time > MAX_DELTA_TIME) {
        delta_time = MAX_DELTA_TIME;
    }
    return (float)(int)delta_time * (1.0f / (float)OS_CPU_COUNTER);
}
