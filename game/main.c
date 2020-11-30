#include "assets/assets.h"
#include "base/base.h"
#include "base/console.h"
#include "base/console_n64.h"
#include "base/os.h"
#include "base/pak/pak.h"
#include "base/random.h"
#include "base/scheduler.h"
#include "game/defs.h"
#include "game/game.h"
#include "game/graphics.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,

    // Maximum time to advance during a frame.
    MAX_DELTA_TIME = OS_CPU_COUNTER / 10,
};

// Stacks (defined in linker script).
extern u8 _main_thread_stack[];
extern u8 _idle_thread_stack[];

OSThread idle_thread;
static OSThread main_thread;

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

static u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.cfb"), aligned(16)));
static u16 zbuffer[SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.zb"), aligned(16)));

// Idle thread. Creates other threads then drops to lowest priority.
static void idle(void *arg);

// Main game thread.
static void main(void *arg);

// Main N64 entry point. This must be declared extern.
void boot(void);

void boot(void) {
    osInitialize();
    thread_create(&idle_thread, idle, NULL, _idle_thread_stack,
                  PRIORITY_IDLE_INIT);
    osStartThread(&idle_thread);
}

static void idle(void *arg) {
    (void)arg;

    // Initialize video.
    osCreateViManager(OS_PRIORITY_VIMGR);
    OSViMode *mode;
    switch (osTvType) {
    case OS_TV_PAL:
        mode = &osViModeFpalLpn1;
        break;
    default:
    case OS_TV_NTSC:
        mode = &osViModeNtscLpn1;
        break;
    case OS_TV_MPAL:
        mode = &osViModeMpalLpn1;
        break;
    }
    osViSetMode(mode);
    osViBlack(1);

    // Initialize peripheral manager.
    osCreatePiManager(OS_PRIORITY_PIMGR, &pi_message_queue, pi_message_buffer,
                      PI_MSG_COUNT);

    // Start main thread.
    thread_create(&main_thread, main, NULL, _main_thread_stack, PRIORITY_MAIN);
    osStartThread(&main_thread);

    // Idle loop.
    osSetThreadPri(NULL, PRIORITY_IDLE);
    for (;;) {}
}

enum {
    SP_STACK_SIZE = 1024,
    RDP_OUTPUT_LEN = 64 * 1024,
};

static u64 sp_dram_stack[SP_STACK_SIZE / 8] __attribute__((section("uninit")));

// Info for the pak objects, to be loaded from cartridge.
struct pak_object pak_objects[(PAK_SIZE + 1) & ~1] __attribute__((aligned(16)));

static Gfx display_lists[2][1024] __attribute__((section("uninit")));
static Mtx matrixes[2][64] __attribute__((section("uninit")));
static struct scheduler scheduler;

// Event types for events on the main thread.
enum {
    EVT_TASKDONE, // RCP task complete.
    EVT_FBDONE,   // Framebuffer no longer in use.
};

// Create an event for the main thread.
static OSMesg make_event(unsigned type, unsigned value) {
    uintptr_t p = (value << 8) | type;
    return (OSMesg)p;
}

// Get the event type for an event.
static unsigned event_type(OSMesg evt) {
    return (uintptr_t)evt & 0xff;
}

// Get the associated value for an event.
static unsigned event_value(OSMesg evt) {
    return (uintptr_t)evt >> 8;
}

// State of the main thread.
struct main_state {
    // Whether each task is running.
    bool task_running[2];
    // Whether each framebuffer is being used.
    bool framebuffer_in_use[2];
    // Whether a controller read is in progress.
    bool controler_read_active;
    // The tasks.
    struct scheduler_task tasks[2];
    // Whether we have a controller connected.
    bool has_controller;
    // Which controller is connected.
    int controller_index;

    // Elapsed time on RCP.
    int rcp_time;

    // Queue receiving scheduler / controller events.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[16];

    // SI event queue. Must be a separate queue, because osContRead will read
    // from it.
    OSMesgQueue si_queue;
    OSMesg si_buffer[1];
};

// Read the next event sent to the main thread and process it.
static int process_event(struct main_state *restrict st, int flags) {
    OSMesg evt;
    int ret = osRecvMesg(&st->evt_queue, &evt, flags);
    if (ret != 0)
        return ret;
    int type = event_type(evt);
    int value = event_value(evt);
    switch (type) {
    case EVT_TASKDONE:
        st->task_running[value] = false;
        st->rcp_time = st->tasks[value].runtime;
        break;
    case EVT_FBDONE:
        st->framebuffer_in_use[value] = false;
        break;
    default:
        fatal_error("Bad message type: %d", type);
    }
    return 0;
}

static struct main_state main_state;
static struct game_state game_state;

static void main(void *arg) {
    (void)arg;
    struct main_state *st = &main_state;

    mem_init();
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
    game_init(&game_state);

    scheduler_start(&scheduler, PRIORITY_SCHEDULER, 1);
    int frame_num = 0;
    const bool is_pal = osTvType == OS_TV_PAL;
    for (int current_task = 0;; current_task ^= 1) {
        console_init(&console, CONSOLE_TRUNCATE);
        console_printf(&console, "Frame %d\n", frame_num);
        console_printf(&console, "Time: %5.1f ms\n",
                       (double)st->rcp_time * (1.0e3 / OS_CPU_COUNTER));
        frame_num++;
        // Wait until the task and framebuffer are both free to use.
        while (st->task_running[current_task])
            process_event(st, OS_MESG_BLOCK);
        while (st->framebuffer_in_use[current_task])
            process_event(st, OS_MESG_BLOCK);
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
                game_input(&game_state,
                           &controller_state[st->controller_index]);
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
            game_update(&game_state, dt);
        }

        // Create up display lists.
        u64 *data_ptr;
        size_t data_size;
        {
            Gfx *dl_start = display_lists[current_task];
            Gfx *dl_end = display_lists[current_task] +
                          ARRAY_COUNT(display_lists[current_task]);
            Mtx *mtx_start = matrixes[current_task];
            Mtx *mtx_end =
                matrixes[current_task] + ARRAY_COUNT(matrixes[current_task]);
            struct graphics gr = {
                .dl_ptr = dl_start,
                .dl_start = dl_start,
                .dl_end = dl_end,
                .mtx_ptr = mtx_start,
                .mtx_start = mtx_start,
                .mtx_end = mtx_end,
                .framebuffer = framebuffers[current_task],
                .zbuffer = zbuffer,
                .is_pal = is_pal,
            };
            game_render(&game_state, &gr);
            data_ptr = (u64 *)dl_start;
            data_size = sizeof(*dl_start) * (gr.dl_ptr - dl_start);
            osWritebackDCache(data_ptr, data_size);
            osWritebackDCache(mtx_start,
                              sizeof(*mtx_start) * (gr.mtx_ptr - mtx_start));
        }

        // Submit RCP task.
        struct scheduler_task *task = &st->tasks[current_task];
        *task = (struct scheduler_task){
            .flags = SCHEDULER_TASK_VIDEO | SCHEDULER_TASK_FRAMEBUFFER,
            .task = {{
                .type = M_GFXTASK,
                .flags = OS_TASK_DP_WAIT,
                .ucode_boot = (u64 *)rspbootTextStart,
                .ucode_boot_size =
                    (uintptr_t)rspbootTextEnd - (uintptr_t)rspbootTextStart,
                .ucode_data = (u64 *)gspF3DEX2_xbusDataStart,
                .ucode_data_size = SP_UCODE_DATA_SIZE,
                .ucode = (u64 *)gspF3DEX2_xbusTextStart,
                .ucode_size = SP_UCODE_SIZE,
                .dram_stack = sp_dram_stack,
                .dram_stack_size = sizeof(sp_dram_stack),
                // No output_buff.
                .data_ptr = data_ptr,
                .data_size = data_size,
            }},
            .done_queue = &st->evt_queue,
            .done_mesg = make_event(EVT_TASKDONE, current_task),
            .data =
                {
                    .framebuffer =
                        {
                            .ptr = framebuffers[current_task],
                            .done_queue = &st->evt_queue,
                            .done_mesg = make_event(EVT_FBDONE, current_task),
                        },
                },
        };
        scheduler_submit(&scheduler, task);
        st->task_running[current_task] = true;
        st->framebuffer_in_use[current_task] = true;
    }
}
