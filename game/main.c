#include <ultra64.h>

#include <stdint.h>

#include "assets/assets.h"

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

static u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT];

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

// Create a 16-bit RGBA color from components. Color components must be 5-bit,
// and the alpha component must be 1-bit.
static unsigned rgba16(unsigned r, unsigned g, unsigned b, unsigned a) {
    unsigned c = 0;
    c |= (r & 31) << 11;
    c |= (g & 31) << 6;
    c |= (b & 31) << 1;
    c |= a & 1;
    return c;
}

// Create a fully saturated color from a hue. The hue goes from 0 to 6<<5.
static unsigned make_hue(unsigned hue) {
    unsigned r, g, b;
    switch (hue >> 5) {
    default:
    case 0:
        r = 31;
        g = hue;
        b = 0;
        break;
    case 1:
        r = ~hue;
        g = 31;
        b = 0;
        break;
    case 2:
        r = 0;
        g = 31;
        b = hue;
        break;
    case 3:
        r = 0;
        g = ~hue;
        b = 31;
        break;
    case 4:
        r = hue;
        g = 0;
        b = 31;
        break;
    case 5:
        r = 31;
        g = 0;
        b = ~hue;
        break;
    }
    return rgba16(r, g, b, 0);
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

static uint16_t img_cat[32 * 32] __attribute__((unused, aligned(16)));
static uint16_t img_ball[32 * 32] __attribute__((unused, aligned(16)));

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
    gsSPTextureRectangle(40 << 2, 10 << 2, (40 + 32 - 1) << 2,
                         (10 + 32 - 1) << 2, 0, 0, 0, 4 << 10, 1 << 10),
    gsDPPipeSync(),
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

static void load_pak_object(void *dest, int index) {
    struct pak_object obj = pak_objects[index - 1];
    load_pak_data(dest, sizeof(pak_objects) + obj.offset, obj.size);
}

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
    load_pak_object(img_cat, IMG_CAT);
    load_pak_object(img_ball, IMG_BALL);

    int which_framebuffer = 0;
    unsigned hue = 0;
    tlist.t.ucode_boot = (u64 *)rspbootTextStart;
    tlist.t.ucode_boot_size =
        (uintptr_t)rspbootTextEnd - (uintptr_t)rspbootTextStart;

    for (;;) {
        // Set up display lists.

        unsigned color = make_hue(hue);
        hue++;
        if (hue == 6 << 5)
            hue = 0;

        Gfx glist[16], *glistp = glist;
        gSPSegment(glistp++, 0, 0);
        gSPDisplayList(glistp++, rdpinit_dl);
        gSPDisplayList(glistp++, rspinit_dl);
        clearframebuffer_dl[1] =
            (Gfx)gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                                   framebuffers[which_framebuffer]);
        clearframebuffer_dl[3] = (Gfx)gsDPSetFillColor(color | (color << 16));
        gSPDisplayList(glistp++, clearframebuffer_dl);
        gSPDisplayList(glistp++, sprite_dl);
        gDPFullSync(glistp++);
        gSPEndDisplayList(glistp++);

        osWritebackDCache(&clearframebuffer_dl[1], sizeof(Gfx) * 3);
        osWritebackDCache(glist, sizeof(*glist) * (glistp - glist));
        tlist.t.data_ptr = (u64 *)glist;
        tlist.t.data_size = sizeof(*glist) * (glistp - glist);

        osSpTaskStart(&tlist);
        osRecvMesg(&rdp_message_queue, NULL, OS_MESG_BLOCK);

        // Copy a version of the txture with the CPU for reference.
        u16 *restrict pixels = (u16 *)img_ball;
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                framebuffers[which_framebuffer][y * SCREEN_WIDTH + x] =
                    pixels[y * 32 + x];
            }
        }
        osWritebackDCacheAll();

        osViSwapBuffer(framebuffers[which_framebuffer]);
        // Remove any old retrace message and wait for a new one.
        if (MQ_IS_FULL(&retrace_message_queue))
            osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        which_framebuffer ^= 1;
    }
}
