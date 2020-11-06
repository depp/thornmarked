#include "game/defs.h"

#include "base/console/console.h"
#include "base/console/n64.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

extern u8 _idle_thread_stack[];

static void fatal_thread(void *arg);

noreturn void fatal_error(const char *fmt, ...) {
    struct console *cs = &console;
    console_init(cs);
    console_puts(cs, "The game has crashed! :-(\n");
    va_list ap;
    va_start(ap, fmt);
    console_vprintf(cs, fmt, ap);
    va_end(ap);

    // Usurp the idle thread and turn it into a maximum-priority thread to
    // display the fault message.
    osStopThread(&idle_thread);
    osDestroyThread(&idle_thread);

    osCreateThread(&idle_thread, 1, fatal_thread, cs, _idle_thread_stack,
                   OS_PRIORITY_APPMAX);
    osStartThread(&idle_thread);
    osStopThread(NULL);
    __builtin_unreachable();
}

static void fatal_thread(void *arg) {
    struct console *cs = arg;

    uint16_t *fb = framebuffers[0];
    osViSetMode(&osViModeNtscLpn1);
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
    osViBlack(false);
    osViSwapBuffer(fb);

    for (;;) {
        console_draw_raw(cs, fb);
        osWritebackDCache(fb, sizeof(uint16_t) * SCREEN_HEIGHT * SCREEN_WIDTH);

        OSTime start = osGetTime();
        while (osGetTime() - start < OS_CPU_COUNTER / 10) {}
    }
}

noreturn void assert_fail(const char *file, int line, const char *pred) {
    fatal_error("\nAssertion failed\n%s:%d\n%s", file, line, pred);
}
