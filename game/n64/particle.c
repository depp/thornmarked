#include "game/n64/particle.h"

#include "assets/texture.h"
#include "base/mat4.h"
#include "base/n64/mat4.h"
#include "base/quat.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

const Vtx particle_vtx[] = {
    {{.ob = {-64, 0, -64}, .tc = {0 << 11, 1 << 11}}},
    {{.ob = {+64, 0, -64}, .tc = {1 << 11, 1 << 11}}},
    {{.ob = {-64, 0, +64}, .tc = {0 << 11, 0 << 11}}},
    {{.ob = {+64, 0, +64}, .tc = {1 << 11, 0 << 11}}},
};

void particle_render_init(void) {}

Gfx *particle_render(Gfx *dl, struct graphics *restrict gr,
                     struct sys_particle *restrict psys) {
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .texture_id = IMG_STAR1,
                      });
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
