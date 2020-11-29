#include "game/camera.h"

#include "game/graphics.h"

#include <math.h>

void camera_init(struct sys_camera *restrict csys) {
    csys->look_at = (vec3){{0.0f, 0.0f, 1.0f}};
}

void camera_update(struct sys_camera *restrict csys) {
    // Viewpoint: for every 1 meter of camera elevation, move this many meters
    // away from the subject. So, 0 = view down from above, 1 = 45 degree angle,
    // etc.
    const float viewpoint = 1.2f;
    // Zoom: camera focal length, 1.0 = 90 degree FOV.
    const float zoom = 2.0f;
    // Distance from subject.
    const float distance = 10.0f * meter;

    // Calculate camera horizontal and vertical position and direction.
    const float cmag = sqrtf(1.0f + viewpoint * viewpoint);
    const float adjust =
        cmag * viewpoint / ((viewpoint + zoom) * (viewpoint - zoom));
    const float cvert = distance / cmag;
    const float choriz = viewpoint * cvert;

    csys->pos = (vec3){{
        csys->look_at.v[0],
        csys->look_at.v[1] - choriz + adjust,
        csys->look_at.v[2] + cvert,
    }};
}

Gfx *camera_render(struct sys_camera *restrict csys,
                   struct graphics *restrict gr, Gfx *dl) {
    u16 perspNorm;
    Mtx *projection = gr->mtx_ptr++;
    const float far = 16.0f * meter;
    const float near = far * (1.0f / 16.0f);
    // FIXME: correct aspect ratio here.
    guPerspective(projection, &perspNorm, 33, 320.0f / 240.0f, near, far, 1.0);
    Mtx *camera = gr->mtx_ptr++;
    guLookAt(camera,                                                     //
             csys->pos.v[0], csys->pos.v[1], csys->pos.v[2],             //
             csys->look_at.v[0], csys->look_at.v[1], csys->look_at.v[2], //
             0.0f, 0.0f, 1.0f);
    gSPMatrix(dl++, K0_TO_PHYS(projection),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(dl++, perspNorm);
    gSPMatrix(dl++, K0_TO_PHYS(camera),
              G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);
    return dl;
}
