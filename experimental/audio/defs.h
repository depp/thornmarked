#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <ultra64.h>

typedef enum {
    EVENT_INVALID,
    EVENT_CONTROLLER,
    EVENT_VTASKDONE,
    EVENT_VBUFDONE,
    EVENT_AUDIO,
} event_type;

struct event_data {
    event_type type;
    int value;
};

inline OSMesg event_pack(struct event_data evt) {
    uintptr_t v = (unsigned)evt.type | ((unsigned)evt.value << 8);
    return (OSMesg)v;
}

inline struct event_data event_unpack(OSMesg mesg) {
    uintptr_t v = (uintptr_t)mesg;
    return (struct event_data){
        .type = v & 0xff,
        .value = v >> 8,
    };
}

struct scheduler;

struct audio_state {
    unsigned busy; // Mask for which resources are busy.
    unsigned wait; // Mask for which resources are needed.
    int current_task;
    int current_buffer;
};

void audio_init(void);

void audio_frame(struct audio_state *restrict st, struct scheduler *sc,
                 OSMesgQueue *queue);
