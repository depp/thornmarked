
#include "assets/font.h"
#include "assets/pak.h"
#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/os.h"
#include "base/n64/scheduler.h"
#include "base/pak/pak.h"
#include "base/random.h"
#include "game/core/input.h"
#include "game/n64/defs.h"
#include "game/n64/game.h"
#include "game/n64/graphics.h"
#include "game/n64/model.h"
#include "game/n64/task.h"
#include "game/n64/text.h"
#include "game/n64/texture.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,

    // Maximum time to advance during a frame.
    MAX_DELTA_TIME = OS_CPU_COUNTER / 10,
};

extern u8 _main_thread_stack[];

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

// Main game thread.
static void main(void *arg);

void boot(void) {
    system_main(main, NULL, _main_thread_stack);
}

enum {
    RDP_OUTPUT_LEN = 64 * 1024,
};

// Info for the pak objects, to be loaded from cartridge.
struct pak_object pak_objects[(PAK_SIZE + 1) & ~1]
    __attribute__((section("uninit"), aligned(16)));

static struct scheduler scheduler;

// State of the main thread.
struct main_state {
    // Whether a controller read is in progress.
    bool controler_read_active;

    // Whether we have a controller connected.
    bool has_controller;
    // Which controller is connected.
    int controller_index;

    // Queue receiving scheduler / controller events.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[16];

    // SI event queue. Must be a separate queue, because osContRead will read
    // from it.
    OSMesgQueue si_queue;
    OSMesg si_buffer[1];

    // Graphics task state.
    struct graphics_state graphics;
};

// Read the next event sent to the main thread and process it.
static int process_event(struct main_state *restrict st, int flags) {
    struct event_data e;
    {
        OSMesg evt;
        int ret = osRecvMesg(&st->evt_queue, &evt, flags);
        if (ret != 0)
            return ret;
        e = event_unpack(evt);
    }
    switch (e.type) {
    case EVENT_VTASKDONE:
        st->graphics.busy &= ~graphics_taskmask(e.value);
        break;
    case EVENT_VBUFDONE:
        st->graphics.busy &= ~graphics_buffermask(e.value);
        break;
    default:
        fatal_error("Bad message type: %d", e.type);
    }
    return 0;
}

static struct main_state main_state;
static struct game_state game_state;

static void main(void *arg) {
    (void)arg;
    struct main_state *st = &main_state;

    // Initialize peripheral manager.
    osCreatePiManager(OS_PRIORITY_PIMGR, &pi_message_queue, pi_message_buffer,
                      PI_MSG_COUNT);

    pak_init(PAK_SIZE);

    // Set up message queues.
    osCreateMesgQueue(&st->evt_queue, st->evt_buffer,
                      ARRAY_COUNT(st->evt_buffer));
    osCreateMesgQueue(&st->si_queue, st->si_buffer, ARRAY_COUNT(st->si_buffer));
    osSetEventMesg(OS_EVENT_SI, &st->si_queue, NULL);

    font_load(FONT_GG);

    // Scan for first controller.
    {
        u8 cont_mask;
        OSContStatus cont_status[MAXCONTROLLERS];
        osContInit(&st->si_queue, &cont_mask, cont_status);
        for (int i = 0; i < MAXCONTROLLERS; i++) {
            if ((cont_mask & (1u << i)) != 0 && cont_status[i].errno == 0 &&
                (cont_status[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL) {
                st->controller_index = i;
                st->has_controller = true;
                break;
            }
        }
    }

    OSTime cur_time = osGetTime();

    game_n64_init(&game_state);

    scheduler_start(&scheduler, 1);
    int frame_num = 0;

    for (int current_task = 0;; current_task ^= 1) {
        console_init(&console, CONSOLE_TRUNCATE);
        console_printf(&console, "Frame %d\n", frame_num);
        /*
        console_printf(&console, "Time: %5.1f ms\n",
                       (double)st->rcp_time * (1.0e3 / OS_CPU_COUNTER));
        */
        frame_num++;

        // Wait until the task and framebuffer are both free to use.
        while ((st->graphics.busy & st->graphics.wait) != 0) {
            process_event(st, OS_MESG_BLOCK);
        }
        while (process_event(st, OS_MESG_NOBLOCK) == 0) {}

        // Read the controller, if ready.
        bool has_cont = false;
        if (osRecvMesg(&st->si_queue, NULL, OS_MESG_NOBLOCK) == 0) {
            has_cont = true;
        }
        if (has_cont) {
            st->controler_read_active = false;
            OSContPad controller_state[MAXCONTROLLERS];
            osContGetReadData(controller_state);
            st->controler_read_active = false;
            if (st->has_controller) {
                const OSContPad *restrict pad =
                    &controller_state[st->controller_index];
                struct controller_input input = {.buttons = pad->button};
                const int dead_zone = 4;
                if (pad->stick_x < -dead_zone || dead_zone < pad->stick_x ||
                    pad->stick_y < -dead_zone || dead_zone < pad->stick_y) {
                    const float scale = 1.0f / 64.0f;
                    input.joystick = (vec2){{
                        scale * (float)pad->stick_x,
                        scale * (float)pad->stick_y,
                    }};
                }
                game_input(&game_state, &input);
            }
        }
        if (!st->controler_read_active) {
            osContStartReadData(&st->si_queue);
            st->controler_read_active = true;
        }

        {
            OSTime last_time = cur_time;
            cur_time = osGetTime();
            uint32_t delta_time = cur_time - last_time;
            // Clamp delta time, in case something gets out of hand.
            if (delta_time > MAX_DELTA_TIME) {
                delta_time = MAX_DELTA_TIME;
            }
            float dt = (float)(int)delta_time * (1.0f / (float)OS_CPU_COUNTER);
            game_n64_update(&game_state, dt);
        }

        graphics_frame(&game_state, &st->graphics, &scheduler, &st->evt_queue);
    }
}
