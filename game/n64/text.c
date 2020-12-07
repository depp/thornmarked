// Conflicts with <string.h>
#define _OS_LIBC_H_ 1

#include "game/n64/defs.h"

#include "assets/pak.h"
#include "base/base.h"
#include "base/fixup.h"
#include "base/pak/pak.h"

#include <ultra64.h>

#include <stdint.h>
#include <string.h>

enum {
    // Maximum number of textures per font.
    MAX_FONT_TEXTURES = 8,

    // Maximum size in bytes of font asset.
    FONT_BUFFER_SIZE = 32 * 1024,
};

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
    uint8_t data[FONT_BUFFER_SIZE];
};

// Fix internal pointers in font after loading.
static void font_fixup(union font_buffer *p) {
    const uintptr_t base = (uintptr_t)p;
    const size_t size = sizeof(union font_buffer);
    struct font_header *restrict hdr = &p->header;
    hdr->textures = pointer_fixup(hdr->textures, base, size);
    if (hdr->texture_count > MAX_FONT_TEXTURES) {
        fatal_error("Font has too many textures\n\nTextures: %d",
                    hdr->texture_count);
    }
    for (int i = 0; i < hdr->texture_count; i++) {
        struct font_texture *restrict tex = &hdr->textures[i];
        tex->pixels = pointer_fixup(tex->pixels, base, size);
    }
}

static union font_buffer font_buffer ASSET;

void font_load(pak_font asset_id) {
    // Load from cartridge memory.
    pak_load_asset_sync(font_buffer.data, sizeof(font_buffer.data),
                        pak_font_object(asset_id));
    font_fixup(&font_buffer);
}

// =============================================================================
// Text Rendering
// =============================================================================

// A glyph and its position on-screen.
struct text_glyph {
    short x, y;
    unsigned short glyph;
    unsigned short texture;
};

static struct text_glyph *text_to_glyphs(const struct font_header *restrict fn,
                                         struct text_glyph *gptr,
                                         struct text_glyph *gend,
                                         const char *text) {
    while (*text != '\0') {
        if (gptr == gend) {
            fatal_error("String too long\nLength: %zu\nString: %s",
                        strlen(text), text);
        }
        unsigned cp = (unsigned char)*text++;
        gptr->glyph = fn->charmap[cp];
        gptr++;
    }
    return gptr;
}

// Calculate the pen coordinates for glyphs.
static void text_place(const struct font_header *restrict fn,
                       struct text_glyph *restrict glyphs, int count, int x,
                       int y) {
    for (int i = 0; i < count; i++) {
        int glyph = glyphs[i].glyph;
        const struct font_glyph *restrict gi = &fn->glyphs[glyph];
        glyphs[i].x = x;
        glyphs[i].y = y;
        x += gi->advance;
    }
}

// Calculate the location of glyph sprite images, removing empty glyphs and
// sorting by texture index. Returns the output number of glyphs.
static int text_to_sprite(const struct font_header *restrict fn,
                          struct text_glyph *restrict out_glyphs,
                          struct text_glyph *restrict in_glyphs, int count) {
    const int NO_TEXTURE = 0xffff;
    // Bucket sort by texture.
    int tex_count[MAX_FONT_TEXTURES] = {0};
    for (int i = 0; i < count; i++) {
        int glyph = in_glyphs[i].glyph;
        const struct font_glyph *restrict gi = &fn->glyphs[glyph];
        if (gi->size[0] > 0 && gi->size[1] > 0) {
            tex_count[gi->texindex]++;
            in_glyphs[i].x += gi->offset[0];
            in_glyphs[i].y += gi->offset[1];
            in_glyphs[i].texture = gi->texindex;
        } else {
            in_glyphs[i].texture = NO_TEXTURE;
        }
    }
    int tex_offset[MAX_FONT_TEXTURES];
    int pos = 0;
    for (int i = 0; i < MAX_FONT_TEXTURES; i++) {
        tex_offset[i] = pos;
        pos += tex_count[i];
    }
    for (int i = 0; i < count; i++) {
        int tex = in_glyphs[i].texture;
        if (tex < MAX_FONT_TEXTURES) {
            int gpos = tex_offset[tex]++;
            out_glyphs[gpos] = in_glyphs[i];
        }
    }
    return pos;
}

// Two buffers -- first buffer is for the raw glyphs, second buffer is for
// glyphs converted to sprites + textures + coordinates.
static struct text_glyph glyph_buffer[2][256];

static Gfx *text_use_texture(Gfx *dl, const struct font_texture *restrict tex) {
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
    return dl;
}

Gfx *text_render(Gfx *dl, Gfx *dl_end, int x, int y, const char *text) {
    const struct font_header *restrict fn = &font_buffer.header;

    struct text_glyph *gend =
        text_to_glyphs(fn, glyph_buffer[0],
                       glyph_buffer[0] + ARRAY_COUNT(glyph_buffer[0]), text);
    int gcount = gend - glyph_buffer[0];
    if (gcount == 0) {
        return dl;
    }
    text_place(fn, glyph_buffer[0], gcount, x, y);
    int scount = text_to_sprite(fn, glyph_buffer[1], glyph_buffer[0], gcount);
    if (scount == 0) {
        return dl;
    }

    int ncommands = 7 + 7 * MAX_FONT_TEXTURES + 3 * scount;
    if (ncommands > dl_end - dl) {
        fatal_dloverflow();
    }

    // 7 commands.
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetTextureFilter(dl++, G_TF_POINT);
    gDPSetTexturePersp(dl++, G_TP_NONE);
    gDPSetPrimColor(dl++, 0, 0, 255, 128, 0, 255);
    gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(dl++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    int current_texture = -1;
    for (int i = 0; i < scount; i++) {
        struct text_glyph *restrict g = &glyph_buffer[1][i];
        const struct font_glyph *restrict gi = &fn->glyphs[g->glyph];
        if (g->texture != current_texture) {
            // 7 commands.
            const struct font_texture *tex = &fn->textures[g->texture];
            dl = text_use_texture(dl, tex);
            current_texture = g->texture;
        }

        // 3 commands.
        gSPTextureRectangle(dl++, g->x << 2, g->y << 2,
                            (g->x + gi->size[0]) << 2,
                            (g->y + gi->size[1]) << 2, 0, gi->pos[0] << 5,
                            gi->pos[1] << 5, 1 << 10, 1 << 10);
    }
    return dl;
}
