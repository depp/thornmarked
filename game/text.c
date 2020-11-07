#include "game/defs.h"

#include "base/pak/pak.h"

#include <ultra64.h>

#include <stdint.h>

struct font_header {
    uint16_t charmap_size;
    uint16_t glyph_count;
    uint16_t texture_count;
};

struct font_char {
    uint16_t codepoint;
    uint16_t glyph;
};

struct font_glyph {
    uint8_t size[2];
    int8_t offset[2];
    uint8_t pos[2];
    uint8_t texindex;
    uint8_t advance;
};

struct font_texture {
    uint16_t width;
    uint16_t height;
    uint8_t pixels[];
};

union font_buffer {
    struct font_header header;
    uint8_t data[8 * 1024];
};

static union font_buffer font_buffer __attribute__((aligned(16)));
static struct font_char *charmap;
static struct font_glyph *glyphinfo;
static struct font_texture *textures[4];

void font_load(int asset_id) {
    // Load from cartridge memory.
    pak_load_asset_sync(font_buffer.data, asset_id);

    // Get pointers to all the objects inside.
    const struct font_header *restrict h = &font_buffer.header;
    const uint8_t *restrict ptr = font_buffer.data + 8;
    charmap = (void *)ptr;
    ptr += sizeof(struct font_char) * h->charmap_size;
    glyphinfo = (void *)ptr;
    ptr += sizeof(struct font_glyph) * h->glyph_count;
    for (int i = 0; i < h->texture_count; i++) {
        struct font_texture *tex = (void *)ptr;
        textures[i] = tex;
        uint32_t size =
            sizeof(struct font_texture) + (tex->width >> 1) * tex->height;
        size = (size + 3) & ~3; // FIXME: macro / function for aligning.
        ptr += size;
    }
}

static uint16_t glyphs[256];

static uint16_t *text_to_glyphs(uint16_t *gptr, const char *text) {
    const struct font_char *cstart = charmap,
                           *cend = charmap + font_buffer.header.charmap_size;
    while (*text != '\0') {
        unsigned cp = (unsigned char)*text++;
        for (const struct font_char *cptr = cstart; cptr != cend; cptr++) {
            if (cptr->codepoint == cp) {
                *gptr++ = cptr->glyph;
                break;
            }
        }
    }
    return gptr;
}

Gfx *text_render(Gfx *dl, int x, int y, const char *text) {
    gDPPipeSync(dl++);
    gDPSetCycleType(dl++, G_CYC_1CYCLE);
    gDPSetPrimColor(dl++, 0, 0, 255, 128, 0, 255);
    gDPSetRenderMode(dl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(dl++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    struct font_texture *tex = textures[0];
    gDPLoadTextureBlock_4b(dl++, tex->pixels, G_IM_FMT_I, tex->width,
                           tex->height, 0, 0, 0, 0, 0, 0, 0);
    uint16_t *gend = text_to_glyphs(glyphs, text);
    for (uint16_t *gptr = glyphs; gptr != gend; gptr++) {
        struct font_glyph *restrict gi = glyphinfo + *gptr;
        if (gi->size[0] != 0) {
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
