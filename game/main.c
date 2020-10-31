#include "game/defs.h"

#include "assets/assets.h"
#include "base/random.h"

#include <ultra64.h>

#include <stdint.h>

#define ARRAY_COUNT(x) (sizeof((x)) / sizeof((x)[0]))

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,

    // Size of the framebuffer, in pixels.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 240,
};

// Stacks (defined in linker script).
extern u8 _main_thread_stack[];
extern u8 _idle_thread_stack[];

// Handle to access ROM data, from osCartRomInit.
static OSPiHandle *rom_handle;

static OSThread idle_thread;
static OSThread main_thread;

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

static OSMesgQueue dma_message_queue;
static OSMesg dma_message_buffer;
static OSIoMesg dma_io_message_buffer;

static OSMesgQueue retrace_message_queue;
static OSMesg retrace_message_buffer;

static OSMesgQueue rdp_message_queue;
static OSMesg rdp_message_buffer;

static u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((aligned(16)));

// Idle thread. Creates other threads then drops to lowest priority.
static void idle(void *arg);

// Main game thread.
static void main(void *arg);

// Main N64 entry point. This must be declared extern.
void boot(void);

void boot(void) {
    osInitialize();
    rom_handle = osCartRomInit();
    osCreateThread(&idle_thread, 1, idle, NULL, _idle_thread_stack, 10);
    osStartThread(&idle_thread);
}

static void idle(void *arg) {
    (void)arg;

    // Initialize video.
    osCreateViManager(OS_PRIORITY_VIMGR);
    osViSetMode(&osViModeTable[OS_VI_NTSC_LPN1]);

    // Initialize peripheral manager.
    osCreatePiManager(OS_PRIORITY_PIMGR, &pi_message_queue, pi_message_buffer,
                      PI_MSG_COUNT);

    // Start main thread.
    osCreateThread(&main_thread, 3, main, NULL, _main_thread_stack, 10);
    osStartThread(&main_thread);

    // Idle loop.
    osSetThreadPri(0, 0);
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

// Clear the color framebuffer.
static Gfx clearframebuffer_dl[] = {
    gsDPSetCycleType(G_CYC_FILL),
    gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                      framebuffers[0]),
    gsDPPipeSync(),
    // This must be in array position [3] because we modify it.
    gsDPSetFillColor(0),
    gsDPFillRectangle(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
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
// static u64 rdp_output[RDP_OUTPUT_LEN] __attribute__((section("uninit.rsp"));

static OSTask tlist = {{
    .type = M_GFXTASK,
    .flags = OS_TASK_DP_WAIT,
    .ucode = (u64 *)gspF3DEX2_xbusTextStart,
    .ucode_size = SP_UCODE_SIZE,
    .ucode_data = (u64 *)gspF3DEX2_xbusDataStart,
    .ucode_data_size = SP_UCODE_DATA_SIZE,
    .dram_stack = sp_dram_stack,
    .dram_stack_size = sizeof(sp_dram_stack),
    // .output_buff = rdp_output + RDP_OUTPUT_LEN,
    // Refer to osSpTaskStart docs... this is a pointer past the edn.
    // .output_buff_size = sizeof(rdp_output),
}};

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
    VEL_BASE = 2,
    VEL_RAND = 2,
};

struct ball {
    int x;
    int y;
    int vx;
    int vy;
};

static struct rand game_rand;
static struct ball balls[NUM_BALLS];

static void game_init(void) {
    OSTime time = osGetTime();
    rand_init(&game_rand, time, 0x243F6A88); // Pi fractional digits.
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i] = (struct ball){
            .x = rand_range_fast(&game_rand, MIN_X, MAX_X),
            .y = rand_range_fast(&game_rand, MIN_Y, MAX_Y),
            .vx = rand_range_fast(&game_rand, VEL_BASE, VEL_BASE + VEL_RAND),
            .vy = rand_range_fast(&game_rand, VEL_BASE, VEL_BASE + VEL_RAND),
        };
        unsigned dir = rand_next(&game_rand);
        if ((dir & 1) != 0) {
            balls[i].vx = -balls[i].vx;
        }
        if ((dir & 2) != 0) {
            balls[i].vy = -balls[i].vy;
        }
    }
}

static void game_update(void) {
    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &balls[i];
        b->x += b->vx;
        b->y += b->vy;
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

static Gfx *game_render(Gfx *dl) {
    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &balls[i];
        int x = b->x - BALL_SIZE / 2;
        int y = b->y - BALL_SIZE / 2;
        gSPTextureRectangle(dl++, x << 2, y << 2, (x + BALL_SIZE - 1) << 2,
                            (y + BALL_SIZE - 1) << 2, 0, 0, 0, 4 << 10,
                            1 << 10);
    }
    return dl;
}

static Gfx display_list[1024];

static void main(void *arg) {
    (void)arg;

    // Set up message queues.
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);
    osCreateMesgQueue(&rdp_message_queue, &rdp_message_buffer, 1);
    osSetEventMesg(OS_EVENT_DP, &rdp_message_queue, NULL);
    osCreateMesgQueue(&retrace_message_queue, &retrace_message_buffer, 1);
    osViSetEvent(&retrace_message_queue, NULL, 1); // 1 = every frame

    // Load the pak header.
    load_pak_data(pak_objects, 0, sizeof(pak_objects));
    asset_load(img_cat, IMG_CAT);
    asset_load(img_ball, IMG_BALL);
    font_load(FONT_GG);

    game_init();

    int which_framebuffer = 0;
    tlist.t.ucode_boot = (u64 *)rspbootTextStart;
    tlist.t.ucode_boot_size =
        (uintptr_t)rspbootTextEnd - (uintptr_t)rspbootTextStart;

    for (;;) {
        game_update();

        // Set up display lists.
        Gfx *glist_start = display_list, *glistp = display_list;
        gSPSegment(glistp++, 0, 0);
        gSPDisplayList(glistp++, rdpinit_dl);
        gSPDisplayList(glistp++, rspinit_dl);
        clearframebuffer_dl[1] =
            (Gfx)gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                                   framebuffers[which_framebuffer]);
        clearframebuffer_dl[3] = (Gfx)gsDPSetFillColor(0);
        gSPDisplayList(glistp++, clearframebuffer_dl);
        gSPDisplayList(glistp++, sprite_dl);
        gDPPipeSync(glistp++);
        glistp = game_render(glistp);
        glistp = text_render(glistp, 10, SCREEN_HEIGHT - 15, "My cool game!");
        gDPFullSync(glistp++);
        gSPEndDisplayList(glistp++);

        osWritebackDCache(&clearframebuffer_dl[1], sizeof(Gfx) * 3);
        osWritebackDCache(glist_start,
                          sizeof(*glist_start) * (glistp - glist_start));
        tlist.t.data_ptr = (u64 *)glist_start;
        tlist.t.data_size = sizeof(*glist_start) * (glistp - glist_start);

        osSpTaskStart(&tlist);
        osRecvMesg(&rdp_message_queue, NULL, OS_MESG_BLOCK);

        osViSwapBuffer(framebuffers[which_framebuffer]);
        // Remove any old retrace message and wait for a new one.
        if (MQ_IS_FULL(&retrace_message_queue))
            osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        which_framebuffer ^= 1;
    }
}
