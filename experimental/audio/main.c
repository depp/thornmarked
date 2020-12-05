#include "experimental/audio/defs.h"

#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/os.h"
#include "base/n64/scheduler.h"
#include "base/n64/system.h"
#include "base/pak/pak.h"
#include "experimental/audio/pak.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

OSMesg event_pack(struct event_data evt);
struct event_data event_unpack(OSMesg mesg);

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

OSThread idle_thread;
static OSThread main_thread;

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

static OSMesg cont_message_buffer[1];
static OSMesgQueue cont_message_queue;

u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.cfb"), aligned(16)));

// Idle thread. Creates other threads then drops to lowest priority.
static void idle(void *arg);

// Main game thread.
static void main(void *arg);

// Main N64 entry point. This must be declared extern.
void boot(void);

void boot(void) {
    osInitialize();
    fatal_init();
    thread_create(&idle_thread, idle, NULL, _idle_thread_stack,
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
    thread_create(&main_thread, main, NULL, _main_thread_stack, PRIORITY_MAIN);
    osStartThread(&main_thread);

    // Idle loop.
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
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

static unsigned init_controllers(OSMesgQueue *mesgq) {
    u8 mask;
    OSContStatus status[MAXCONTROLLERS];
    osContInit(mesgq, &mask, status);
    unsigned result = 0;
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

static Gfx display_list[4 * 1024];
static struct scheduler_task video_task;
static struct scheduler scheduler;

static unsigned rgb16(unsigned r, unsigned g, unsigned b) {
    unsigned c = ((r << 8) & (31u << 11)) | ((g << 3) & (31u << 3)) |
                 ((b >> 2) & (31u << 1)) | 1;
    return (c << 16) | c;
}

struct main_state {
    unsigned cont_mask;
    unsigned cont_state;
    bool controller_read_active;
    bool task_active;
    bool framebuffer_active[2];
    struct audio_state audio;
    OSMesg message_buffer[16];
    OSMesgQueue message_queue;
};

static int main_event(struct main_state *st, int mode) {
    OSMesg mesg = NULL;
    int r = osRecvMesg(&st->message_queue, &mesg, mode);
    if (r != 0) {
        return r;
    }
    struct event_data evt = event_unpack(mesg);
    switch (evt.type) {
    case EVENT_VTASKDONE:
        st->task_active = false;
        break;
    case EVENT_VBUFDONE:
        st->framebuffer_active[evt.value] = false;
        break;
    case EVENT_AUDIO:;
        st->audio.busy &= ~(unsigned)evt.value;
        break;
    default:
        console_fatal(&console, "Bad event: %p", &mesg);
    }
    return 0;
}

static struct main_state main_state;

// Info for the pak objects, to be loaded from cartridge.
struct pak_object pak_objects[PAK_SIZE] __attribute__((aligned(16)));

static void main(void *arg) {
    (void)arg;

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        framebuffers[0][i] = 0xffff;
    }
    osWritebackDCache(framebuffers[0], sizeof(framebuffers[0]));

    mem_init();
    pak_init(PAK_SIZE);
    audio_init();

    struct main_state *st = &main_state;

    // Set up message queues.
    osCreateMesgQueue(&cont_message_queue, cont_message_buffer,
                      ARRAY_COUNT(cont_message_buffer));
    osCreateMesgQueue(&st->message_queue, st->message_buffer,
                      ARRAY_COUNT(st->message_buffer));
    osSetEventMesg(OS_EVENT_SI, &cont_message_queue, NULL);
    st->cont_mask = init_controllers(&cont_message_queue);

    scheduler_start(&scheduler, 3);

    int which_vbuffer = 0;
    struct console *cs = &console;
    int frame_number = 0;
    for (;;) {
        while ((st->task_active || st->framebuffer_active[which_vbuffer]) &&
               (st->audio.busy & st->audio.wait) != 0) {
            main_event(st, OS_MESG_BLOCK);
        }
        while (main_event(st, OS_MESG_NOBLOCK) == 0) {}
        if ((st->audio.busy & st->audio.wait) == 0) {
            audio_frame(&st->audio, &scheduler, &st->message_queue);
        }
        if (!st->task_active && !st->framebuffer_active[which_vbuffer]) {
            console_init(cs, CONSOLE_TRUNCATE);
            console_printf(&console, "Frame %d\n", frame_number);
            frame_number++;
            if (osRecvMesg(&cont_message_queue, NULL, OS_MESG_NOBLOCK) == 0) {
                st->controller_read_active = false;
                st->cont_state = get_controllers(st->cont_mask);
            }
            if (!st->controller_read_active) {
                osContStartReadData(&cont_message_queue);
                st->controller_read_active = true;
            }
            console_printf(cs, "Controller %02x\n", st->cont_state);
            Gfx *dl_start = display_list;
            Gfx *dl_end = display_list + ARRAY_COUNT(display_list);
            Gfx *dl = dl_start;
            gSPSegment(dl++, 0, 0);
            gSPDisplayList(dl++, rspinit_dl);
            gSPDisplayList(dl++, rdpinit_dl);
            gDPSetCycleType(dl++, G_CYC_FILL);
            gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                             framebuffers[which_vbuffer]);
            gDPPipeSync(dl++);
            unsigned color = rgb16(49, 155, 181);
            gDPSetFillColor(dl++, color);
            gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
            dl = console_draw_displaylist(&console, dl, dl_end);
            if (2 > dl_end - dl) {
                fatal_dloverflow();
            }
            gDPFullSync(dl++);
            gSPEndDisplayList(dl++);
            osWritebackDCache(dl_start, sizeof(*dl_start) * (dl - dl_start));
            video_task = (struct scheduler_task){
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
                    .data_ptr = (u64 *)dl_start,
                    .data_size = sizeof(*dl_start) * (dl - dl_start),
                }},
                .done_queue = &st->message_queue,
                .done_mesg =
                    event_pack((struct event_data){.type = EVENT_VTASKDONE}),
                .data =
                    {
                        .framebuffer =
                            {
                                .ptr = framebuffers[which_vbuffer],
                                .done_queue = &st->message_queue,
                                .done_mesg = event_pack((struct event_data){
                                    .type = EVENT_VBUFDONE,
                                    .value = which_vbuffer,
                                }),
                            },
                    },
            };
            scheduler_submit(&scheduler, &video_task);
            st->task_active = true;
            st->framebuffer_active[which_vbuffer] = true;
            which_vbuffer ^= 1;
        }
    }
}
