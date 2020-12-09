// Large image handling.
#pragma once

#include <ultra64.h>

// Initialize large image handling.
void image_init(void);

// Render large images.
Gfx *image_render(Gfx *dl, Gfx *dl_end);
