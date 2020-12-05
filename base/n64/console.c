#include "base/n64/console.h"

#define INCLUDE_FONT_DATA 1

#include "base/base.h"
#include "base/console_internal.h"

#include <stdalign.h>
#include <stdbool.h>

void console_draw_raw(struct console *cs, uint16_t *restrict framebuffer) {
    struct console_rowptr rows[CON_ROWS];
    int nrows = console_rows(cs, rows);
    for (int row = 0; row < nrows; row++) {
        const uint8_t *ptr = rows[row].start, *end = rows[row].end;
        int cy = CON_YMARGIN + row * FONT_HEIGHT;
        for (int col = 0; col < end - ptr; col++) {
            unsigned c = ptr[col];
            int cx = CON_XMARGIN + col * FONT_WIDTH;
            if (FONT_START <= c && c < FONT_END) {
                for (int y = 0; y < FONT_HEIGHT; y++) {
                    uint16_t *optr = framebuffer + (cy + y) * CON_WIDTH + cx;
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

static_assert((TEX_WIDTH / 2) * TEX_HEIGHT <= 4096);

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

static const Gfx console_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetCombineMode(G_CC_DECALRGB, G_CC_DECALRGB),
    gsDPLoadTextureBlock_4b(console_texture.pixels, G_IM_FMT_I, TEX_WIDTH,
                            TEX_HEIGHT, 0, 0, 0, 0, 0, 0, 0),
    gsSPEndDisplayList(),
};

Gfx *console_draw_displaylist(struct console *cs, Gfx *dl, Gfx *dl_end) {
    struct console_rowptr rows[CON_ROWS];
    int nrows = console_rows(cs, rows);
    if (nrows == 0) {
        return dl;
    }
    struct console_texture *restrict ct = &console_texture;
    if (!ct->did_init) {
        console_texture_init(ct);
    }
    if (dl == dl_end) {
        fatal_dloverflow();
    }
    gSPDisplayList(dl++, console_dl);
    for (int row = 0; row < nrows; row++) {
        const uint8_t *ptr = rows[row].start, *end = rows[row].end;
        int cy = CON_YMARGIN + row * FONT_HEIGHT;
        int width = end - ptr;
        if (width * 3 > dl_end - dl) {
            fatal_dloverflow();
        }
        for (int col = 0; col < end - ptr; col++) {
            unsigned c = ptr[col];
            int cx = CON_XMARGIN + col * FONT_WIDTH;
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
