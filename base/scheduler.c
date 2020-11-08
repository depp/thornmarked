#include "base/scheduler.h"

#include "base/base.h"
#include "base/console.h"

#include <limits.h>
#include <stdint.h>

// =============================================================================
// Video State
// =============================================================================

struct scheduler_vstate {
    unsigned pending;
    // Framebuffers: framebuffer[0] is on-screen right now, framebuffer[1] being
    // swapped in by the VI thread, and framebufer[2] is waiting. The
    // framebuffers_pending field is the index of the last valid array entry.
    struct scheduler_framebuffer buffers[3];
};

static void scheduler_vpush(struct scheduler_vstate *restrict st,
                            struct scheduler_framebuffer *restrict fb) {
    if (((uintptr_t)fb->ptr & 15) != 0) {
        fatal_error("Unaligned framebuffer\nptr=%p", fb->ptr);
    }
    if (st->pending >= 2) {
        fatal_error("Framebuffer overflow");
    }
    if (st->pending == 0) {
        osViSwapBuffer(fb->ptr);
        if (st->buffers[0].ptr == NULL) {
            osViBlack(false);
        }
    }
    st->pending++;
    st->buffers[st->pending] = *fb;
}

static void scheduler_vpop(struct scheduler_vstate *restrict st) {
    if (st->pending == 0) {
        return;
    }
    if (st->buffers[0].done_queue != NULL) {
        int r = osSendMesg(st->buffers[0].done_queue, st->buffers[0].done_mesg,
                           OS_MESG_NOBLOCK);
        if (r != 0) {
            fatal_error("Dropped video buffer message");
        }
    }
    st->buffers[0] = st->buffers[1];
    st->buffers[1] = st->buffers[2];
    st->buffers[2] = (struct scheduler_framebuffer){0};
    if (st->pending > 1) {
        osViSwapBuffer(st->buffers[1].ptr);
    }
    st->pending--;
}

// =============================================================================
// Audio State
// =============================================================================

struct scheduler_astate {
    unsigned count;
    // Audiobuffers: audiobuffer[0] and audiobuffer[1] are in the DMA FIFO, and
    // have been sent to osAiSetNextBuffer. audiobuffer[2] is pending. The
    // audiobuffers_count is the number of entries in this array that are valid.
    struct scheduler_audiobuffer buffers[3];
};

static void scheduler_apush(struct scheduler_astate *restrict st,
                            struct scheduler_audiobuffer *restrict ab) {
    if ((((uintptr_t)ab->ptr | ab->size) & 15) != 0) {
        fatal_error("Unaligned audio buffer\nptr=%p\nsize=%zu", ab->ptr,
                    ab->size);
    }
    if (st->count > 2) {
        fatal_error("Audio buffer overflow");
    }
    if (st->count < 2) {
        int r = osAiSetNextBuffer(ab->ptr, ab->size);
        if (r != 0) {
            fatal_error("Audio device busy");
        }
    }
    st->buffers[st->count] = *ab;
    st->count++;
}

static void scheduler_apop(struct scheduler_astate *restrict st) {
    if (st->count == 0) {
        return;
    }
    if (st->buffers[0].done_queue != NULL) {
        int r = osSendMesg(st->buffers[0].done_queue, st->buffers[0].done_mesg,
                           OS_MESG_NOBLOCK);
        if (r != 0) {
            fatal_error("Dropped audio buffer message");
        }
    }
    st->buffers[0] = st->buffers[1];
    st->buffers[1] = st->buffers[2];
    st->buffers[2] = (struct scheduler_audiobuffer){0};
    if (st->count > 1) {
        int r = osAiSetNextBuffer(st->buffers[1].ptr, st->buffers[1].size);
        if (r != 0) {
            fatal_error("Audio device busy");
        }
    }
    st->count--;
}

// =============================================================================
// Main Scheduler Thread
// =============================================================================

// Scheduler events. Event 0 is invalid in order to catch errors.
typedef enum {
    EVT_INVALID,
    EVT_TASK,  // New task submitted.
    EVT_RSP,   // RSP is done.
    EVT_RDP,   // RDP is done.
    EVT_AUDIO, // Audio buffer consumed.
    EVT_VSYNC, // Vertical refresh.
} scheduler_event;

typedef enum {
    STATE_READY,
    STATE_AUDIO,
    STATE_VIDEO,
} scheduler_state;

static void scheduler_done(struct scheduler_task *restrict task,
                           OSTime starttime) {
    uint64_t delta = osGetTime() - starttime;
    if (delta > INT_MAX) {
        task->runtime = INT_MAX;
    } else {
        task->runtime = delta;
    }
    if (task->done_queue != NULL) {
        int r = osSendMesg(task->done_queue, task->done_mesg, OS_MESG_NOBLOCK);
        if (r != 0) {
            fatal_error("Dropped task done message");
        }
    }
}

static void scheduler_main(void *arg) {
    struct scheduler *sc = arg;
    scheduler_state state = STATE_READY;

    // List of pending tasks, removed from queue.
    struct scheduler_task *pending_tasks[SCHEDULER_TASK_BUFSIZE] = {0};
    unsigned pending_count = 0;

    // Pointer to the currently executing task, and the time when it started.
    struct scheduler_task *restrict task = NULL;
    OSTime starttime = 0;

    // Audio and video state.
    struct scheduler_vstate video = {0};
    struct scheduler_astate audio = {0};

    for (;;) {
        scheduler_event evt;
        {
            OSMesg mesg;
            osRecvMesg(&sc->evt_queue, &mesg, OS_MESG_BLOCK);
            evt = (uintptr_t)mesg;
        }
        switch (evt) {
        case EVT_TASK:
            break;

        case EVT_RSP:
            if (state == STATE_AUDIO) {
                if ((task->flags & SCHEDULER_TASK_AUDIOBUFFER) != 0) {
                    scheduler_apush(&audio, &task->data.audiobuffer);
                }
                scheduler_done(task, starttime);
                task = NULL;
                state = STATE_READY;
            }
            break;

        case EVT_RDP:
            if (state == STATE_VIDEO) {
                if ((task->flags & SCHEDULER_TASK_FRAMEBUFFER) != 0) {
                    scheduler_vpush(&video, &task->data.framebuffer);
                }
                scheduler_done(task, starttime);
                task = NULL;
                state = STATE_READY;
            }
            break;

        case EVT_AUDIO:
            scheduler_apop(&audio);
            break;

        case EVT_VSYNC:
            scheduler_vpop(&video);
            break;

        default:
            fatal_error("Invalid scheduler event: %d", evt);
        }

        // Read tasks from the task queue.
        {
            OSMesg mesg;
            while (osRecvMesg(&sc->task_queue, &mesg, OS_MESG_NOBLOCK) == 0) {
                if (pending_count >= ARRAY_COUNT(pending_tasks)) {
                    fatal_error("Task overflow");
                }
                pending_tasks[pending_count++] = mesg;
            }
        }

        // If we are idle and there is a task pending, run it.
        while (state == STATE_READY && pending_count > 0) {
            // Dequeue.
            task = pending_tasks[0];
            for (unsigned i = 1; i < ARRAY_COUNT(pending_tasks); i++) {
                pending_tasks[i - 1] = pending_tasks[i];
            }
            pending_tasks[ARRAY_COUNT(pending_tasks) - 1] = NULL;
            pending_count--;

            // Run.
            starttime = osGetTime();
            switch (task->flags &
                    (SCHEDULER_TASK_VIDEO | SCHEDULER_TASK_AUDIO)) {
            case 0:
                switch (task->flags & (SCHEDULER_TASK_FRAMEBUFFER |
                                       SCHEDULER_TASK_AUDIOBUFFER)) {
                case 0:
                    break;
                case SCHEDULER_TASK_FRAMEBUFFER:
                    scheduler_vpush(&video, &task->data.framebuffer);
                    break;
                case SCHEDULER_TASK_AUDIOBUFFER:
                    scheduler_apush(&audio, &task->data.audiobuffer);
                    break;
                default:
                    fatal_error("Invalid task flags");
                }
                scheduler_done(task, starttime);
                task = NULL;
                break;
            case SCHEDULER_TASK_VIDEO:
                osSpTaskLoad(&task->task);
                osSpTaskStartGo(&task->task);
                state = STATE_VIDEO;
                break;
            case SCHEDULER_TASK_AUDIO:
                osSpTaskLoad(&task->task);
                osSpTaskStartGo(&task->task);
                state = STATE_AUDIO;
                break;
            default:
                fatal_error("Invalid task flags");
            }
        }
    }
}

void scheduler_start(struct scheduler *sc, int priority, int video_divisor) {
    extern u8 _scheduler_thread_stack[];
    osCreateMesgQueue(&sc->task_queue, sc->task_buffer,
                      ARRAY_COUNT(sc->task_buffer));
    osCreateMesgQueue(&sc->evt_queue, sc->evt_buffer,
                      ARRAY_COUNT(sc->evt_buffer));
    osSetEventMesg(OS_EVENT_AI, &sc->evt_queue, (OSMesg)EVT_AUDIO);
    osSetEventMesg(OS_EVENT_SP, &sc->evt_queue, (OSMesg)EVT_RSP);
    osSetEventMesg(OS_EVENT_DP, &sc->evt_queue, (OSMesg)EVT_RDP);
    osViSetEvent(&sc->evt_queue, (OSMesg)EVT_VSYNC, video_divisor);
    osCreateThread(&sc->thread, 4, scheduler_main, sc, _scheduler_thread_stack,
                   priority);
    osStartThread(&sc->thread);
}

void scheduler_submit(struct scheduler *scheduler,
                      struct scheduler_task *task) {
    osSendMesg(&scheduler->task_queue, task, OS_MESG_BLOCK);
    osSendMesg(&scheduler->evt_queue, (OSMesg)EVT_TASK, OS_MESG_BLOCK);
}
