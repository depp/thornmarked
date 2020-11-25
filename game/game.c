#include "game/game.h"

#include "assets/assets.h"
#include "base/base.h"
#include "base/console.h"
#include "base/console_n64.h"
#include "base/pak/pak.h"
#include "base/random.h"
#include "game/defs.h"
#include "game/graphics.h"

#include <stdbool.h>

// Header for a model loaded from disk. The rest of the data follows directly
// afterwards.
struct model_data {
    float scale;
};

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

// Viewport scaling parameters (clipped viewport, full viewport).
static const Vp viewport[2] = {
    {{
        .vscale = {(SCREEN_WIDTH - MARGIN_X * 2) * 2,
                   (SCREEN_HEIGHT - MARGIN_Y * 2) * 2, G_MAXZ / 2, 0},
        .vtrans = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
    }},
    {{
        .vscale = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
        .vtrans = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
    }},
};

#define ASSET __attribute__((section("uninit"), aligned(16)))

static uint8_t model[2][8 * 1024] ASSET;
static uint8_t texture[4 * 1024] ASSET;

void game_init(struct game_state *restrict gs) {
    rand_init(&gs->rand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    pak_load_asset_sync(model[0], sizeof(model), MODEL_SPIKE);
    pak_load_asset_sync(&texture, sizeof(texture), IMG_GROUND);
    physics_init(&gs->physics);
    walk_init(&gs->walk);
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
}

static const Gfx model_setup_dl[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

void game_update(struct game_state *restrict gs, float dt) {
    walk_update(&gs->walk, &gs->physics, dt);
    physics_update(&gs->physics, dt);
}

static const Lights1 lights =
    gdSPDefLights1(16, 16, 64,                               // Ambient
                   255 - 16, 255 - 16, 255 - 64, 0, 0, 100); // Sun

// Display list to load the ground texture.
static Gfx texture_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(0x8000, 0x8000, 0, 0, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGB, G_CC_DECALRGB),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPLoadTextureBlock(texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                         G_TX_NOMIRROR, G_TX_NOMIRROR, 5, 5, G_TX_NOLOD,
                         G_TX_NOLOD),
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
#define Y0 (-2)
#define X1 7
#define Y1 12
#define V (1 << 6)
#define T (1 << 11)
static const Vtx ground_vtx[] = {
    {{.ob = {X0 * V, Y0 *V, 0}, .tc = {X0 * T, Y0 *T}}},
    {{.ob = {X1 * V, Y0 *V, 0}, .tc = {X1 * T, Y0 *T}}},
    {{.ob = {X0 * V, Y1 *V, 0}, .tc = {X0 * T, Y1 *T}}},
    {{.ob = {X1 * V, Y1 *V, 0}, .tc = {X1 * T, Y1 *T}}},
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
    const bool full = false;
    const bool clear_z = true;
    const bool clear_cfb = true;
    const bool clear_border = true;

    gSPSegment(dl++, 0, 0);
    gSPViewport(dl++, &viewport[full]);
    gSPDisplayList(dl++, init_dl);

    {
        int x0 = 0, y0 = 0, x1 = SCREEN_WIDTH, y1 = SCREEN_HEIGHT;
        if (!full) {
            x0 += MARGIN_X;
            y0 += MARGIN_Y;
            x1 -= MARGIN_X;
            y1 -= MARGIN_Y;
        }

        // Clear the zbuffer.
        if (clear_z) {
            gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                             gr->zbuffer);
            gDPSetFillColor(
                dl++, (GPACK_ZDZ(G_MAXFBZ, 0) << 16) | GPACK_ZDZ(G_MAXFBZ, 0));
            gDPSetScissor(dl++, G_SC_NON_INTERLACE, x0, y0, x1, y1);
            gDPFillRectangle(dl++, x0, y0, x1 - 1, y1 - 1);
            gDPPipeSync(dl++);
        }

        gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                         gr->framebuffer);

        // Clear the borders of the color framebuffer.
        if (clear_border) {
            const uint32_t border_color = RGB16_32(0, 31, 0);
            gDPSetScissor(dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH,
                          SCREEN_HEIGHT);
            gDPSetFillColor(dl++, border_color);
            gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, MARGIN_Y - 1);
            gDPFillRectangle(dl++, 0, MARGIN_Y, MARGIN_X - 1,
                             SCREEN_HEIGHT - MARGIN_Y - 1);
            gDPFillRectangle(dl++, SCREEN_WIDTH - MARGIN_X, MARGIN_Y,
                             SCREEN_WIDTH - 1, SCREEN_HEIGHT - MARGIN_Y - 1);
            gDPFillRectangle(dl++, 0, SCREEN_HEIGHT - MARGIN_Y,
                             SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
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
    u16 perspNorm;
    Mtx *projection = gr->mtx_ptr++;
    const float meter = 64.0f;
    const float far = 16.0f * meter;
    const float near = far * (1.0f / 16.0f);
    guPerspective(projection, &perspNorm, 33, 320.0f / 240.0f, near, far, 1.0);
    Mtx *camera = gr->mtx_ptr++;
    guLookAt(camera,                            //
             0.0f, -5.0f * meter, 4.0f * meter, // eye
             0.0f, 0.0f, 1.0f * meter,          // look at
             0.0f, 0.0f, 1.0f);                 // up
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPDisplayList(dl++, model_setup_dl);

    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    gSPSetLights1(dl++, lights);
    gSPSetGeometryMode(dl++, G_LIGHTING);
    gSPSegment(dl++, 1, K0_TO_PHYS(&model));
    float scale = 1.0;
    for (struct cp_phys *cp = gs->physics.entities,
                        *ce = cp + gs->physics.count;
         cp != ce; cp++) {
        Mtx *mtx_tr = gr->mtx_ptr++;
        Mtx *mtx_sc = gr->mtx_ptr++;
        guTranslate(mtx_tr, cp->pos.v[0] * meter, cp->pos.v[1] * meter, meter);
        guScale(mtx_sc, scale, scale, scale);
        gSPMatrix(dl++, K0_TO_PHYS(mtx_tr),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(dl++, K0_TO_PHYS(mtx_sc),
                  G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gSPDisplayList(dl++, SEGMENT_ADDR(1, MODEL_DL_OFFSET));
        scale *= 0.5f;
    }

    gSPDisplayList(dl++, texture_dl);
    gSPDisplayList(dl++, ground_dl);

    dl = text_render(dl, gr->dl_end, 20, SCREEN_HEIGHT - 18, "Mintemblo 63");

    // Render debugging text overlay.
    dl = console_draw_displaylist(&console, dl, gr->dl_end);
    if (2 > gr->dl_end - dl) {
        fatal_dloverflow();
    }

    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);
    gr->dl_ptr = dl;
}
