#include "game/camera.h"

#include "base/mat4.h"
#include "base/mat4_n64.h"
#include "game/graphics.h"

#include <math.h>

void camera_init(struct sys_camera *restrict csys) {
    csys->look_at = (vec3){{0.0f, 0.0f, 1.0f}};
}

// Camera focal length, 1.0 = 90 degree FOV.
const float focal = 3.5f;

void camera_update(struct sys_camera *restrict csys) {
    // Viewpoint: for every 1 meter of camera elevation, move this many meters
    // away from the subject. So, 0 = view down from above, 1 = 45 degree angle,
    // etc.
    const float viewpoint = 1.6f;
    // Distance from subject.
    const float distance = 10.0f;

    // Calculate camera horizontal and vertical position and direction.
    const float cmag = sqrtf(1.0f + viewpoint * viewpoint);
    const float cvert = distance / cmag;
    float choriz = -viewpoint * cvert;
    if (false) {
        choriz +=
            cmag * viewpoint / ((viewpoint + focal) * (viewpoint - focal));
    }

    csys->pos = (vec3){{
        csys->look_at.v[0],
        csys->look_at.v[1] + choriz,
        csys->look_at.v[2] + cvert,
    }};
}

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
        const float xfocal = focal / (0.5f * gr->aspect + 0.5f);
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
