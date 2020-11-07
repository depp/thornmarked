// RCP task scheduler.
#pragma once

#include <ultra64.h>

#include <stdbool.h>

enum {
    SCHEDULER_TASK_BUFSIZE = 2,
    SCHEDULER_EVT_BUFSIZE = 8,
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

// RCP scheduler state. Except for task_queue, these fields are private.
struct scheduler {
    // Queue for sending commands to the scheduler. The messages should be
    // pointers to scheduler_task.
    OSMesgQueue task_queue;
    OSMesg task_buffer[SCHEDULER_TASK_BUFSIZE];

    // Queue for the scheduler to receive events from subsystems.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[SCHEDULER_EVT_BUFSIZE];

    // List of pending tasks, removed from queue.
    struct scheduler_task *pending_tasks[SCHEDULER_TASK_BUFSIZE];
    unsigned pending_count;

    // Pointer to the currently executing task.
    struct scheduler_task *task_running;
    OSTime task_starttime;

    // Framebuffers: framebuffer[0] is on-screen right now, framebuffer[1] being
    // swapped in by the VI thread, and framebufer[2] is waiting. The
    // framebuffers_pending field is the index of the last valid array entry.
    struct scheduler_framebuffer framebuffers[3];
    unsigned framebuffers_pending;

    // The main scheduler thread.
    OSThread thread;
};

// A task to be scheduled on the RCP.
struct scheduler_task {
    // RCP task to run.
    OSTask task;

    // Send this message to this queue when the RCP task is done.
    OSMesgQueue *done_queue;
    OSMesg done_mesg;

    // RCP task runtime, elapsed CPU counter value (delta osGetTime). Filled in
    // by the scheduler after the task finishes.
    int runtime;

    // The framebuffer to be displayed on screen after the RCP task is done.
    struct scheduler_framebuffer framebuffer;
};

// Start the scheduler.
void scheduler_start(struct scheduler *scheduler, int priority);

// Submit a task to the scheduler.
void scheduler_submit(struct scheduler *scheduler, struct scheduler_task *task);
