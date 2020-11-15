#pragma once

#include <ultra64.h>

void game_init(void);
void game_input(OSContPad *restrict pad);
void game_update(void);

struct graphics {
    Gfx *dl_start;
    Gfx *dl_end;
    uint16_t *framebuffer;

    Mtx projection;
    Mtx camera;
    Mtx translate[2];
    Mtx rotate_y;
    Mtx rotate_x;
};

Gfx *game_render(struct graphics *restrict gr);
