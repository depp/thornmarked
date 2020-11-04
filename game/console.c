#include "game/console.h"

#include "game/defs.h"

#include <PR/os_libc.h>

#include <stdarg.h>

// Include font data.
#include "assets/fonts/terminus/ter-u12n.h"

enum {
    XMARGIN = 16,
    YMARGIN = 16,
    COLS = (SCREEN_WIDTH - XMARGIN * 2) / FONT_WIDTH,
    ROWS = (SCREEN_HEIGHT - YMARGIN * 2) / FONT_HEIGHT,
};

struct console {
    uint8_t *ptr, *rowend;
    int row;
    uint8_t *rowends[ROWS];
    uint8_t chars[ROWS * COLS];
};

void console_init(struct console *cs) {
    cs->ptr = cs->chars;
    cs->rowend = cs->chars + COLS;
    cs->row = 0;
}

static void console_nextrow(struct console *cs) {
    cs->rowends[cs->row] = cs->ptr;
    cs->row++;
    if (cs->row < ROWS) {
        cs->rowend = cs->ptr + COLS;
    } else {
        cs->rowend = cs->ptr;
    }
}

void console_putc(struct console *cs, int c) {
    if (c == '\n') {
        if (cs->row < ROWS) {
            console_nextrow(cs);
        }
    } else if (cs->ptr < cs->rowend) {
        *cs->ptr++ = c;
    } else if (cs->row < ROWS) {
        console_nextrow(cs);
        *cs->ptr++ = c;
    }
}

void console_puts(struct console *cs, const char *s) {
    for (; *s != '\0'; s++) {
        console_putc(cs, *s);
    }
}

static void console_putnum(struct console *cs, char *buf, int bufsize,
                           int width) {
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
        console_putc(cs, *ptr);
    }
}

static void console_putd(struct console *cs, unsigned x, int width) {
    char buf[10];
    for (int i = 0; i < 10; i++) {
        buf[9 - i] = '0' + (x % 10);
        x /= 10;
    }
    console_putnum(cs, buf, sizeof(buf), width);
}

static void console_putx(struct console *cs, unsigned x, int width) {
    static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char buf[8];
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = hex[(x >> (4 * i)) & 15];
    }
    console_putnum(cs, buf, sizeof(buf), width);
}

static const char *console_putitem(struct console *cs, const char *restrict fmt,
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
                console_putc(cs, '-');
                uval = -ival;
            } else {
                uval = ival;
            }
            console_putd(cs, uval, 1);
            return fmt;
        }
        case 'o': {
            va_arg(*ap, unsigned);
            console_puts(cs, "%o");
            return fmt;
        }
        case 'u': {
            unsigned uval = va_arg(*ap, unsigned);
            console_putd(cs, uval, 1);
            return fmt;
        }
        case 'x':
        case 'X': {
            unsigned xval = va_arg(*ap, unsigned);
            console_putx(cs, xval, 1);
            return fmt;
        }
        case 'c': {
            int cval = va_arg(*ap, int);
            console_putc(cs, cval);
            return fmt;
        }
        case 'p': {
            void *pval = va_arg(*ap, void *);
            console_puts(cs, "$");
            console_putx(cs, (uintptr_t)pval, 8);
            return fmt;
        }
        case 's': {
            const char *sval = va_arg(*ap, const char *);
            console_puts(cs, sval);
            return fmt;
        }
        case '%':
            console_putc(cs, '%');
            return fmt;
        }
    }
}

void console_vprintf(struct console *cs, const char *fmt, va_list ap) {
    for (;;) {
        unsigned c = (unsigned char)*fmt++;
        if (c == '\0') {
            return;
        } else if (c == '%') {
            fmt = console_putitem(cs, fmt, &ap);
        } else {
            console_putc(cs, c);
        }
    }
}

static int console_nrows(struct console *cs) {
    int nrows = cs->row;
    uint8_t *rowstart = nrows > 0 ? cs->rowends[nrows - 1] : cs->chars;
    if (cs->ptr != rowstart) {
        cs->rowends[nrows] = cs->ptr;
        nrows++;
    }
    return nrows;
}

void console_draw_raw(struct console *cs, uint16_t *restrict framebuffer) {
    uint8_t *ptr = cs->chars;
    for (int row = 0, nrows = console_nrows(cs); row < nrows; row++) {
        uint8_t *end = cs->rowends[row];
        int cy = YMARGIN + row * FONT_HEIGHT;
        for (int col = 0; col < end - ptr; col++) {
            unsigned c = ptr[col];
            int cx = XMARGIN + col * FONT_WIDTH;
            if (c < FONT_NCHARS) {
                for (int y = 0; y < FONT_HEIGHT; y++) {
                    uint16_t *optr = framebuffer + (cy + y) * SCREEN_WIDTH + cx;
                    uint32_t idata = FONT_DATA[c * FONT_HEIGHT + y];
                    for (int x = 0; x < FONT_WIDTH; x++) {
                        optr[x] = (idata & (1u << x)) != 0 ? 0xffff : 0x0000;
                    }
                }
            }
        }
        ptr = end;
    }
}

struct console console;
