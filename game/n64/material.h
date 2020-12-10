#pragma once

#include "base/pak/types.h"
#include "game/core/material.h"

#include <ultra64.h>

// Initialize the texture system.
void material_init(void);

// Information about the current material.
struct material_state {
    // Current active texture.
    pak_texture texture_id;
};

// Load and use the given texture.
Gfx *material_use(struct material_state *restrict mst, Gfx *dl,
                  struct material mat);
