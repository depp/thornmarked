#pragma once

#include <ultra64.h>

enum {
    // Size of framebuffer.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 288,
};

// Program initialization. Must be defined by program.
void program_init(void);

// Program renderer. Must be defined by program.
Gfx *program_render(Gfx *dl, Gfx *dl_end);
