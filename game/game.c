#include "game/game.h"

#include "assets/assets.h"
#include "base/base.h"
#include "base/console.h"
#include "base/console_n64.h"
#include "base/mat4.h"
#include "base/mat4_n64.h"
#include "base/pak/pak.h"
#include "base/random.h"
#include "base/vec2.h"
#include "base/vec3.h"
#include "game/defs.h"
#include "game/graphics.h"
#include "game/model.h"

#include <stdbool.h>

enum {
    // Offset of display list in model data.
    MODEL_DL_OFFSET = 0,
};

enum {
    // Margin (black border) at edge of screen.
    MARGIN_X = 16,
    MARGIN_Y = 16,
};

#define RGB16(r, g, b) \
    ((((r)&31u) << 11) | (((g)&31u) << 6) | (((b)&31u) << 1) | 1)
#define RGB16_32(r, g, b) ((RGB16(r, g, b) << 16) | RGB16(r, g, b))

// Initialization display list, invoked at the beginning of each frame.
static const Gfx init_dl[] = {
    // Initialize the RSP.
    gsSPGeometryMode(~(u32)0, 0),
    gsSPTexture(0, 0, 0, 0, G_OFF),

    // Initialize the RDP.
    gsDPPipeSync(),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetCycleType(G_CYC_FILL),

    gsSPEndDisplayList(),
};

#define ASSET __attribute__((section("uninit"), aligned(16)))

static uint8_t model_data[2][16 * 1024] ASSET;
static uint8_t texture[2][4 * 1024] ASSET;

void game_init(struct game_state *restrict gs) {
    rand_init(&gs->rand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    pak_load_asset_sync(model_data[0], sizeof(model_data[0]), MODEL_FAIRY);
    pak_load_asset_sync(model_data[1], sizeof(model_data[1]), MODEL_SPIKE);
    pak_load_asset_sync(texture[0], sizeof(texture[0]), IMG_GROUND);
    pak_load_asset_sync(texture[1], sizeof(texture[1]), IMG_FAIRY);
    physics_init(&gs->physics);
    walk_init(&gs->walk);
    camera_init(&gs->camera);
    model_init(&gs->model);
    for (int i = 0; i < 3; i++) {
        struct cp_phys *restrict phys = physics_new(&gs->physics);
        *phys = (struct cp_phys){
            .pos = {{
                rand_frange(&gs->rand, -1.0f, 1.0f),
                rand_frange(&gs->rand, -1.0f, 1.0f),
            }},
            .vel = {{
                rand_frange(&gs->rand, -1.0f, 1.0f),
                rand_frange(&gs->rand, -1.0f, 1.0f),
            }},
        };
    }
    struct cp_walk *restrict wp = walk_new(&gs->walk);
    wp->drive = (vec2){{0, 0}};
    gs->button_state = 0;
    gs->prev_button_state = 0;
    struct cp_model *restrict mp = model_new(&gs->model);
    mp->model = MODEL_FAIRY;
    mp = model_new(&gs->model);
    mp->model = MODEL_SPIKE;
    mp = model_new(&gs->model);
    mp->model = MODEL_SPIKE;
}

void game_input(struct game_state *restrict gs, OSContPad *restrict pad) {
    const float scale = 1.0f / 80.0f;
    const vec2 drive = {{
        scale * (float)pad->stick_x,
        scale * (float)pad->stick_y,
    }};
    for (unsigned i = 0; i < gs->walk.count; i++) {
        gs->walk.entities[i].drive = drive;
    }
    gs->button_state = pad->button;
}

static const Gfx model_setup_dl[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPGeometryMode(0, G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH | G_LIGHTING),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

static Gfx texture_dl[];

static const Gfx fairy_setup_dl[] = {
    gsDPPipeSync(),
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture[1]),
    gsSPDisplayList(texture_dl),
    gsSPGeometryMode(G_LIGHTING | G_CULL_BACK, G_SHADE | G_SHADING_SMOOTH),
    gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATERGB2),
    gsSPEndDisplayList(),
};

void game_update(struct game_state *restrict gs, float dt) {
    walk_update(&gs->walk, &gs->physics, dt);
    physics_update(&gs->physics, dt);
    camera_update(&gs->camera);
    unsigned button = gs->button_state & ~gs->prev_button_state;
    gs->prev_button_state = gs->button_state;
    if ((button & A_BUTTON) != 0) {
        gs->show_console = !gs->show_console;
    }
}

static const Lights1 lights =
    gdSPDefLights1(16, 16, 64,                               // Ambient
                   255 - 16, 255 - 16, 255 - 64, 0, 0, 100); // Sun

// Display list to load the ground texture.
static Gfx texture_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetCycleType(G_CYC_2CYCLE),
    gsDPSetRenderMode(G_RM_PASS, G_RM_ZB_OPA_SURF2),
    gsSPTexture(0x8000, 0x8000, 5, 0, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetPrimColor(0, 0, 255, 255, 255, 255),
    gsDPSetTextureLOD(G_TL_LOD),
    gsDPSetTextureFilter(G_TF_BILERP),

    // Load data into TMEM (using G_TX_LOADTILE).

    gsDPSetTile(       //
        G_IM_FMT_RGBA, // format
        G_IM_SIZ_16b,  // size
        0,             // size of one row
        0,             // tmem
        G_TX_LOADTILE, // tile
        0,             // palette
        G_TX_NOMIRROR, // cmt
        0,             // maskt
        G_TX_NOLOD,    // shiftt
        G_TX_NOMIRROR, // cms
        0,             // masks
        G_TX_NOLOD),   // shifts

    gsDPLoadSync(),

    gsDPLoadBlock(     //
        G_TX_LOADTILE, // tile
        0,             // uls
        0,             // ult
        1371,          // lrs
        0),            // dxt

    gsDPPipeSync(),

    // Tile: format, size, length of row in 8-byte units, TMEM address, tile,
    // palette, cmt, maskt, shiftt, cms, masks, shifts.
    //
    // TileSize: tile, uls, ult, lrs, lrt
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0, 0, 5, 0, 0, 5, 0),
    gsDPSetTileSize(0, 0, 0, 31 << G_TEXTURE_IMAGE_FRAC,
                    31 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 4, 256, 1, 0, 0, 4, 1, 0, 4, 1),
    gsDPSetTileSize(1, 0, 0, 15 << G_TEXTURE_IMAGE_FRAC,
                    15 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 2, 320, 2, 0, 0, 3, 2, 0, 3, 2),
    gsDPSetTileSize(2, 0, 0, 7 << G_TEXTURE_IMAGE_FRAC,
                    7 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 336, 3, 0, 0, 2, 3, 0, 2, 3),
    gsDPSetTileSize(3, 0, 0, 3 << G_TEXTURE_IMAGE_FRAC,
                    3 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 340, 4, 0, 0, 1, 4, 0, 1, 4),
    gsDPSetTileSize(4, 0, 0, 1 << G_TEXTURE_IMAGE_FRAC,
                    1 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 342, 5, 0, 0, 0, 5, 0, 0, 5),
    gsDPSetTileSize(5, 0, 0, 0 << G_TEXTURE_IMAGE_FRAC,
                    0 << G_TEXTURE_IMAGE_FRAC),

    gsSPEndDisplayList(),
};

// The identity matrix.
static const Mtx identity = {{
    {1 << 16, 0, 1, 0},
    {0, 1 << 16, 0, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
}};

// Vertex data for the ground.
#define X0 (-7)
#define Y0 (-4)
#define X1 7
#define Y1 12
#define V (1 << 6)
#define T (1 << 11)
static const Vtx ground_vtx[] = {
    {{.ob = {X0 * V, Y0 *V, 0}, .tc = {X0 * T, -Y0 *T}}},
    {{.ob = {X1 * V, Y0 *V, 0}, .tc = {X1 * T, -Y0 *T}}},
    {{.ob = {X0 * V, Y1 *V, 0}, .tc = {X0 * T, -Y1 *T}}},
    {{.ob = {X1 * V, Y1 *V, 0}, .tc = {X1 * T, -Y1 *T}}},
};
#undef X0
#undef Y0
#undef X1
#undef Y1
#undef V
#undef T

// Display list to draw the ground, once the texture is loaded.
static const Gfx ground_dl[] = {
    gsSPMatrix(&identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH),
    gsSPVertex(ground_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0),
    gsSPEndDisplayList(),
};

void game_render(struct game_state *restrict gs, struct graphics *restrict gr) {
    Gfx *dl = gr->dl_start;
    const bool full = true;
    const bool clear_z = true;
    const bool clear_cfb = true;
    const bool clear_border = true;

    const int xsize = SCREEN_WIDTH, ysize = gr->is_pal ? 288 : 240;
    {
        int x0 = 0, y0 = 0, x1 = xsize, y1 = ysize;
        float xaspect = 4.0f, yaspect = 3.0f;
        if (!full) {
            xaspect *= (xsize - 2 * MARGIN_X) * ysize;
            yaspect *= (ysize - 2 * MARGIN_Y) * xsize;
            x0 += MARGIN_X;
            y0 += MARGIN_Y;
            x1 -= MARGIN_X;
            y1 -= MARGIN_Y;
        }
        gr->aspect = xaspect / yaspect;
        gSPSegment(dl++, 0, 0);
        gr->viewport = (Vp){{
            .vscale = {(x1 - x0) * 2, (y1 - y0) * 2, G_MAXZ / 2, 0},
            .vtrans = {(x1 + x0) * 2, (y1 + y0) * 2, G_MAXZ / 2, 0},
        }};
        osWritebackDCache(&gr->viewport, sizeof(gr->viewport));
        gSPViewport(dl++, &gr->viewport);
        gSPDisplayList(dl++, init_dl);

        // Clear the zbuffer.
        if (clear_z) {
            gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, xsize,
                             gr->zbuffer);
            gDPSetFillColor(
                dl++, (GPACK_ZDZ(G_MAXFBZ, 0) << 16) | GPACK_ZDZ(G_MAXFBZ, 0));
            gDPSetScissor(dl++, G_SC_NON_INTERLACE, x0, y0, x1, y1);
            gDPFillRectangle(dl++, x0, y0, x1 - 1, y1 - 1);
            gDPPipeSync(dl++);
        }

        gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, xsize,
                         gr->framebuffer);

        // Clear the borders of the color framebuffer.
        if (clear_border) {
            const uint32_t border_color = RGB16_32(0, 0, 0);
            gDPSetScissor(dl++, G_SC_NON_INTERLACE, 0, 0, xsize, ysize);
            gDPSetFillColor(dl++, border_color);
            gDPFillRectangle(dl++, 0, 0, xsize - 1, MARGIN_Y - 1);
            gDPFillRectangle(dl++, 0, MARGIN_Y, MARGIN_X - 1,
                             ysize - MARGIN_Y - 1);
            gDPFillRectangle(dl++, xsize - MARGIN_X, MARGIN_Y, xsize - 1,
                             ysize - MARGIN_Y - 1);
            gDPFillRectangle(dl++, 0, ysize - MARGIN_Y, xsize - 1, ysize - 1);
            gDPPipeSync(dl++);
        }

        gDPSetScissor(dl++, G_SC_NON_INTERLACE, x0, y0, x1, y1);

        // Clear the color framebuffer.
        if (clear_cfb) {
            const uint32_t clear_color = RGB16_32(31, 0, 31);
            gDPSetFillColor(dl++, clear_color);
            gDPFillRectangle(dl++, x0, y0, x1 - 1, y1 - 1);
            gDPPipeSync(dl++);
        }
    }

    // Render game.
    gDPSetDepthImage(dl++, gr->zbuffer);
    gSPSetGeometryMode(dl++, G_ZBUFFER);
    gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    dl = camera_render(&gs->camera, gr, dl);

    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    gSPSetLights1(dl++, lights);
    int current_model = 0;
    float scale = 0.5f;
    for (unsigned i = 0; i < gs->physics.count; i++) {
        struct cp_phys *restrict cp = &gs->physics.entities[i];
        if (i >= gs->model.count) {
            continue;
        }
        struct cp_model *restrict mp = &gs->model.entities[i];
        if (mp->model == 0) {
            continue;
        }
        int model = mp->model;
        if (model != current_model) {
            int index;
            switch (model) {
            case MODEL_FAIRY:
                index = 0;
                gSPDisplayList(dl++, fairy_setup_dl);
                break;
            case MODEL_SPIKE:
                index = 1;
                gSPDisplayList(dl++, model_setup_dl);
                break;
            default:
                index = -1;
            }
            if (index < 0) {
                continue;
            }
            gSPSegment(dl++, 1, K0_TO_PHYS(&model_data[index]));
        }
        Mtx *mtx = gr->mtx_ptr++;
        {
            mat4 mat;
            mat4_translate_rotate_scale(
                &mat, vec3_vec2(vec2_scale(cp->pos, meter), meter),
                cp->orientation, scale);
            mat4_tofixed(mtx, &mat);
        }
        gSPMatrix(dl++, K0_TO_PHYS(mtx),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(dl++, SEGMENT_ADDR(1, MODEL_DL_OFFSET));
        scale *= 0.5f;
    }

    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture[0]);
    gSPDisplayList(dl++, texture_dl);
    gSPClearGeometryMode(dl++,
                         G_SHADE | G_SHADING_SMOOTH | G_LIGHTING | G_CULL_BACK);
    gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_DECALRGB2);
    gSPDisplayList(dl++, ground_dl);
    gDPSetTextureLOD(dl++, G_TL_TILE);

    dl = text_render(dl, gr->dl_end, 20, ysize - 18, "Fairies");

    // Render debugging text overlay.
    if (gs->show_console) {
        dl = console_draw_displaylist(&console, dl, gr->dl_end);
    }

    if (2 > gr->dl_end - dl) {
        fatal_dloverflow();
    }

    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);
    gr->dl_ptr = dl;
}
