#include "game/n64/camera.h"

#include "base/mat4.h"
#include "base/n64/mat4.h"
#include "game/camera.h"
#include "game/graphics.h"

Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl) {
    int persp_norm;
    Mtx *projection = gr->mtx_ptr++;
    {
        const float far = 20.0f * meter;
        const float near = far * (1.0f / 16.0f);
        if (near + far < 2.0f) {
            persp_norm = 0xffff;
        } else {
            int value = (2.0f * 65536.0f) / (near + far);
            persp_norm = value > 0 ? value : 1;
        }
        const float xfocal = csys->focal / (0.5f * gr->aspect + 0.5f);
        const float yfocal = xfocal * gr->aspect;
        mat4 mat;
        mat4_perspective(&mat, xfocal, yfocal, near, far, 1.0f);
        mat4_tofixed(projection, &mat);
    }
    Mtx *camera = gr->mtx_ptr++;
    guLookAt(camera, //
             meter * csys->pos.v[0], meter * csys->pos.v[1],
             meter * csys->pos.v[2], //
             meter * csys->look_at.v[0], meter * csys->look_at.v[1],
             meter * csys->look_at.v[2], //
             0.0f, 0.0f, 1.0f);
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(dl++, persp_norm);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    return dl;
}
