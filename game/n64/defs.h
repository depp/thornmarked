#pragma once

enum {
    // Size of the framebuffer, in pixels.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 288,
};

#define ASSET __attribute__((section("uninit"), aligned(16)))
