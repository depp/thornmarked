#pragma once

#include "base/pak/types.h"

#include <ultra64.h>

// Initialize the texture system.
void texture_init(void);

// Load and use the given texture.
Gfx *texture_use(Gfx *dl, pak_texture texture_id);
