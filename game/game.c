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

static u8 model[8 * 1024] __attribute__((aligned(16)));

void game_init(struct game_state *restrict gs) {
    rand_init(&gs->rand, 0x01234567, 0x243F6A88); // Pi fractional digits.
    pak_load_asset_sync(model, sizeof(model), MESH_FAIRY);
    physics_init(&gs->physics);
    for (int i = 0; i < 10; i++) {
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
}

void game_input(struct game_state *restrict gs, OSContPad *restrict pad) {
    (void)gs;
    (void)pad;
}

static const Gfx model_setup_dl[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPSetGeometryMode(G_CULL_BACK),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

void game_update(struct game_state *restrict gs, float dt) {
    physics_update(&gs->physics, dt);
}

static const Lights1 lights =
    gdSPDefLights1(16, 16, 64,                               // Ambient
                   255 - 16, 255 - 16, 255 - 64, 0, 0, 100); // Sun

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
    guLookAt(camera,                  //
             200.0f, -700.0f, 200.0f, // eye
             0.0f, 0.0f, 0.0f,        // look at
             0.0f, 0.0f, 1.0f);       // up
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    gSPDisplayList(dl++, model_setup_dl);

    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    gSPSetLights1(dl++, lights);
    gSPSetGeometryMode(dl++, G_LIGHTING);
    gSPSegment(dl++, 1, K0_TO_PHYS(model));
    for (struct cp_phys *cp = gs->physics.entities,
                        *ce = cp + gs->physics.count;
         cp != ce; cp++) {
        Mtx *mtx = gr->mtx_ptr++;
        guTranslate(mtx, cp->pos.v[0] * 100.0f, cp->pos.v[1] * 100.0f, 0.0f);
        gSPMatrix(dl++, K0_TO_PHYS(mtx),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(dl++, SEGMENT_ADDR(1, 0));
    }

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
