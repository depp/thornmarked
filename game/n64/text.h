// Text rendering.
#pragma once

#include "base/pak/types.h"

#include <PR/gbi.h>

// Load the font with the given asset ID.
void font_load(pak_font asset_id);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, Gfx *dl_end, int x, int y, const char *text);
