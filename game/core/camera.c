#include "game/core/camera.h"

#include "base/base.h"
#include "base/vec3.h"

#include <math.h>
#include <stdbool.h>

void camera_init(struct sys_camera *restrict csys) {
    *csys = (struct sys_camera){
        .focal = 3.5f,
        .look_at = {{0.0f, -2.0f, 1.0f}},
    };
}

void camera_update(struct sys_camera *restrict csys) {
    // Viewpoint: for every 1 meter of camera elevation, move this many meters
    // away from the subject. So, 0 = view down from above, 1 = 45 degree angle,
    // etc.
    const float viewpoint = 1.2f;
    // Distance from subject.
    const float distance = 20.0f;

    // Calculate camera horizontal and vertical position and direction.
    const float cmag = sqrtf(1.0f + viewpoint * viewpoint);
    const float cvert = distance / cmag;
    float choriz = -viewpoint * cvert;
    if (false) {
        choriz += cmag * viewpoint /
                  ((viewpoint + csys->focal) * (viewpoint - csys->focal));
    }

    csys->pos = (vec3){{
        csys->look_at.v[0],
        csys->look_at.v[1] + choriz,
        csys->look_at.v[2] + cvert,
    }};

    vec3 up = (vec3){{0.0f, 0.0f, 1.0f}};
    csys->forward = vec3_normalize(vec3_sub(csys->look_at, csys->pos));
    csys->right = vec3_normalize(vec3_cross(csys->forward, up));
    csys->up = vec3_normalize(vec3_cross(csys->right, csys->forward));
}
