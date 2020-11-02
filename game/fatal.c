#include "game/defs.h"

#include "assets/fonts/terminus/ter-u12n.h"

#include <stdarg.h>
#include <stdint.h>

#include <PR/os_libc.h>

extern u8 _idle_thread_stack[];

struct fatal_ctx {
    const char *msg;
    va_list ap;
};

static void fatal_thread(void *arg);

noreturn void fatal_error(const char *msg, ...) {
    // Usurp the idle thread and turn it into a maximum-priority thread to
    // display the fault message.
    osStopThread(&idle_thread);
    osDestroyThread(&idle_thread);
    va_list ap;
    va_start(ap, msg);
    struct fatal_ctx ctx = {.msg = msg, .ap = ap};
    osCreateThread(&idle_thread, 1, fatal_thread, &ctx, _idle_thread_stack,
                   OS_PRIORITY_APPMAX);
    osStartThread(&idle_thread);
    osStopThread(NULL);
    __builtin_unreachable();
}

enum {
    XMARGIN = 16,
    YMARGIN = 16,
    COLS = (SCREEN_WIDTH - XMARGIN * 2) / FONT_WIDTH,
    ROWS = (SCREEN_HEIGHT - YMARGIN * 2) / FONT_HEIGHT,
};

struct cscreen {
    uint8_t *ptr, *rowend;
    uint8_t chars[ROWS * COLS];
};

static void cs_nl(struct cscreen *cs) {
    if (cs->rowend < cs->chars + ROWS * COLS) {
        cs->ptr = cs->rowend;
        cs->rowend += COLS;
    }
}

static void cs_putc(struct cscreen *cs, char c) {
    if (c == '\n') {
        cs_nl(cs);
        return;
    }
    if (cs->ptr < cs->rowend) {
        *cs->ptr++ = c;
    } else if (cs->rowend < cs->chars + ROWS * COLS) {
        cs->ptr = cs->rowend;
        cs->rowend += COLS;
        *cs->ptr++ = c;
    }
}

static void cs_puts(struct cscreen *restrict cs, const char *s) {
    for (; *s != '\0'; s++) {
        cs_putc(cs, *s);
    }
}

static void fatal_thread(void *arg) {
    struct fatal_ctx *ctx = arg;
    const char *msg = ctx->msg;
    va_list ap = ctx->ap;
    (void)ap;
    struct cscreen cs;
    bzero(&cs, sizeof(cs));
    cs.ptr = cs.chars;
    cs.rowend = cs.chars + COLS;
    cs_puts(&cs, "The game has crashed :-(\n");
    cs_puts(&cs, msg);

    uint16_t *fb = framebuffers[0];
    osViSetMode(&osViModeNtscLpn1);
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
    osViSwapBuffer(fb);

    for (;;) {
        for (int cy = 0; cy < ROWS; cy++) {
            for (int cx = 0; cx < COLS; cx++) {
                unsigned c = cs.chars[cy * COLS + cx];
                if (c != 0 && c < FONT_NCHARS) {
                    for (int y = 0; y < FONT_HEIGHT; y++) {
                        uint16_t *optr =
                            fb +
                            (cy * FONT_HEIGHT + y + YMARGIN) * SCREEN_WIDTH +
                            cx * FONT_WIDTH + XMARGIN;
                        uint32_t idata = FONT_DATA[c * FONT_HEIGHT + y];
                        for (int x = 0; x < FONT_WIDTH; x++) {
                            optr[x] =
                                (idata & (1u << x)) != 0 ? 0xffff : 0x0000;
                        }
                    }
                }
            }
        }
        osWritebackDCache(fb, sizeof(u16) * SCREEN_HEIGHT * SCREEN_WIDTH);

        OSTime start = osGetTime();
        while (osGetTime() - start < OS_CPU_COUNTER / 10) {}
    }
}
