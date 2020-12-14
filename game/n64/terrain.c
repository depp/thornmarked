#include "game/n64/terrain.h"

#include "assets/texture.h"
#include "base/base.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

enum {
    NBLOCK = 1,
};

static int terrain_vtx_size;
static Vtx *terrain_vtx[4];
static Gfx *terrain_dl[2];

void terrain_init(void) {
    const int sx = 4, sy = 4;
    const int y0 = -(sy >> 1), x0 = -(sx >> 1);
    const int vsz = 1 << 8, tsz = 1 << 12;
    const int nvert = (sx + 1) * (sy + 1);
    terrain_vtx_size = nvert;
    Vtx *vtx_start = mem_alloc(nvert * NBLOCK * sizeof(*vtx_start));
    Vtx *vtx = vtx_start;
    for (int i = 0; i < NBLOCK; i++) {
        terrain_vtx[i] = vtx;
        for (int y = 0; y <= sy; y++) {
            for (int x = 0; x <= sx; x++) {
                vtx[y * (sx + 1) + x] = (Vtx){{
                    .ob = {(y0 + y) * vsz, (x0 + x) * vsz, -64},
                    .tc = {(y0 + sy - y) * tsz, (x0 + sx - x) * tsz},
                }};
            }
        }
        vtx += nvert;
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
    osWritebackDCache(vtx_start, nvert * NBLOCK * sizeof(*vtx));
    osWritebackDCache(dlstart, dlsz * sizeof(*dlstart));
    return;
}

Gfx *terrain_render(Gfx *dl, struct graphics *restrict gr) {
    static const pak_texture TEXTURES[] = {IMG_GROUND1, IMG_GROUND2};
    for (int i = 0; i < 1; i++) {
        dl = material_use(&gr->material, dl,
                          (struct material){
                              .texture_id = TEXTURES[0],
                          });
        gSPVertex(dl++, terrain_vtx[i], terrain_vtx_size, 0);
        gSPDisplayList(dl++, terrain_dl[0]);
        dl = material_use(&gr->material, dl,
                          (struct material){
                              .texture_id = TEXTURES[1],
                          });
        gSPDisplayList(dl++, terrain_dl[1]);
    }
    return dl;
}
