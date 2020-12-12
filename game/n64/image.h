// Large image handling.
#pragma once

#include <ultra64.h>

struct graphics;
struct sys_menu;

// Initialize large image handling.
void image_init(void);

// Render large images.
Gfx *image_render(Gfx *dl, struct graphics *restrict gr,
                  struct sys_menu *restrict msys);
