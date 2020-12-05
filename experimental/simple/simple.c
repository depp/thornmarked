#include "experimental/simple/simple.h"

#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/os.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,
};

// Stacks (defined in linker script).
extern u8 _main_thread_stack[];
extern u8 _idle_thread_stack[];

OSThread idle_thread;
static OSThread main_thread;

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

static OSMesg main_message_buffer[16];
static OSMesgQueue main_message_queue;

u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.cfb"), aligned(16)));

// Idle thread. Creates other threads then drops to lowest priority.
static void idle(void *arg);

// Main game thread.
static void main(void *arg);

// Main N64 entry point. This must be declared extern.
void boot(void);

enum {
    PRIORITY_IDLE = OS_PRIORITY_IDLE,
    PRIORITY_IDLE_INIT = 10,
    PRIORITY_MAIN = 10,
};

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

// Viewport scaling parameters.
static const Vp viewport = {{
    .vscale = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
    .vtrans = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
}};

static const Gfx init_dl[] = {
    // Initialize the RSP.
    gsSPViewport(&viewport),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH | G_FOG |
                          G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR |
                          G_LOD),
    gsSPTexture(0, 0, 0, 0, G_OFF),

    // Initialize the RDP.
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPPipeSync(),

    // Fill the color framebuffer.
    gsDPSetCycleType(G_CYC_FILL),
    gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                      SEGMENT_ADDR(1, 0)),
    gsDPSetFillColor((GPACK_RGBA5551(49, 155, 181, 1) << 16) |
                     GPACK_RGBA5551(49, 155, 181, 1)),
    gsDPFillRectangle(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),

    gsSPEndDisplayList(),
};

enum {
    SP_STACK_SIZE = 1024,
};

static u64 sp_dram_stack[SP_STACK_SIZE / 8] __attribute__((section("uninit")));

enum {
    EVENT_NONE,
    EVENT_RDP,
    EVENT_RETRACE,
};

static inline OSMesg make_event(int type) {
    return (OSMesg)(uintptr_t)type;
}

static Gfx display_list[1024];
static OSTask task;

static void main(void *arg) {
    (void)arg;
    program_init();

    // Set up message queues.
    osCreateMesgQueue(&main_message_queue, main_message_buffer,
                      ARRAY_COUNT(main_message_buffer));

    osSetEventMesg(OS_EVENT_DP, &main_message_queue, make_event(EVENT_RDP));
    osViSetEvent(&main_message_queue, make_event(EVENT_RETRACE),
                 3); // Every 3rd frame

    int which_buffer = 0;
    bool task_active = false;
    for (;;) {
        OSMesg mesg;
        osRecvMesg(&main_message_queue, &mesg, 0);
        switch ((uintptr_t)mesg) {
        case EVENT_RDP:
            if (task_active) {
                osViSwapBuffer(framebuffers[which_buffer]);
                osViBlack(false);
                task_active = false;
                which_buffer ^= 1;
            }
            break;
        case EVENT_RETRACE:
            if (!task_active) {
                Gfx *dl_start = display_list;
                Gfx *dl_end = display_list + ARRAY_COUNT(display_list);
                Gfx *dl = dl_start;
                gSPSegment(dl++, 0, 0);
                gSPSegment(dl++, 1, framebuffers[which_buffer]);
                gSPDisplayList(dl++, init_dl);
                dl = program_render(dl, dl_end);
                gDPFullSync(dl++);
                gSPEndDisplayList(dl++);
                osWritebackDCache(dl_start,
                                  sizeof(*dl_start) * (dl - dl_start));
                task = (OSTask){{
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
                    .data_ptr = (u64 *)dl_start,
                    .data_size = sizeof(*dl_start) * (dl - dl_start),
                }};
                osSpTaskStart(&task);
                task_active = true;
            }
            break;
        }
    }
}
