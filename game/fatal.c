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

static void cs_puts(struct cscreen *cs, const char *s) {
    for (; *s != '\0'; s++) {
        cs_putc(cs, *s);
    }
}

static void cs_putnum(struct cscreen *cs, char *buf, int bufsize, int width) {
    if (width < 1) {
        width = 1;
    } else if (width > bufsize) {
        width = bufsize;
    }
    char *end = buf + bufsize;
    char *ptr = buf;
    char *stop = end - width;
    while (ptr < stop && *ptr == '0') {
        ptr++;
    }
    for (; ptr < end; ptr++) {
        cs_putc(cs, *ptr);
    }
}

static void cs_putd(struct cscreen *cs, unsigned x, int width) {
    char buf[10];
    for (int i = 0; i < 10; i++) {
        buf[9 - i] = '0' + (x % 10);
        x /= 10;
    }
    cs_putnum(cs, buf, sizeof(buf), width);
}

static void cs_putx(struct cscreen *cs, unsigned x, int width) {
    static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char buf[8];
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = hex[(x >> (4 * i)) & 15];
    }
    cs_putnum(cs, buf, sizeof(buf), width);
}

static const char *cs_vputfit(struct cscreen *cs, const char *restrict fmt,
                              va_list *ap) {
    for (;;) {
        unsigned c = (unsigned char)*fmt++;
        switch (c) {
        case '\0':
            return fmt;
            // Conversion specifiers.
        case 'd':
        case 'i': {
            int ival = va_arg(*ap, int);
            unsigned uval;
            if (ival < 0) {
                cs_putc(cs, '-');
                uval = -ival;
            } else {
                uval = ival;
            }
            cs_putd(cs, uval, 1);
            return fmt;
        }
        case 'o': {
            va_arg(*ap, unsigned);
            cs_puts(cs, "%o");
            return fmt;
        }
        case 'u': {
            unsigned uval = va_arg(*ap, unsigned);
            cs_putd(cs, uval, 1);
            return fmt;
        }
        case 'x':
        case 'X': {
            unsigned xval = va_arg(*ap, unsigned);
            cs_putx(cs, xval, 1);
            return fmt;
        }
        case 'c': {
            int cval = va_arg(*ap, int);
            cs_putc(cs, cval);
            return fmt;
        }
        case 'p': {
            void *pval = va_arg(*ap, void *);
            cs_puts(cs, "$");
            cs_putx(cs, (uintptr_t)pval, 8);
            return fmt;
        }
        case 's': {
            const char *sval = va_arg(*ap, const char *);
            cs_puts(cs, sval);
            return fmt;
        }
        case '%':
            cs_putc(cs, '%');
            return fmt;
        }
    }
}

static void cs_vputf(struct cscreen *cs, const char *restrict fmt, va_list ap) {
    for (;;) {
        unsigned c = (unsigned char)*fmt++;
        if (c == '\0') {
            return;
        } else if (c == '%') {
            fmt = cs_vputfit(cs, fmt, &ap);
        } else {
            cs_putc(cs, c);
        }
    }
}

static void fatal_thread(void *arg) {
    struct fatal_ctx *ctx = arg;
    const char *msg = ctx->msg;
    va_list ap = ctx->ap;
    struct cscreen cs;
    bzero(&cs, sizeof(cs));
    cs.ptr = cs.chars;
    cs.rowend = cs.chars + COLS;
    cs_puts(&cs, "The game has crashed! :-(\n");
    cs_vputf(&cs, msg, ap);

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
