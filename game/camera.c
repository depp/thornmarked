#include "game/camera.h"

#include "game/graphics.h"

Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl) {
    (void)csys;
    u16 perspNorm;
    Mtx *projection = gr->mtx_ptr++;
    const float far = 16.0f * meter;
    const float near = far * (1.0f / 16.0f);
    // FIXME: correct aspect ratio here.
    guPerspective(projection, &perspNorm, 33, 320.0f / 240.0f, near, far, 1.0);
    Mtx *camera = gr->mtx_ptr++;
    guLookAt(camera,                            //
             0.0f, -5.0f * meter, 4.0f * meter, // eye
             0.0f, 0.0f, 1.0f * meter,          // look at
             0.0f, 0.0f, 1.0f);                 // up
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(dl++, perspNorm);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    return dl;
}