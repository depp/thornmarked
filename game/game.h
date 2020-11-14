#pragma once

#include <ultra64.h>

void game_init(void);
void game_input(OSContPad *restrict pad);
void game_update(void);
Gfx *game_render(Gfx *dl, Gfx *dl_end, uint16_t *framebuffer);
