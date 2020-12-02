#pragma once

#include <ultra64.h>

#include <stdnoreturn.h>

// Load the font with the given asset ID.
void font_load(int asset_id);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, Gfx *dl_end, int x, int y, const char *text);

enum {
    // Size of the framebuffer, in pixels.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 288,
};

enum {
    // Thread priorities.
    PRIORITY_IDLE_INIT = 10,
    PRIORITY_IDLE = OS_PRIORITY_IDLE,
    PRIORITY_MAIN = 10,
    PRIORITY_SCHEDULER = 11,
};

#define ASSET __attribute__((section("uninit"), aligned(16)))
