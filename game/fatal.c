#include "game/defs.h"

#include "assets/fonts/terminus/ter-u12n.h"

#include <stdint.h>

extern u8 _idle_thread_stack[];

static void fatal_thread(void *arg);

noreturn void fatal_error(void) {
    // Usurp the idle thread and turn it into a maximum-priority thread to
    // display the fault message.
    osStopThread(&idle_thread);
    osDestroyThread(&idle_thread);
    osCreateThread(&idle_thread, 1, fatal_thread, NULL, _idle_thread_stack,
                   OS_PRIORITY_APPMAX);
    osStartThread(&idle_thread);
    osStopThread(NULL);
    __builtin_unreachable();
}

static void fatal_thread(void *arg) {
    (void)arg;
    uint16_t *fb = framebuffers[0];
    osViSetMode(&osViModeNtscLpn1);
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
    osViSwapBuffer(fb);

    const char *text = "The game has crashed!";

    for (;;) {
        for (int i = 0; text[i] != '\0'; i++) {
            unsigned c = (unsigned char)text[i];
            if (c >= FONT_NCHARS) {
                c = 0;
            }
            for (int y = 0; y < FONT_HEIGHT; y++) {
                uint16_t *optr =
                    &fb[(y + 16) * SCREEN_WIDTH + i * FONT_WIDTH + 16];
                uint32_t idata = FONT_DATA[c * FONT_HEIGHT + y];
                for (int x = 0; x < FONT_WIDTH; x++) {
                    optr[x] = (idata & (1u << x)) != 0 ? 0xffff : 0x0000;
                }
            }
        }
        osWritebackDCache(fb, sizeof(u16) * SCREEN_HEIGHT * SCREEN_WIDTH);

        OSTime start = osGetTime();
        while (osGetTime() - start < OS_CPU_COUNTER / 10) {}
    }
}
