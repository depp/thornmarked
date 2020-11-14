#include "game/game.h"

#include "assets/assets.h"
#include "base/base.h"
#include "base/console.h"
#include "base/console_n64.h"
#include "base/pak/pak.h"
#include "base/random.h"
#include "game/defs.h"

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

static uint16_t img_cat[32 * 32] __attribute__((aligned(16)));
static uint16_t img_ball[32 * 32] __attribute__((aligned(16)));

static Gfx sprite_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCycleType(G_CYC_COPY), // G_CYC_2CYCLE
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGB, G_CC_DECALRGB),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureFilter(G_TF_POINT), // G_TF_BILERP
    gsDPLoadTextureBlock(img_cat, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                         G_TX_NOMIRROR, G_TX_NOMIRROR, 0, 0, G_TX_NOLOD,
                         G_TX_NOLOD),
    gsSPEndDisplayList(),
};

enum {
    BALL_SIZE = 32,
    NUM_BALLS = 10,
    MARGIN = 10 + BALL_SIZE / 2,
    MIN_X = MARGIN,
    MIN_Y = MARGIN,
    MAX_X = SCREEN_WIDTH - MARGIN - 1,
    MAX_Y = SCREEN_HEIGHT - MARGIN - 1,
    VEL_BASE = 50,
    VEL_RAND = 50,
};

struct ball {
    float x;
    float y;
    float vx;
    float vy;
};

struct game_state {
    // True if the first frame has already passed.
    bool after_first_frame;

    // Timestamp of last frame, low 32 bits.
    uint32_t last_frame_time;

    // Current controller inputs.
    OSContPad controller;

    // True if button is currently pressed.
    bool is_pressed;

    // Background color.
    uint16_t color;

    // Random number generator state.
    struct rand rand;

    // Balls on screen.
    struct ball balls[NUM_BALLS];
};

struct game_state game_state;

void game_init(void) {
    struct game_state *restrict gs = &game_state;
    OSTime time = osGetTime();
    rand_init(&gs->rand, time, 0x243F6A88); // Pi fractional digits.
    for (int i = 0; i < NUM_BALLS; i++) {
        gs->balls[i] = (struct ball){
            .x = rand_frange(&gs->rand, MIN_X, MAX_X),
            .y = rand_frange(&gs->rand, MIN_Y, MAX_Y),
            .vx = rand_frange(&gs->rand, VEL_BASE, VEL_BASE + VEL_RAND),
            .vy = rand_frange(&gs->rand, VEL_BASE, VEL_BASE + VEL_RAND),
        };
        unsigned dir = rand_next(&gs->rand);
        if ((dir & 1) != 0) {
            gs->balls[i].vx = -gs->balls[i].vx;
        }
        if ((dir & 2) != 0) {
            gs->balls[i].vy = -gs->balls[i].vy;
        }
    }
    pak_load_asset_sync(img_cat, IMG_CAT);
    pak_load_asset_sync(img_ball, IMG_BALL);
}

void game_input(OSContPad *restrict pad) {
    struct game_state *restrict gs = &game_state;
    gs->controller = *pad;
}

void game_update(void) {
    struct game_state *restrict gs = &game_state;
    bool was_pressed = gs->is_pressed;
    gs->is_pressed = (gs->controller.button & A_BUTTON) != 0;
    if (!was_pressed && gs->is_pressed) {
        gs->color = rand_next(&gs->rand);
    }

    uint32_t cur_time = osGetTime();
    uint32_t delta_time = cur_time - gs->last_frame_time;
    gs->last_frame_time = cur_time;
    if (!gs->after_first_frame) {
        gs->after_first_frame = true;
        return;
    }
    // Clamp to 100ms, in case something gets out of hand.
    const uint32_t MAX_DELTA = OS_CPU_COUNTER / 10;
    if (delta_time > MAX_DELTA) {
        delta_time = MAX_DELTA;
    }
    float dt = (float)((int)delta_time) * (1.0f / (float)OS_CPU_COUNTER);

    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &gs->balls[i];
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        if (b->x > MAX_X) {
            b->x = MAX_X * 2 - b->x;
            b->vx = -b->vx;
        } else if (b->x < MIN_X) {
            b->x = MIN_X * 2 - b->x;
            b->vx = -b->vx;
        }
        if (b->y > MAX_Y) {
            b->y = MAX_Y * 2 - b->y;
            b->vy = -b->vy;
        } else if (b->y < MIN_Y) {
            b->y = MIN_Y * 2 - b->y;
            b->vy = -b->vy;
        }
    }
}

Gfx *game_render(Gfx *dl, Gfx *dl_end, uint16_t *framebuffer) {
    gSPSegment(dl++, 0, 0);
    gSPDisplayList(dl++, rdpinit_dl);
    gSPDisplayList(dl++, rspinit_dl);

    // Clear the color framebuffer.
    gDPSetCycleType(dl++, G_CYC_FILL);
    gDPSetColorImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     framebuffer);
    gDPPipeSync(dl++);
    gDPSetFillColor(dl++, game_state.color | (game_state.color << 16));
    gDPFillRectangle(dl++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    // Render game.
    gSPDisplayList(dl++, sprite_dl);
    gDPPipeSync(dl++);

    struct game_state *restrict gs = &game_state;
    for (int i = 0; i < NUM_BALLS; i++) {
        struct ball *restrict b = &gs->balls[i];
        int x = b->x - BALL_SIZE / 2;
        int y = b->y - BALL_SIZE / 2;
        gSPTextureRectangle(dl++, x << 2, y << 2, (x + BALL_SIZE - 1) << 2,
                            (y + BALL_SIZE - 1) << 2, 0, 0, 0, 4 << 10,
                            1 << 10);
    }

    dl = text_render(dl, dl_end, 20, SCREEN_HEIGHT - 18,
                     "Scheduler in operation");

    // Render debugging text overlay.
    dl = console_draw_displaylist(&console, dl, dl_end);
    if (2 > dl_end - dl) {
        fatal_dloverflow();
    }

    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);
    return dl;
}
