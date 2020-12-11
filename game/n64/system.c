#include "game/n64/system.h"

#include "assets/texture.h"
#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/scheduler.h"
#include "game/n64/audio.h"
#include "game/n64/camera.h"
#include "game/n64/defs.h"
#include "game/n64/graphics.h"
#include "game/n64/image.h"
#include "game/n64/input.h"
#include "game/n64/material.h"
#include "game/n64/model.h"
#include "game/n64/particle.h"
#include "game/n64/text.h"
#include "game/n64/texture.h"

void game_system_init(struct game_state *restrict gs) {
    audio_init();
    input_init(&gs->input);
    time_init();
    model_render_init();
    particle_render_init();
    texture_init();
    image_init();
    game_init(gs);
    gs->show_console = true;
}

void game_system_update(struct game_state *restrict gs, struct scheduler *sc) {
    input_update(&gs->input);
    float dt = time_update(&gs->time, sc);
    game_update(gs, dt);
}

enum {
    // Margin (black border) at edge of screen.
    MARGIN_X = 16,
    MARGIN_Y = 16,
};

#define RGB16(r, g, b) \
    ((((r)&31u) << 11) | (((g)&31u) << 6) | (((b)&31u) << 1) | 1)
#define RGB16_32(r, g, b) ((RGB16(r, g, b) << 16) | RGB16(r, g, b))

// The identity matrix.
static const Mtx identity = {{
    {1 << 16, 0, 1, 0},
    {0, 1 << 16, 0, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
}};

// Initialization display list, invoked at the beginning of each frame.
static const Gfx init_dl[] = {
    // Initialize the RSP.
    gsSPGeometryMode(~(u32)0, 0),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPMatrix(&identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH),

    // Initialize the RDP.
    gsDPPipeSync(),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetCycleType(G_CYC_FILL),

    gsSPEndDisplayList(),
};

// Vertex data for the ground.
#define X0 (-8)
#define Y0 (-4)
#define X1 8
#define Y1 14
#define V (1 << 7)
#define T (1 << 11)
#define Z (-V)
static const Vtx ground_vtx[] = {
    {{.ob = {X0 * V, Y0 *V, Z}, .tc = {X0 * T, -Y0 *T}}},
    {{.ob = {X1 * V, Y0 *V, Z}, .tc = {X1 * T, -Y0 *T}}},
    {{.ob = {X0 * V, Y1 *V, Z}, .tc = {X0 * T, -Y1 *T}}},
    {{.ob = {X1 * V, Y1 *V, Z}, .tc = {X1 * T, -Y1 *T}}},
};
#undef X0
#undef Y0
#undef X1
#undef Y1
#undef V
#undef T
#undef Z

// Display list to draw the ground, once the texture is loaded.
static const Gfx ground_dl[] = {
    gsSPVertex(ground_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0),
    gsSPEndDisplayList(),
};

void game_system_render(struct game_state *restrict gs,
                        struct graphics *restrict gr) {
    if (gs->time.track_loop > 0) {
        const float to_beats = 138.0f / (60.0f * AUDIO_SAMPLERATE);
        const float fbeat = to_beats * gs->time.track_pos;
        int beat = fbeat;
        const float subbeat = fbeat - beat;
        int measure = (beat >> 2) + 1;
        beat = (beat & 3) + 1;
        cprintf("%d:%02d:%d:%04.2f\n", gs->time.track_loop, measure, beat,
                (double)subbeat);
    }

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
        *gr->viewport = (Vp){{
            .vscale = {(x1 - x0) * 2, (y1 - y0) * 2, G_MAXZ / 2, 0},
            .vtrans = {(x1 + x0) * 2, (y1 + y0) * 2, G_MAXZ / 2, 0},
        }};
        osWritebackDCache(gr->viewport, sizeof(*gr->viewport));
        gSPViewport(dl++, gr->viewport);
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
    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    dl = camera_render(&gs->camera, gr, dl);
    dl = model_render(dl, gr, &gs->model, &gs->physics);
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .texture_id = IMG_GROUND,
                      });
    gSPDisplayList(dl++, ground_dl);
    dl = particle_render(dl, gr, &gs->particle, &gs->camera);
    gDPSetTextureLOD(dl++, G_TL_TILE);

    dl = text_render(dl, gr->dl_end, 20, ysize - 18, "Monsters!");

    // Render debugging text overlay.
    if (gs->show_console) {
        dl = console_draw_displaylist(&console, dl, gr->dl_end);
    }

    // Render large images.
    dl = image_render(dl, gr->dl_end);

    if (2 > gr->dl_end - dl) {
        fatal_dloverflow();
    }

    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);
    gr->dl_ptr = dl;
}
