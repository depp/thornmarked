#include <ultra64.h>

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

static OSMesgQueue retrace_message_queue;
static OSMesg retrace_message_buffer;

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

// Fill a framebuffer with the given color, whuch must be in 16-bit RGBA format.
static void fill_framebuffer(u16 (*framebuffer)[SCREEN_WIDTH * SCREEN_HEIGHT],
                             unsigned color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        (*framebuffer)[i] = color;
    }
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

static void main(void *arg) {
    (void)arg;

    // Set up message queues.
    osCreateMesgQueue(&retrace_message_queue, &retrace_message_buffer, 1);
    osViSetEvent(&retrace_message_queue, NULL, 1); // 1 = every frame

    int which_framebuffer = 0;
    unsigned hue = 0;

    for (;;) {
        unsigned color = make_hue(hue);
        hue++;
        if (hue == 6 << 5)
            hue = 0;

        fill_framebuffer(&framebuffers[which_framebuffer], color);
        osWritebackDCache(&framebuffers[which_framebuffer],
                          sizeof(framebuffers[which_framebuffer]));

        osViSwapBuffer(framebuffers[which_framebuffer]);
        // Remove any old retrace message and wait for a new one.
        if (MQ_IS_FULL(&retrace_message_queue))
            osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        osRecvMesg(&retrace_message_queue, NULL, OS_MESG_BLOCK);
        which_framebuffer ^= 1;
    }
}
