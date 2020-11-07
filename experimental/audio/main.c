#include "base/console.h"
#include "base/console_n64.h"
#include "base/defs.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,

    // Size of framebuffer.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 240,
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
// static OSIoMesg dma_io_message_buffer;

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

enum {
    SP_STACK_SIZE = 1024,
};

static u64 sp_dram_stack[SP_STACK_SIZE / 8] __attribute__((section("uninit")));

enum {
    EVENT_CONTROLLER,
    EVENT_RDP,
    EVENT_RETRACE,
};

static inline OSMesg make_event(int type) {
    return (OSMesg)(uintptr_t)type;
}

static unsigned init_controllers(void) {
    u8 mask;
    OSContStatus status[MAXCONTROLLERS];
    osContInit(&main_message_queue, &mask, status);
    unsigned result;
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        if ((mask & (1u << i)) != 0 && status[i].errno == 0 &&
            (status[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL) {
            result |= 1u << i;
        }
    }
    return result;
}

enum {
    BUTTON_SELECT = 01,
    BUTTON_CANCEL = 02,
    BUTTON_DOWN = 04,
    BUTTON_UP = 010,
};

static unsigned get_controllers(unsigned mask) {
    unsigned state = 0;
    OSContPad pad[MAXCONTROLLERS];
    osContGetReadData(pad);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        if ((mask & (1u << i)) == 0) {
            continue;
        }
        if (pad[i].stick_y > 64) {
            state |= BUTTON_UP;
        } else if (pad[i].stick_y < -64) {
            state |= BUTTON_DOWN;
        }
        if ((pad[i].button & CONT_DOWN) != 0) {
            state |= BUTTON_DOWN;
        }
        if ((pad[i].button & CONT_UP) != 0) {
            state |= BUTTON_UP;
        }
        if ((pad[i].button & A_BUTTON) != 0) {
            state |= BUTTON_SELECT;
        }
        if ((pad[i].button & B_BUTTON) != 0) {
            state |= BUTTON_CANCEL;
        }
    }
    unsigned m = BUTTON_SELECT | BUTTON_CANCEL;
    if ((state & m) == m) {
        state &= ~m;
    }
    m = BUTTON_DOWN | BUTTON_UP;
    if ((state & m) == m) {
        state &= ~m;
    }
    return state;
}

static Gfx display_list[1024];
static OSTask task;

static unsigned rgb16(unsigned r, unsigned g, unsigned b) {
    unsigned c = ((r << 8) & (31u << 11)) | ((g << 3) & (31u << 3)) |
                 ((b >> 2) & (31u << 1)) | 1;
    return (c << 16) | c;
}

static void main(void *arg) {
    (void)arg;

    // Set up message queues.
    osCreateMesgQueue(&main_message_queue, main_message_buffer,
                      ARRAY_COUNT(main_message_buffer));
    osSetEventMesg(OS_EVENT_SI, &main_message_queue,
                   make_event(EVENT_CONTROLLER));
    unsigned cont_mask = init_controllers();

    osSetEventMesg(OS_EVENT_DP, &main_message_queue, make_event(EVENT_RDP));
    osViSetEvent(&main_message_queue, make_event(EVENT_RETRACE),
                 3); // Every 3rd frame
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);

    int frame_num = 0;
    int which_buffer = 0;
    bool controller_read_active = false;
    bool task_active = false;
    unsigned cont_state = 0;
    for (;;) {
        OSMesg mesg;
        osRecvMesg(&main_message_queue, &mesg, 0);
        switch ((uintptr_t)mesg) {
        case EVENT_CONTROLLER:
            controller_read_active = false;
            cont_state = get_controllers(cont_mask);
            break;
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
                if (!controller_read_active) {
                    osContStartReadData(&main_message_queue);
                    controller_read_active = true;
                }
                frame_num++;
                console_init(&console);
                console_printf(&console, "Frame %d\n", frame_num);
                console_printf(&console, "Controller: 0x%04x\n", cont_state);
                Gfx *dl_start = display_list;
                Gfx *dl = dl_start;
                gSPSegment(dl++, 0, 0);
                gSPDisplayList(dl++, rspinit_dl);
                gSPDisplayList(dl++, rdpinit_dl);
                gDPSetCycleType(dl++, G_CYC_FILL);
                gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b,
                                 SCREEN_WIDTH, framebuffers[which_buffer]);
                gDPPipeSync(dl++);
                unsigned color = rgb16(49, 155, 181);
                gDPSetFillColor(dl++, color);
                gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1,
                                 SCREEN_HEIGHT - 1);
                dl = console_draw_displaylist(&console, dl);
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
