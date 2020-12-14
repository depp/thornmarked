#pragma once

#include <ultra64.h>

struct graphics;

void terrain_init(void);

Gfx *terrain_render(Gfx *dl, struct graphics *restrict gr);
