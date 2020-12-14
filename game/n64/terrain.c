#include "game/n64/terrain.h"

#include "assets/texture.h"
#include "base/base.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

static int terrain_vtx_size;
static Vtx *terrain_vtx;
static Gfx *terrain_dl[2];

void terrain_init(void) {
    const int sx = 4, sy = 4;
    const int y0 = -(sy >> 1), x0 = -(sx >> 1);
    const int vsz = 1 << 8, tsz = 1 << 12;
    Vtx *vtx = mem_alloc((sx + 1) * (sy + 1) * sizeof(*vtx));
    terrain_vtx = vtx;
    terrain_vtx_size = (sx + 1) * (sy + 1);
    for (int y = 0; y <= sy; y++) {
        for (int x = 0; x <= sx; x++) {
            vtx[y * (sx + 1) + x] = (Vtx){{
                .ob = {(y0 + y) * vsz, (x0 + x) * vsz, -64},
                .tc = {(y0 + sy - y) * tsz, (x0 + sx - x) * tsz},
            }};
        }
    }
    const int dlsz = sx * sy * 2 + 2;
    Gfx *dlstart = mem_alloc(dlsz * sizeof(*terrain_dl));
    Gfx *dl[2] = {dlstart, dlstart + (dlsz >> 1)};
    terrain_dl[0] = dl[0];
    terrain_dl[1] = dl[1];
    const int tx = 1, ty = sx + 1;
    for (int y = 0; y < sy; y++) {
        for (int x = 0; x < sx; x++) {
            int off = y * (sx + 1) + x;
            gSP2Triangles(dl[(x + y) & 1]++, off, off + tx, off + ty, 0,
                          off + ty, off + tx, off + tx + ty, 0);
        }
    }
    gSPEndDisplayList(dl[0]++);
    gSPEndDisplayList(dl[1]++);
    osWritebackDCache(vtx, (sx + 1) * (sy + 1) * sizeof(*vtx));
    osWritebackDCache(dlstart, dlsz * sizeof(*dlstart));
    return;
}

Gfx *terrain_render(Gfx *dl, struct graphics *restrict gr) {
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .texture_id = IMG_GROUND1,
                      });
    gSPVertex(dl++, terrain_vtx, terrain_vtx_size, 0);
    gSPDisplayList(dl++, terrain_dl[0]);
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .texture_id = IMG_GROUND2,
                      });
    gSPDisplayList(dl++, terrain_dl[1]);
    return dl;
}
