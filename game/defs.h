#pragma once

#include <ultra64.h>

// Load the font with the given asset ID.
void font_load(int asset_id);

// Load the data for the given asset.
void asset_load(void *dest, int asset_id);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, const char *text);