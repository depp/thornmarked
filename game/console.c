#include "game/console.h"

#include "game/defs.h"

#include <PR/os_libc.h>

#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>

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

void console_printf(struct console *cs, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vprintf(cs, fmt, ap);
    va_end(ap);
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
            if (FONT_START <= c && c < FONT_END) {
                for (int y = 0; y < FONT_HEIGHT; y++) {
                    uint16_t *optr = framebuffer + (cy + y) * SCREEN_WIDTH + cx;
                    uint32_t idata =
                        FONT_DATA[(c - FONT_START) * FONT_HEIGHT + y];
                    for (int x = 0; x < FONT_WIDTH; x++) {
                        optr[x] = (idata & (1u << x)) != 0 ? 0xffff : 0x0000;
                    }
                }
            }
        }
        ptr = end;
    }
}

enum {
    FONT_WIDTH2 = 2 * ((FONT_WIDTH + 1) / 2),
    TEX_WIDTH = 128,
    TEX_COLS = TEX_WIDTH / FONT_WIDTH,
    TEX_ROWS = (FONT_NCHARS + TEX_COLS - 1) / TEX_COLS,
    TEX_HEIGHT = TEX_ROWS * FONT_HEIGHT,
};

_Static_assert((TEX_WIDTH / 2) * TEX_HEIGHT <= 4096);

struct console_texpos {
    uint8_t x, y;
};

struct console_texture {
    bool did_init;
    struct console_texpos charpos[FONT_NCHARS];
    uint8_t alignas(8) pixels[(TEX_WIDTH / 2) * TEX_HEIGHT];
};

static void console_texture_init(struct console_texture *restrict ct) {
    ct->did_init = true;
    for (int row = 0; row < TEX_ROWS; row++) {
        for (int col = 0; col < TEX_COLS; col++) {
            int idx = row * TEX_COLS + col;
            if (idx >= FONT_NCHARS) {
                break;
            }
            ct->charpos[idx] = (struct console_texpos){
                .x = col * FONT_WIDTH2,
                .y = row * FONT_HEIGHT,
            };
            for (int y = 0; y < FONT_HEIGHT; y++) {
                unsigned bits = FONT_DATA[idx * FONT_HEIGHT + y];
                uint8_t *optr = ct->pixels +
                                (TEX_WIDTH / 2) * (row * FONT_HEIGHT + y) +
                                (FONT_WIDTH2 / 2) * col;
                for (int x = 0; x < FONT_WIDTH2 / 2; x++, bits >>= 2) {
                    unsigned value = 0;
                    if ((bits & 1) != 0) {
                        value |= 0xf0;
                    }
                    if ((bits & 2) != 0) {
                        value |= 0x0f;
                    }
                    optr[x] = value;
                }
            }
        }
    }
    osWritebackDCache(&ct->pixels, sizeof(ct->pixels));
}

static struct console_texture alignas(8) console_texture;

Gfx *console_draw_displaylist(struct console *cs, Gfx *dl) {
    int nrows = console_nrows(cs);
    if (nrows == 0) {
        return dl;
    }
    struct console_texture *restrict ct = &console_texture;
    if (!ct->did_init) {
        console_texture_init(ct);
    }
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetRenderMode(dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCombineMode(dl++, G_CC_DECALRGB, G_CC_DECALRGB);
    gDPLoadTextureBlock_4b(dl++, ct->pixels, G_IM_FMT_I, TEX_WIDTH, TEX_HEIGHT,
                           0, 0, 0, 0, 0, 0, 0);
    uint8_t *ptr = cs->chars;
    for (int row = 0; row < nrows; row++) {
        uint8_t *end = cs->rowends[row];
        int cy = YMARGIN + row * FONT_HEIGHT;
        for (int col = 0; col < end - ptr; col++) {
            unsigned c = ptr[col];
            int cx = XMARGIN + col * FONT_WIDTH;
            if (FONT_START <= c && c < FONT_END) {
                struct console_texpos tex = ct->charpos[c - FONT_START];
                gSPTextureRectangle(dl++, cx << 2, cy << 2,
                                    (cx + FONT_WIDTH) << 2,
                                    (cy + FONT_HEIGHT) << 2, 0, tex.x << 5,
                                    tex.y << 5, 1 << 10, 1 << 10);
            }
        }
        ptr = end;
    }
    return dl;
}

struct console console;
