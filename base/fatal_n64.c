#include "base/base.h"

#include "base/console.h"
#include "base/console_n64.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

static OSThread fatal_thread;
static u8 fatal_thread_stack[256]
    __attribute__((section("uninit"), aligned(16)));

static void fatal_thread_func(void *arg);

noreturn void fatal_error(const char *fmt, ...) {
    struct console *cs = &console;
    console_init(cs, CONSOLE_TRUNCATE);
    console_puts(cs, "The game has crashed! :-(\n");
    va_list ap;
    va_start(ap, fmt);
    console_vprintf(cs, fmt, ap);
    va_end(ap);

    osCreateThread(&fatal_thread, 1, fatal_thread_func, cs,
                   fatal_thread_stack + ARRAY_COUNT(fatal_thread_stack),
                   OS_PRIORITY_APPMAX);
    osStartThread(&fatal_thread);
    osStopThread(NULL);
    __builtin_unreachable();
}

static void fatal_thread_func(void *arg) {
    struct console *cs = arg;
    enum {
        SCREEN_WIDTH = 320,
        SCREEN_HEIGHT = 240,
    };

    uint16_t *fb = (uint16_t *)(uintptr_t)0x80300000;
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
