#pragma once

#include "base/pak/types.h"

#include <ultra64.h>

#include <stdnoreturn.h>

// Load the font with the given asset ID.
void font_load(pak_font asset_id);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, Gfx *dl_end, int x, int y, const char *text);

enum {
    // Size of the framebuffer, in pixels.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 288,
};

#define ASSET __attribute__((section("uninit"), aligned(16)))
