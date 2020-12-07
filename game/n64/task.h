// RCP task definitions.
#pragma once

#include <ultra64.h>

#include <stdint.h>

// Events received on the main queue.
typedef enum {
    EVENT_INVALID,
    EVENT_VIDEO,
    EVENT_AUDIO,
} event_type;

// A decoded event.
struct event_data {
    event_type type;
    unsigned value;
};

// Pack an event into an OSMesg.
inline OSMesg event_pack(struct event_data evt) {
    uintptr_t v = (unsigned)evt.type | (evt.value << 8);
    return (OSMesg)v;
}

// Unpack an event from an OSMesg.
inline struct event_data event_unpack(OSMesg mesg) {
    uintptr_t v = (uintptr_t)mesg;
    return (struct event_data){
        .type = v & 0xff,
        .value = v >> 8,
    };
}
