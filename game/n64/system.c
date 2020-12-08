#include "game/n64/system.h"

#include "assets/texture.h"
#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/scheduler.h"
#include "game/n64/camera.h"
#include "game/n64/defs.h"
#include "game/n64/graphics.h"
#include "game/n64/model.h"
#include "game/n64/text.h"
#include "game/n64/texture.h"

enum {
    // Maximum time to advance during a frame.
    MAX_DELTA_TIME = OS_CPU_COUNTER / 10,
};

void game_system_init(struct game_system *restrict sys) {
    model_render_init();
    texture_init();
    game_init(&sys->state);
    sys->current_frame = 1;
    sys->update_time = osGetTime();
}

void game_system_update(struct game_system *restrict sys,
                        struct scheduler *sc) {
    struct scheduler_frame fr = scheduler_getframe(sc);
    sys->time_frame = fr.frame;
    sys->time_sample = fr.sample;

    OSTime last_time = sys->update_time;
    sys->update_time = osGetTime();
    uint32_t delta_time = sys->update_time - last_time;
    // Clamp delta time, in case something gets out of hand.
    if (delta_time > MAX_DELTA_TIME) {
        delta_time = MAX_DELTA_TIME;
    }
    float dt = (float)(int)delta_time * (1.0f / (float)OS_CPU_COUNTER);

    model_update(&sys->state.model, dt);
    game_update(&sys->state, dt);
}

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

static const Lights1 lights =
    gdSPDefLights1(16, 16, 64,                               // Ambient
                   255 - 16, 255 - 16, 255 - 64, 0, 0, 100); // Sun

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

void game_system_render(struct game_system *restrict sys,
                        struct graphics *restrict gr) {
    struct game_state *restrict gs = &sys->state;
    console_init(&console, CONSOLE_TRUNCATE);
    console_printf(&console, "Frame: %u\n", sys->current_frame);
    console_printf(&console, "DFrame: %u\n",
                   sys->current_frame - sys->time_frame);
    console_printf(&console, "Sample: %u\n", sys->time_sample);

    texture_startframe();
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

    dl = model_render(dl, gr, &gs->model, &gs->physics);

    dl = texture_use(dl, IMG_GROUND);
    gSPClearGeometryMode(dl++,
                         G_SHADE | G_SHADING_SMOOTH | G_LIGHTING | G_CULL_BACK);
    gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_DECALRGB2);
    gSPDisplayList(dl++, ground_dl);
    gDPSetTextureLOD(dl++, G_TL_TILE);

    dl = text_render(dl, gr->dl_end, 20, ysize - 18, "Music");

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
