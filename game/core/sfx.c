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

void sfx_update(struct sys_sfx *restrict ssys, float dt) {
    (void)ssys;
    (void)dt;
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
