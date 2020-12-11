#include "game/n64/particle.h"

#include "assets/texture.h"
#include "base/mat4.h"
#include "base/n64/mat4.h"
#include "base/quat.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

const Vtx particle_vtx[] = {
    {{.ob = {-64, 0, -64}, .tc = {0 << 12, 1 << 12}}},
    {{.ob = {+64, 0, -64}, .tc = {1 << 12, 1 << 12}}},
    {{.ob = {-64, 0, +64}, .tc = {0 << 12, 0 << 12}}},
    {{.ob = {+64, 0, +64}, .tc = {1 << 12, 0 << 12}}},
};

void particle_render_init(void) {}

Gfx *particle_render(Gfx *dl, struct graphics *restrict gr,
                     struct sys_particle *restrict psys) {
    if (psys->count == 0) {
        return dl;
    }
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .flags = MAT_PARTICLE,
                          .texture_id = IMG_STAR1,
                      });
    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    for (int i = 0; i < psys->count; i++) {
        struct particle *restrict pp = &psys->particle[i];
        Mtx *mtx = gr->mtx_ptr++;
        {
            mat4 mat;
            mat4_translate_rotate_scale(&mat, vec3_scale(pp->pos, meter),
                                        quat_identity(), 0.5f);
            mat4_tofixed(mtx, &mat);
        }
        gSPMatrix(dl++, K0_TO_PHYS(mtx),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPVertex(dl++, K0_TO_PHYS(particle_vtx), 4, 0);
        gSP2Triangles(dl++, 0, 1, 2, 0, 2, 1, 3, 0);
    }
    return dl;
}
