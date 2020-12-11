#include "game/n64/camera.h"

#include "base/mat4.h"
#include "base/n64/mat4.h"
#include "base/vec3.h"
#include "game/n64/graphics.h"

Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl) {
    int persp_norm;
    Mtx *projection = gr->mtx_ptr++;
    {
        const float far = 45.0f * meter;
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
    {
        mat4 mat = (mat4){{
            [0] = csys->right.v[0],
            [1] = csys->up.v[0],
            [2] = -csys->forward.v[0],

            [4] = csys->right.v[1],
            [5] = csys->up.v[1],
            [6] = -csys->forward.v[1],

            [8] = csys->right.v[2],
            [9] = csys->up.v[2],
            [10] = -csys->forward.v[2],

            [12] = -meter * vec3_dot(csys->pos, csys->right),
            [13] = -meter * vec3_dot(csys->pos, csys->up),
            [14] = meter * vec3_dot(csys->pos, csys->forward),
            [15] = 1.0f,
        }};
        mat4_tofixed(camera, &mat);
    }
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(dl++, persp_norm);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    return dl;
}
