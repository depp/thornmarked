#include "game/n64/particle.h"

#include "assets/texture.h"
#include "base/base.h"
#include "base/ivec3.h"
#include "base/mat4.h"
#include "base/n64/mat4.h"
#include "base/quat.h"
#include "game/core/camera.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

// Particle vertex buffers.
static Vtx *particle_vert[2];

void particle_render_init(void) {
    size_t sz = sizeof(Vtx) * MAX_PARTICLE_COUNT * 4;
    particle_vert[0] = mem_alloc(sz);
    particle_vert[1] = mem_alloc(sz);
    for (int i = 0; i < 2; i++) {
        for (Vtx *vp = particle_vert[i], *ve = vp + MAX_PARTICLE_COUNT * 4;
             vp != ve; vp += 4) {
            vp[0] = (Vtx){{.tc = {0 << 12, 1 << 12}}};
            vp[1] = (Vtx){{.tc = {1 << 12, 1 << 12}}};
            vp[2] = (Vtx){{.tc = {0 << 12, 0 << 12}}};
            vp[3] = (Vtx){{.tc = {1 << 12, 0 << 12}}};
        }
    }
}

Gfx *particle_render(Gfx *dl, struct graphics *restrict gr,
                     struct sys_particle *restrict psys,
                     struct sys_camera *restrict csys) {
    if (psys->count == 0) {
        return dl;
    }
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .flags = MAT_PARTICLE,
                          .texture_id = IMG_STAR1,
                      });
    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    Vtx *vstart = particle_vert[gr->current_task];
    Vtx *restrict vp = vstart;
    vec3 xx = vec3_scale(csys->up, meter);
    vec3 yy = vec3_scale(csys->right, meter);
    for (int i = 0; i < psys->count; i++) {
        struct particle *restrict pp = &psys->particle[i];
        ivec3 pt = ivec3_vec3(vec3_scale(pp->pos, meter));
        ivec3 x = ivec3_vec3(xx);
        ivec3 y = ivec3_vec3(yy);
        for (int i = 0; i < 3; i++) {
            vp[0].v.ob[i] = pt.v[i] - x.v[i] - y.v[i];
            vp[1].v.ob[i] = pt.v[i] + x.v[i] - y.v[i];
            vp[2].v.ob[i] = pt.v[i] - x.v[i] + y.v[i];
            vp[3].v.ob[i] = pt.v[i] + x.v[i] + y.v[i];
        }
        gSPVertex(dl++, K0_TO_PHYS(vp), 4, 0);
        gSP2Triangles(dl++, 0, 1, 2, 0, 2, 1, 3, 0);
        vp += 4;
    }
    osWritebackDCache(vstart, sizeof(*vp) * (vp - vstart));
    return dl;
}
