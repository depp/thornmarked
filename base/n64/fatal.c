#include "base/n64/system.h"

#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/os.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

static const char *FATAL_MESSAGE = "The game has crashed :-(\n";

static OSThread fatal_thread;
static u8 fatal_thread_stack[256]
    __attribute__((section("uninit"), aligned(16)));

static void fatal_thread_func(void *arg);

static noreturn void console_vfatal_impl(struct console *cs, const char *fmt,
                                         va_list ap) {
    if (cs == NULL) {
        cs = &console;
        console_init(cs, CONSOLE_TRUNCATE);
    } else {
        console_newline(cs);
    }
    console_puts(cs, FATAL_MESSAGE);
    console_vprintf(cs, fmt, ap);
    thread_create(&fatal_thread, fatal_thread_func, cs,
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
    {
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
        osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
        osViBlack(false);
        osViSwapBuffer(fb);
    }

    for (;;) {
        console_draw_raw(cs, fb);
        osWritebackDCache(fb, sizeof(uint16_t) * SCREEN_HEIGHT * SCREEN_WIDTH);

        OSTime start = osGetTime();
        while (osGetTime() - start < OS_CPU_COUNTER / 10) {}
    }
}

noreturn void fatal_dloverflow_func(const char *file, int line) {
    fatal_error("Display list overflow\n%s:%d", file, line);
}

void fatal_init(void) {
    console_vfatal_func = console_vfatal_impl;
}
