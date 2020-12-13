#pragma once

#include "base/pak/types.h"
#include "base/vectypes.h"

// A sounf effect source.
struct sfx_src {
    pak_track track_id;
    vec3 pos;
    float volume;
};

// Sound effects system.
struct sys_sfx {
    struct sfx_src *src;
    int count;

    float timer;
};

// Initialize the sound effects system.
void sfx_init(struct sys_sfx *restrict ssys);

// Update the sound effect system.
void sfx_update(struct sys_sfx *restrict ssys, float dt);

// Play a sound effect.
void sfx_play(struct sys_sfx *restrict ssys,
              const struct sfx_src *restrict sfx);
