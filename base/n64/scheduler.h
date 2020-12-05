// RCP task scheduler.
#pragma once

#include <ultra64.h>

#include <stdbool.h>
#include <stddef.h>

enum {
    SCHEDULER_TASK_BUFSIZE = 5,
    SCHEDULER_EVT_BUFSIZE = 8,
};

// Scheduler task flags.
enum {
    SCHEDULER_TASK_VIDEO = 01,
    SCHEDULER_TASK_AUDIO = 02,
    SCHEDULER_TASK_FRAMEBUFFER = 04,
    SCHEDULER_TASK_AUDIOBUFFER = 010,
};

// A framebuffer to be displayed on screen.
struct scheduler_framebuffer {
    // Switch to this framebuffer.
    void *ptr;

    // Send this message to this queue after the framebuffer is no longer on
    // screen, after another framebuffer has replaced it.
    OSMesgQueue *done_queue;
    OSMesg done_mesg;
};

struct scheduler_audiobuffer {
    // Pointer to audio data, and its length in bytes.
    void *ptr;
    size_t size;

    // Send this message to this queue after the audio buffer has been consumed.
    OSMesgQueue *done_queue;
    OSMesg done_mesg;
};

// RCP scheduler state. Except for task_queue, these fields are private.
struct scheduler {
    // Queue for sending commands to the scheduler. The messages should be
    // pointers to scheduler_task.
    OSMesgQueue task_queue;
    OSMesg task_buffer[SCHEDULER_TASK_BUFSIZE];

    // Queue for the scheduler to receive events from subsystems.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[SCHEDULER_EVT_BUFSIZE];

    // The main scheduler thread.
    OSThread thread;
};

// A task to be scheduled on the RCP.
struct scheduler_task {
    unsigned flags;

    // RCP task to run.
    OSTask task;

    // Send this message to this queue when the RCP task is done.
    OSMesgQueue *done_queue;
    OSMesg done_mesg;

    // RCP task runtime, elapsed CPU counter value (delta osGetTime). Filled in
    // by the scheduler after the task finishes.
    int runtime;

    union {
        // The audiobuffer to be played after the RCP task is done.
        struct scheduler_audiobuffer audiobuffer;

        // The framebuffer to be displayed on screen after the RCP task is done.
        struct scheduler_framebuffer framebuffer;
    } data;
};

// Start the scheduler. The video_divisor is the number of vsyncs per vsync
// event. A divisor of 1 swaps frames at 60/50 Hz, a divisor of 2 swaps at 30/25
// Hz, etc.
void scheduler_start(struct scheduler *scheduler, int priority,
                     int video_divisor);

// Submit a task to the scheduler.
void scheduler_submit(struct scheduler *scheduler, struct scheduler_task *task);
