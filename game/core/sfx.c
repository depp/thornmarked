#include "game/core/sfx.h"

#include "assets/track.h"
#include "base/base.h"
#include "base/console.h"

enum {
    // This is just a per-frame limit, actually.
    MAX_SFX_COUNT = 4,
};

void sfx_init(struct sys_sfx *restrict ssys) {
    *ssys = (struct sys_sfx){
        .src = mem_alloc(MAX_SFX_COUNT * sizeof(*ssys->src)),
    };
}

static int n = 0;

void sfx_update(struct sys_sfx *restrict ssys, float dt) {
    ssys->timer -= dt;
    cprintf("timer = %f; n=%d\n", (double)ssys->timer, n);
    if (ssys->timer < 0.0f) {
        n++;
        sfx_play(ssys, &(struct sfx_src){
                           .track_id = SFX_CLANG,
                           .volume = 1.0,
                       });
        ssys->timer = 1.0f;
    }
}

void sfx_play(struct sys_sfx *restrict ssys,
              const struct sfx_src *restrict sfx) {
    // Drop the sfx if there are too many.
    if (ssys->count >= MAX_SFX_COUNT) {
        return;
    }
    int index = ssys->count++;
    ssys->src[index] = *sfx;
}
