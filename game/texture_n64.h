#pragma once

#include <ultra64.h>

// Initialize the texture system.
void texture_init(void);

// Load and use the given texture.
Gfx *texture_use(Gfx *dl, int asset_id);
