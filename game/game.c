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

// Viewport scaling parameters.
static const Vp viewport = {{
    .vscale = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
    .vtrans = {SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0},
}};

// Initialize the RSP.
static const Gfx rspinit_dl[] = {
    gsSPViewport(&viewport),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH | G_FOG |
                          G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR |
                          G_LOD),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPEndDisplayList(),
};

// Initialize the RDP.
static const Gfx rdpinit_dl[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};

#define ASSET __attribute__((section("uninit"), aligned(16)))

static uint8_t model[8 * 1024] ASSET;
static uint8_t texture[4 * 1024] ASSET;

void game_init(struct game_state *restrict gs) {
    rand_init(&gs->rand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    pak_load_asset_sync(&model, sizeof(model), MODEL_FAIRY);
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

static Gfx texture_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(0x8000, 0x8000, 0, 0, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGB, G_CC_DECALRGB),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPLoadTextureBlock(texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                         G_TX_NOMIRROR, G_TX_NOMIRROR, 0, 0, G_TX_NOLOD,
                         G_TX_NOLOD),
    gsSPEndDisplayList(),
};

void game_render(struct game_state *restrict gs, struct graphics *restrict gr) {
    Gfx *dl = gr->dl_start;

    gSPSegment(dl++, 0, 0);
    gSPDisplayList(dl++, rdpinit_dl);
    gSPDisplayList(dl++, rspinit_dl);

    // Clear the zbuffer.
    gDPSetCycleType(dl++, G_CYC_FILL);
    gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     gr->zbuffer);
    gDPSetFillColor(dl++,
                    (GPACK_ZDZ(G_MAXFBZ, 0) << 16) | GPACK_ZDZ(G_MAXFBZ, 0));
    gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Clear the color framebuffer.
    gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     gr->framebuffer);
    gDPPipeSync(dl++);
    gDPSetFillColor(dl++, 0);
    gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Render game.
    gDPPipeSync(dl++);
    gDPSetDepthImage(dl++, gr->zbuffer);
    gSPSetGeometryMode(dl++, G_ZBUFFER);
    gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
    u16 perspNorm;
    Mtx *projection = gr->mtx_ptr++;
    guPerspective(projection, &perspNorm, 33, 320.0f / 240.0f, 64, 2048, 1.0);
    Mtx *camera = gr->mtx_ptr++;
    const float meter = 64.0f;
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
        guTranslate(mtx_tr, cp->pos.v[0] * meter, cp->pos.v[1] * meter, 0.0f);
        guScale(mtx_sc, scale, scale, scale);
        gSPMatrix(dl++, K0_TO_PHYS(mtx_tr),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPMatrix(dl++, K0_TO_PHYS(mtx_sc),
                  G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_NOPUSH);
        gSPDisplayList(dl++, SEGMENT_ADDR(1, MODEL_DL_OFFSET));
        scale *= 0.5f;
    }

    gSPDisplayList(dl++, texture_dl);
    gSPTextureRectangle(dl++, 20 << 2, 100 << 2, (20 + 32) << 2,
                        (100 + 32) << 2, 0, 0, 0, 1 << 10, 1 << 10);

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
