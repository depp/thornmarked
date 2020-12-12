// Text rendering.
#pragma once

#include "base/pak/types.h"

#include <PR/gbi.h>

struct graphics;
struct sys_menu;

// Initialize text rendering system.
void text_init(void);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, struct graphics *restrict gr,
                 struct sys_menu *restrict msys);
