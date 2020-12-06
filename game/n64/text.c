#include "game/n64/defs.h"

#include "assets/pak.h"
#include "base/base.h"
#include "base/fixup.h"
#include "base/pak/pak.h"

#include <ultra64.h>

#include <stdint.h>

// =============================================================================
// Font Loading
// =============================================================================

struct font_glyph {
    uint8_t size[2];
    int8_t offset[2];
    uint8_t pos[2];
    uint8_t texindex;
    uint8_t advance;
};

struct font_header {
    uint16_t glyph_count;
    uint16_t texture_count;
    struct font_texture *textures;
    uint8_t charmap[256];
    struct font_glyph glyphs[];
};

struct font_texture {
    uint16_t width;
    uint16_t height;
    uint16_t pix_fmt;
    uint16_t pix_size;
    void *pixels;
};

union font_buffer {
    struct font_header header;
    uint8_t data[8 * 1024];
};

// Fix internal pointers in font after loading.
static void font_fixup(union font_buffer *p) {
    const uintptr_t base = (uintptr_t)p;
    const size_t size = sizeof(union font_buffer);
    struct font_header *restrict hdr = &p->header;
    hdr->textures = pointer_fixup(hdr->textures, base, size);
    for (int i = 0; i < hdr->texture_count; i++) {
        struct font_texture *restrict tex = &hdr->textures[i];
        tex->pixels = pointer_fixup(tex->pixels, base, size);
    }
}

static union font_buffer font_buffer ASSET;

void font_load(pak_data asset_id) {
    // Load from cartridge memory.
    pak_load_asset_sync(font_buffer.data, sizeof(font_buffer.data),
                        pak_data_object(asset_id));
    font_fixup(&font_buffer);
}

// =============================================================================
// Text Rendering
// =============================================================================

static uint16_t glyphs[256];

static uint16_t *text_to_glyphs(const struct font_header *restrict fn,
                                uint16_t *gptr, const char *text) {
    while (*text != '\0') {
        unsigned cp = (unsigned char)*text++;
        *gptr++ = fn->charmap[cp];
    }
    return gptr;
}

Gfx *text_render(Gfx *dl, Gfx *dl_end, int x, int y, const char *text) {
    const struct font_header *restrict fn = &font_buffer.header;
    if (16 > dl_end - dl) {
        fatal_dloverflow();
    }
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetTextureFilter(dl++, G_TF_POINT);
    gDPSetTexturePersp(dl++, G_TP_NONE);
    gDPSetPrimColor(dl++, 0, 0, 255, 128, 0, 255);
    gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(dl++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    struct font_texture *tex = &fn->textures[0];
    switch (tex->pix_size) {
    case G_IM_SIZ_4b:
        gDPLoadTextureBlock_4b(dl++, tex->pixels, tex->pix_fmt, tex->width,
                               tex->height, 0, 0, 0, 0, 0, 0, 0);
        break;
    case G_IM_SIZ_8b:
        gDPLoadTextureBlock(dl++, tex->pixels, tex->pix_fmt, G_IM_SIZ_8b,
                            tex->width, tex->height, 0, 0, 0, 0, 0, 0, 0);
        break;
    case G_IM_SIZ_16b:
        gDPLoadTextureBlock(dl++, tex->pixels, tex->pix_fmt, G_IM_SIZ_16b,
                            tex->width, tex->height, 0, 0, 0, 0, 0, 0, 0);
        break;
    default:
        fatal_error("Unsupported font texture size\nSize: %d", tex->pix_size);
    }
    uint16_t *gend = text_to_glyphs(fn, glyphs, text);
    for (uint16_t *gptr = glyphs; gptr != gend; gptr++) {
        const struct font_glyph *restrict gi = &fn->glyphs[*gptr];
        if (gi->size[0] != 0) {
            if (3 > dl_end - dl) {
                fatal_dloverflow();
            }
            int dx = x + gi->offset[0];
            int dy = y + gi->offset[1];
            gSPTextureRectangle(dl++, dx << 2, dy << 2, (dx + gi->size[0]) << 2,
                                (dy + gi->size[1]) << 2, 0, gi->pos[0] << 5,
                                gi->pos[1] << 5, 1 << 10, 1 << 10);
        }
        x += gi->advance;
    }
    return dl;
}
