// Conflicts with <string.h>
#define _OS_LIBC_H_ 1

#include "game/n64/text.h"

#include "assets/font.h"
#include "assets/pak.h"
#include "base/base.h"
#include "base/fixup.h"
#include "base/memory.h"
#include "base/pak/pak.h"
#include "game/core/menu.h"
#include "game/n64/defs.h"
#include "game/n64/graphics.h"

#include <ultra64.h>

#include <stdalign.h>
#include <stdint.h>
#include <string.h>

enum {
    // Maximum total number of font textures.
    MAX_FONT_TEXTURES = 12,

    // Memory available for fonts.
    FONT_HEAP_SIZE = 48 * 1024,
};

// =============================================================================
// Special Sprites
// =============================================================================

union font_sprite_name {
    char text[4];
    unsigned u;
};

struct font_sprite_info {
    union font_sprite_name name;
    color color;
};

#define BLUE 80, 120, 200
#define GREEN 80, 170, 80
#define YELLOW 220, 200, 100
#define RED 250, 100, 100
#define GRAY 150, 150, 150
// clang-format off
const struct font_sprite_info font_sprite_info[] = {
    {{"A"}, {{BLUE}}}, 
    {{"B"}, {{GREEN}}}, 
    {{"CU"}, {{YELLOW}}},
    {{"CD"}, {{YELLOW}}},
    {{"Z"}, {{GRAY}}},
    {{"ST"}, {{RED}}},
    {{"CL"}, {{YELLOW}}},
    {{"CR"}, {{YELLOW}}},
    {{"L"}, {{GRAY}}},
    {{"R"}, {{GRAY}}},
};
// clang-format on
#undef BLUE
#undef GREEN
#undef YELLOW
#undef RED
#undef GRAY

struct font_sprite {
    int schar;
    color color;
};

static struct font_sprite font_get_sprite(const char *name, size_t namelen) {
    if (namelen <= 4) {
        union font_sprite_name nm = {{0}};
        for (size_t i = 0; i < namelen; i++) {
            nm.text[i] = name[i];
        }
        for (size_t i = 0; i < ARRAY_COUNT(font_sprite_info); i++) {
            const struct font_sprite_info *sp = &font_sprite_info[i];
            if (sp->name.u == nm.u) {
                return (struct font_sprite){
                    .schar = 'A' + i,
                    .color = sp->color,
                };
            }
        }
    }
    fatal_error("Unknown text sprite\nSprite: '%.*s'", (int)namelen, name);
}

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
    struct font_texture *textures; // Not used at runtime, use font_textures.
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

struct font_texture_slot {
    struct font_texture texture;
    struct font_glyph *glyphs;
};

// All textures loaded. Slot 0 is invalid, used for invisible glyphs.
static struct font_texture_slot font_textures[MAX_FONT_TEXTURES];

// Fix internal pointers in font after loading.
static void font_fixup(struct font_header *restrict fn, size_t size,
                       int first_texture) {
    const uintptr_t base = (uintptr_t)fn;
    const struct font_texture *restrict texarr =
        pointer_fixup(fn->textures, base, size);
    for (int i = 0; i < fn->texture_count; i++) {
        struct font_texture tex = texarr[i];
        tex.pixels = pointer_fixup(tex.pixels, base, size);
        font_textures[first_texture + i] = (struct font_texture_slot){
            .texture = tex,
            .glyphs = fn->glyphs,
        };
    }
    for (int i = 0; i < fn->glyph_count; i++) {
        struct font_glyph *gi = &fn->glyphs[i];
        if (gi->size[0] == 0 || gi->size[1] == 0) {
            gi->texindex = 0;
        } else {
            gi->texindex += first_texture;
        }
    }
}

// Memory for loading fonts.
static struct mem_zone font_heap;

// Pointer to font data for each font asset.
static struct font_header *font_slots[PAK_FONT_COUNT + 1];

// Number of textures loaded. Includes the empty slot, 0.
static int font_texture_count = 1;

static void font_load(pak_font asset_id) {
    // Load from cartridge memory.
    int obj = pak_font_object(asset_id);
    size_t size = pak_objects[obj].size;
    struct font_header *fn = mem_zone_alloc(&font_heap, size);
    pak_load_asset_sync(fn, size, obj);
    int first = font_texture_count;
    if (fn->texture_count > MAX_FONT_TEXTURES - first) {
        fatal_error("Too many font textures\nLoaded: %d\nNew: %d", first,
                    fn->texture_count);
    }
    font_fixup(fn, size, first);
    font_slots[asset_id.id] = fn;
    font_texture_count = first + fn->texture_count;
}

static const struct font_header *font_get(pak_font asset_id) {
    struct font_header *fn = font_slots[asset_id.id];
    if (fn == NULL) {
        fatal_error("Font not loaded\nFont: %d", asset_id.id);
    }
    return fn;
}

// =============================================================================
// Text Rendering
// =============================================================================

// A glyph and its position on-screen.
struct text_glyph {
    short x, y;
    unsigned short src; // Texture or font.
    unsigned short glyph;
    color color;
};

static noreturn void text_badstring(const char *text, const char *tptr) {
    fatal_error("Bad text\nOffset: %d\nText: \"%s\"", (int)(tptr - text), text);
}

// Convert a string to glyphs. Sets the source to the font ID.
static struct text_glyph *text_to_glyphs(pak_font font_id, pak_font sprite_id,
                                         struct text_glyph *gptr,
                                         struct text_glyph *gend,
                                         const char *text, color tcolor) {
    const char *tptr = text;
    const struct font_header *restrict fn = font_get(font_id);
    const struct font_header *restrict bfn = font_get(sprite_id);
    int line = 0;
    while (*tptr != '\0') {
        if (gptr == gend) {
            fatal_error("Text too long\nLength: %zu\nText: \"%s\"",
                        strlen(text), text);
        }
        unsigned cp = (unsigned char)*tptr++;
        if (cp < 32) {
            if (cp == '\n') {
                line++;
            }
            continue;
        } else if (cp == '{') {
            if (*tptr == '{') {
                tptr++;
            } else {
                const char *name = tptr;
                while (*tptr != '}' && *tptr != '\0') {
                    tptr++;
                }
                if (*tptr == 0) {
                    text_badstring(text, tptr);
                }
                struct font_sprite sp = font_get_sprite(name, tptr - name);
                tptr++;
                *gptr++ = (struct text_glyph){
                    .x = line,
                    .src = sprite_id.id,
                    .glyph = bfn->charmap[sp.schar],
                    .color = {.u = sp.color.u | (tcolor.u & 0xff)},
                };
                continue;
            }
        } else if (cp == '}') {
            if (*tptr == '}') {
                tptr++;
            } else {
                text_badstring(text, tptr);
            }
        }
        *gptr++ = (struct text_glyph){
            .x = line,
            .src = font_id.id,
            .glyph = fn->charmap[cp],
            .color = tcolor,
        };
    }
    return gptr;
}

static void text_align(struct text_glyph *restrict glyphs, int count, int len) {
    int xoff = -(len >> 1);
    for (int i = 0; i < count; i++) {
        glyphs[i].x += xoff;
    }
}

// Calculate the pen coordinates for glyphs. Converts source from font ID to
// texture index.
static void text_place(struct text_glyph *restrict glyphs, int count, int x,
                       int y) {
    int xpos = x, ypos = y, line = 0, line_start = 0;
    for (int i = 0; i < count; i++) {
        if (glyphs[i].x != line) {
            ypos += 18 * (glyphs[i].x - line);
            line = glyphs[i].x;
            text_align(glyphs + line_start, i - line_start, xpos - x);
            xpos = x;
            line_start = i;
        }
        int glyph = glyphs[i].glyph;
        const struct font_glyph *restrict gi =
            &font_slots[glyphs[i].src]->glyphs[glyph];
        glyphs[i].x = xpos + gi->offset[0];
        glyphs[i].y = ypos + gi->offset[1];
        glyphs[i].src = gi->texindex;
        xpos += gi->advance;
    }
    text_align(glyphs + line_start, count - line_start, xpos - x);
}

// Sorts glyphs by texture index, removing empty glyphs. Returns the output
// number of glyphs.
static int text_sort(struct text_glyph *restrict out_glyphs,
                     struct text_glyph *restrict in_glyphs, int count) {
    // Bucket sort by texture.
    int tex_offset[MAX_FONT_TEXTURES] = {0};
    for (int i = 0; i < count; i++) {
        tex_offset[in_glyphs[i].src]++;
    }
    tex_offset[0] = 0; // Don't draw invisible glyphs.
    int pos = 0;
    for (int i = 0; i < MAX_FONT_TEXTURES; i++) {
        int count = tex_offset[i];
        tex_offset[i] = pos;
        pos += count;
    }
    for (int i = 0; i < count; i++) {
        int tex = in_glyphs[i].src;
        if (tex != 0) {
            int gpos = tex_offset[tex]++;
            out_glyphs[gpos] = in_glyphs[i];
        }
    }
    return pos;
}

// Two buffers -- first buffer is for the raw glyphs, second buffer is for
// glyphs converted to sprites + textures + coordinates.
static struct text_glyph glyph_buffer[2][256];

void text_init(void) {
    mem_zone_init(&font_heap, FONT_HEAP_SIZE, "font");
    font_load(FONT_BUTTONS);
    font_load(FONT_TITLE);
    font_load(FONT_BODY);
}

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

Gfx *text_render(Gfx *dl, struct graphics *restrict gr,
                 struct sys_menu *restrict msys) {
    if (msys->text_count == 0) {
        return dl;
    }

    const int x0 = gr->width >> 1, y0 = gr->height >> 1;

    struct text_glyph *gstart = glyph_buffer[0],
                      *gend = gstart + ARRAY_COUNT(glyph_buffer[0]),
                      *gptr = gstart;
    for (int i = 0; i < msys->text_count; i++) {
        struct menu_text *restrict txp = &msys->text[i];
        if (txp->font.id == 0) {
            continue;
        }
        struct text_glyph *gline = gptr;
        gptr = text_to_glyphs(txp->font, FONT_BUTTONS, gptr, gend, txp->text,
                              txp->color);
        if (gptr != gline) {
            text_place(gline, gptr - gline, x0 + txp->pos.x, y0 - txp->pos.y);
        }
    }
    if (gstart == gend) {
        return dl;
    }

    int scount = text_sort(glyph_buffer[1], gstart, gptr - gstart);
    if (scount == 0) {
        return dl;
    }

    int ncommands = 7 + 7 * MAX_FONT_TEXTURES + 4 * scount;
    if (ncommands > gr->dl_end - dl) {
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
    const struct font_glyph *restrict tglyphs = NULL;
    unsigned current_color = glyph_buffer[1][0].color.u;
    *dl++ = (Gfx){.words = {G_SETPRIMCOLOR << 24, current_color}};
    for (int i = 0; i < scount; i++) {
        struct text_glyph *restrict g = &glyph_buffer[1][i];
        if (g->src != current_texture) {
            // 7 commands.
            const struct font_texture_slot *tex = &font_textures[g->src];
            current_texture = g->src;
            dl = text_use_texture(dl, &tex->texture);
            tglyphs = tex->glyphs;
        }
        const struct font_glyph *gi = &tglyphs[g->glyph];

        if (g->color.u != current_color) {
            current_color = g->color.u;
            *dl++ = (Gfx){.words = {G_SETPRIMCOLOR << 24, current_color}};
        }
        // 3 commands.
        gSPTextureRectangle(dl++, g->x << 2, g->y << 2,
                            (g->x + gi->size[0]) << 2,
                            (g->y + gi->size[1]) << 2, 0, gi->pos[0] << 5,
                            gi->pos[1] << 5, 1 << 10, 1 << 10);
    }
    return dl;
}
