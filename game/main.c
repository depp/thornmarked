#include "game/defs.h"

#include "assets/assets.h"
#include "base/random.h"
#include "scheduler.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

#define ARRAY_COUNT(x) (sizeof((x)) / sizeof((x)[0]))

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,
};

// Stacks (defined in linker script).
extern u8 _main_thread_stack[];
extern u8 _idle_thread_stack[];

// Handle to access ROM data, from osCartRomInit.
static OSPiHandle *rom_handle;

OSThread idle_thread;
static OSThread main_thread;

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

static OSMesgQueue dma_message_queue;
static OSMesg dma_message_buffer;
static OSIoMesg dma_io_message_buffer;

u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT] __attribute__((aligned(16)));

// Idle thread. Creates other threads then drops to lowest priority.
static void idle(void *arg);

// Main game thread.
static void main(void *arg);

// Main N64 entry point. This must be declared extern.
void boot(void);

void boot(void) {
    osInitialize();
    rom_handle = osCartRomInit();
    osCreateThread(&idle_thread, 1, idle, NULL, _idle_thread_stack,
                   PRIORITY_IDLE_INIT);
    osStartThread(&idle_thread);
}

static void idle(void *arg) {
    (void)arg;

    // Initialize video.
    osCreateViManager(OS_PRIORITY_VIMGR);
    osViSetMode(&osViModeNtscLpn1);
    osViBlack(1);

    // Initialize peripheral manager.
    osCreatePiManager(OS_PRIORITY_PIMGR, &pi_message_queue, pi_message_buffer,
                      PI_MSG_COUNT);

    // Start main thread.
    osCreateThread(&main_thread, 3, main, NULL, _main_thread_stack,
                   PRIORITY_MAIN);
    osStartThread(&main_thread);

    // Idle loop.
    osSetThreadPri(NULL, PRIORITY_IDLE);
    for (;;) {}
}

// Viewport scaling parameters.
static const Vp viewport = {{
    .vscale = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
    .vtrans = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
}};

// Initialize the RSP.
static const Gfx rspinit_dl[] = {
    gsSPViewport(&viewport),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH | G_FOG |
                          G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR |
                          G_LOD),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPEndDisplayList(),
};

// Initialize the RDP.
static const Gfx rdpinit_dl[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};

static uint16_t img_cat[32 * 32] __attribute__((aligned(16)));
static uint16_t img_ball[32 * 32] __attribute__((aligned(16)));

static Gfx sprite_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCycleType(G_CYC_COPY), // G_CYC_2CYCLE
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGB, G_CC_DECALRGB),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureFilter(G_TF_POINT), // G_TF_BILERP
    gsDPLoadTextureBlock(img_cat, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                         G_TX_NOMIRROR, G_TX_NOMIRROR, 0, 0, G_TX_NOLOD,
                         G_TX_NOLOD),
    gsSPEndDisplayList(),
};

enum {
    SP_STACK_SIZE = 1024,
    RDP_OUTPUT_LEN = 64 * 1024,
};

static u64 sp_dram_stack[SP_STACK_SIZE / 8]
    __attribute__((section("uninit.rsp")));

// Offset in cartridge where data is stored.
extern u8 _pakdata_offset[];

// Descriptor for an object in the pak.
struct pak_object {
    uint32_t offset;
    uint32_t size;
};

// Info for the pak objects, to be loaded from cartridge.
static struct pak_object pak_objects[PAK_SIZE] __attribute__((aligned(16)));

static void load_pak_data(void *dest, uint32_t offset, uint32_t size) {
    osWritebackDCache(dest, size);
    osInvalDCache(dest, size);
    dma_io_message_buffer = (OSIoMesg){
        .hdr =
            {
                .pri = OS_MESG_PRI_NORMAL,
                .retQueue = &dma_message_queue,
            },
        .dramAddr = dest,
        .devAddr = (uint32_t)_pakdata_offset + offset,
        .size = size,
    };
    osEPiStartDma(rom_handle, &dma_io_message_buffer, OS_READ);
    osRecvMesg(&dma_message_queue, NULL, OS_MESG_BLOCK);
    osInvalDCache(dest, size);
}

void asset_load(void *dest, int asset_id) {
    struct pak_object obj = pak_objects[asset_id - 1];
    load_pak_data(dest, sizeof(pak_objects) + obj.offset, obj.size);
}

enum {
    BALL_SIZE = 32,
    NUM_BALLS = 10,
    MARGIN = 10 + BALL_SIZE / 2,
    MIN_X = MARGIN,
    MIN_Y = MARGIN,
    MAX_X = SCREEN_WIDTH - MARGIN - 1,
    MAX_Y = SCREEN_HEIGHT - MARGIN - 1,
    VEL_BASE = 50,
    VEL_RAND = 50,
};

struct ball {
    float x;
    float y;
    float vx;
    float vy;
};

struct game_state {
    // True if the first frame has already passed.
    bool after_first_frame;

    // Timestamp of last frame, low 32 bits.
    uint32_t last_frame_time;

    // Current controller inputs.
    OSContPad controller;

    // True if button is currently pressed.
    bool is_pressed;

    // Background color.
    uint16_t color;

    // Random number generator state.
    struct rand rand;

    // Balls on screen.
    struct ball balls[NUM_BALLS];
};

static void game_init(struct game_state *restrict gs) {
    OSTime time = osGetTime();
    rand_init(&gs->rand, time, 0x243F6A88); // Pi fractional digits.
    for (int i = 0; i < NUM_BALLS; i++) {
        gs->balls[i] = (struct ball){
            .x = rand_frange(&gs->rand, MIN_X, MAX_X),
            .y = rand_frange(&gs->rand, MIN_Y, MAX_Y),
            .vx = rand_frange(&gs->rand, VEL_BASE, VEL_BASE + VEL_RAND),
            .vy = rand_frange(&gs->rand, VEL_BASE, VEL_BASE + VEL_RAND),
        };
        unsigned dir = rand_next(&gs->rand);
        if ((dir & 1) != 0) {
            gs->balls[i].vx = -gs->balls[i].vx;
        }
        if ((dir & 2) != 0) {
            gs->balls[i].vy = -gs->balls[i].vy;
        }
    }
}

static void game_update(struct game_state *restrict gs) {
    bool was_pressed = gs->is_pressed;
    gs->is_pressed = (gs->controller.button & A_BUTTON) != 0;
    if (!was_pressed && gs->is_pressed) {
        gs->color = rand_next(&gs->rand);
    }

    uint32_t cur_time = osGetTime();
    uint32_t delta_time = cur_time - gs->last_frame_time;
    gs->last_frame_time = cur_time;
    if (!gs->after_first_frame) {
        gs->after_first_frame = true;
        return;
    }
    // Clamp to 100ms, in case something gets out of hand.
    const uint32_t MAX_DELTA = OS_CPU_COUNTER / 10;
    if (delta_time > MAX_DELTA) {
        delta_time = MAX_DELTA;
    }
    float dt = (float)((int)delta_time) * (1.0f / (float)OS_CPU_COUNTER);

    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &gs->balls[i];
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        if (b->x > MAX_X) {
            b->x = MAX_X * 2 - b->x;
            b->vx = -b->vx;
        } else if (b->x < MIN_X) {
            b->x = MIN_X * 2 - b->x;
            b->vx = -b->vx;
        }
        if (b->y > MAX_Y) {
            b->y = MAX_Y * 2 - b->y;
            b->vy = -b->vy;
        } else if (b->y < MIN_Y) {
            b->y = MIN_Y * 2 - b->y;
            b->vy = -b->vy;
        }
    }
}

static Gfx *game_render(struct game_state *restrict gs, Gfx *dl) {
    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &gs->balls[i];
        int x = b->x - BALL_SIZE / 2;
        int y = b->y - BALL_SIZE / 2;
        gSPTextureRectangle(dl++, x << 2, y << 2, (x + BALL_SIZE - 1) << 2,
                            (y + BALL_SIZE - 1) << 2, 0, 0, 0, 4 << 10,
                            1 << 10);
    }
    return dl;
}

struct game_state game_state;

static Gfx *render(Gfx *dl, uint16_t *framebuffer) {
    gSPSegment(dl++, 0, 0);
    gSPDisplayList(dl++, rdpinit_dl);
    gSPDisplayList(dl++, rspinit_dl);

    // Clear the color framebuffer.
    gDPSetCycleType(dl++, G_CYC_FILL);
    gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     framebuffer);
    gDPPipeSync(dl++);
    gDPSetFillColor(dl++, game_state.color | (game_state.color << 16));
    gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Render game.
    gSPDisplayList(dl++, sprite_dl);
    gDPPipeSync(dl++);
    dl = game_render(&game_state, dl);
    dl = text_render(dl, 20, SCREEN_HEIGHT - 18, "Scheduler in operation");
    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);
    return dl;
}

static Gfx display_lists[2][1024];
static struct scheduler scheduler;

// Event types for events on the main thread.
enum {
    EVT_CONTROL,  // Controller update.
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

    // Queue receiving scheduler / controller events.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[16];
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
    case EVT_CONTROL:;
        OSContPad controller_state[MAXCONTROLLERS];
        osContGetReadData(controller_state);
        st->controler_read_active = false;
        struct game_state *restrict gs = &game_state;
        if (st->has_controller) {
            gs->controller = controller_state[st->controller_index];
        }
        break;
    case EVT_TASKDONE:
        st->task_running[value] = false;
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

static void main(void *arg) {
    (void)arg;
    struct main_state *st = &main_state;

    // Set up message queues.
    osCreateMesgQueue(&st->evt_queue, st->evt_buffer,
                      ARRAY_COUNT(st->evt_buffer));
    osSetEventMesg(OS_EVENT_SI, &st->evt_queue, NULL);
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);

    // Load the pak header.
    load_pak_data(pak_objects, 0, sizeof(pak_objects));
    asset_load(img_cat, IMG_CAT);
    asset_load(img_ball, IMG_BALL);
    font_load(FONT_GG);

    // Scan for first controller.
    {
        u8 cont_mask;
        OSContStatus cont_status[MAXCONTROLLERS];
        osContInit(&st->evt_queue, &cont_mask, cont_status);
        for (int i = 0; i < MAXCONTROLLERS; i++) {
            if ((cont_mask & (1u << i)) != 0 && cont_status[i].errno == 0 &&
                (cont_status[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL) {
                st->controller_index = i;
                st->has_controller = true;
                break;
            }
        }
    }

    game_init(&game_state);

    scheduler_start(&scheduler);
    int frame_num = 0;
    for (int current_task = 0;; current_task ^= 1) {
        frame_num++;
        if (frame_num == 100) {
            fatal_error(
                "fatal error at frame=%d\n"
                "it works\n"
                "hex = $%x\n"
                "a very long line that must be wrapped because it is so long "
                "so long so very very long",
                frame_num, 12345);
        }
        // Wait until the task and framebuffer are both free to use.
        while (st->task_running[current_task])
            process_event(st, true);
        while (st->framebuffer_in_use[current_task])
            process_event(st, true);
        while (process_event(st, false) == 0) {}

        if (!st->controler_read_active) {
            osContStartQuery(&st->evt_queue);
            st->controler_read_active = true;
        }
        game_update(&game_state);

        // Set up display lists.
        Gfx *dl_start = display_lists[current_task];
        Gfx *dl_end = render(dl_start, framebuffers[current_task]);

        struct scheduler_task *task = &st->tasks[current_task];
        *task = (struct scheduler_task){
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
                .data_ptr = (u64 *)dl_start,
                .data_size = sizeof(*dl_start) * (dl_end - dl_start),
            }},
            .done_queue = &st->evt_queue,
            .done_mesg = make_event(EVT_TASKDONE, current_task),
            .framebuffer =
                {
                    .ptr = framebuffers[current_task],
                    .done_queue = &st->evt_queue,
                    .done_mesg = make_event(EVT_FBDONE, current_task),
                },
        };
        osWritebackDCache(dl_start, sizeof(*dl_start) * (dl_end - dl_start));
        scheduler_submit(&scheduler, task);
        st->task_running[current_task] = true;
        st->framebuffer_in_use[current_task] = true;
    }
}
