#include "game/scheduler.h"

#include "base/defs.h"
#include "game/defs.h"

#include <limits.h>
#include <stdint.h>

enum {
    NUM_FIELDS = 1,
};

enum {
    EVT_TASK,  // New task submitted.
    EVT_RDP,   // RDP is done.
    EVT_VSYNC, // Vertical refresh.
};

static void scheduler_main(void *arg) {
    struct scheduler *sc = arg;
    for (;;) {
        OSMesg mesg;
        osRecvMesg(&sc->evt_queue, &mesg, OS_MESG_BLOCK);
        switch ((uintptr_t)mesg) {
        case EVT_TASK:
            break;

        case EVT_RDP: {
            struct scheduler_task *restrict task = sc->task_running;
            sc->task_running = NULL;
            if (task == NULL) {
                fatal_error("NULL task");
            }
            uint64_t delta = osGetTime() - sc->task_starttime;
            if (delta > INT_MAX) {
                task->runtime = INT_MAX;
            } else {
                task->runtime = delta;
            }
            if (task->framebuffer.ptr != NULL) {
                unsigned pending = sc->framebuffers_pending;
                if (pending >= 2) {
                    fatal_error("Framebuffer overflow");
                }
                if (pending == 0) {
                    osViSwapBuffer(task->framebuffer.ptr);
                    if (sc->framebuffers[0].ptr == NULL) {
                        osViBlack(false);
                    }
                }
                pending++;
                sc->framebuffers[pending] = task->framebuffer;
                sc->framebuffers_pending = pending;
            }
            if (task->done_queue != NULL) {
                int r = osSendMesg(task->done_queue, task->done_mesg,
                                   OS_MESG_NOBLOCK);
                if (r != 0) {
                    fatal_error("Dropped RCP task done message");
                }
            }
        } break;

        case EVT_VSYNC: {
            unsigned pending = sc->framebuffers_pending;
            if (pending > 0) {
                struct scheduler_framebuffer *restrict fb = sc->framebuffers;
                if (fb[0].done_queue != NULL) {
                    int r = osSendMesg(fb[0].done_queue, fb[0].done_mesg,
                                       OS_MESG_NOBLOCK);
                    if (r != 0) {
                        fatal_error("Dropped VSYNC message");
                    }
                }
                fb[0] = fb[1];
                fb[1] = fb[2];
                fb[2] = (struct scheduler_framebuffer){0};
                if (pending > 1) {
                    osViSwapBuffer(fb[1].ptr);
                }
                sc->framebuffers_pending = pending - 1;
            }
        } break;

        default:
            fatal_error("scheduler event: %u", (unsigned)(uintptr_t)mesg);
        }

        // Read tasks from the task queue.
        {
            OSMesg mesg;
            while (osRecvMesg(&sc->task_queue, &mesg, OS_MESG_NOBLOCK) == 0) {
                if (sc->pending_count >= ARRAY_COUNT(sc->pending_tasks)) {
                    fatal_error("Task overflow");
                }
                sc->pending_tasks[sc->pending_count++] = mesg;
            }
        }

        // If we are idle and there is a task pending, run it.
        if (sc->task_running == NULL && sc->pending_count > 0) {
            // Dequeue.
            struct scheduler_task *task = sc->pending_tasks[0];
            for (unsigned i = 1; i < ARRAY_COUNT(sc->pending_tasks); i++) {
                sc->pending_tasks[i - 1] = sc->pending_tasks[i];
            }
            sc->pending_tasks[ARRAY_COUNT(sc->pending_tasks) - 1] = NULL;
            sc->pending_count--;

            // Run.
            sc->task_starttime = osGetTime();
            osSpTaskLoad(&task->task);
            osSpTaskStartGo(&task->task);
            sc->task_running = task;
        }
    }
}

void scheduler_start(struct scheduler *sc) {
    extern u8 _scheduler_thread_stack[];
    osCreateMesgQueue(&sc->task_queue, sc->task_buffer,
                      ARRAY_COUNT(sc->task_buffer));
    osCreateMesgQueue(&sc->evt_queue, sc->evt_buffer,
                      ARRAY_COUNT(sc->evt_buffer));
    osSetEventMesg(OS_EVENT_DP, &sc->evt_queue, (OSMesg)EVT_RDP);
    osViSetEvent(&sc->evt_queue, (OSMesg)EVT_VSYNC, NUM_FIELDS);
    osCreateThread(&sc->thread, 4, scheduler_main, sc, _scheduler_thread_stack,
                   PRIORITY_SCHEDULER);
    osStartThread(&sc->thread);
}

void scheduler_submit(struct scheduler *scheduler,
                      struct scheduler_task *task) {
    osSendMesg(&scheduler->task_queue, task, OS_MESG_BLOCK);
    osSendMesg(&scheduler->evt_queue, (OSMesg)EVT_TASK, OS_MESG_BLOCK);
}
