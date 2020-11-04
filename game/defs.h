#pragma once

#include <ultra64.h>

#include <stdnoreturn.h>

// Load the font with the given asset ID.
void font_load(int asset_id);

// Load the data for the given asset.
void asset_load(void *dest, int asset_id);

// Create a display list to draw the given text.
Gfx *text_render(Gfx *dl, int x, int y, const char *text);

// Show a "fatal error" screen.
noreturn void fatal_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

enum {
    // Size of the framebuffer, in pixels.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 240,
};

extern u16 framebuffers[2][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((aligned(16)));

extern OSThread idle_thread;

enum {
    // Thread priorities.
    PRIORITY_IDLE_INIT = 10,
    PRIORITY_IDLE = OS_PRIORITY_IDLE,
    PRIORITY_MAIN = 10,
    PRIORITY_SCHEDULER = 11,
};
