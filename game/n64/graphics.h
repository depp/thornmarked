#pragma once

#include "game/n64/material.h"

#include <ultra64.h>

#include <stdbool.h>

struct game_state;
struct scheduler;

enum {
    // Size of the framebuffer, in pixels. Note that the buffer always has the
    // size of the PAL framebuffer, and only a portion of it is used on non-PAL
    // systems.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT_PAL = 288,
    SCREEN_HEIGHT_NONPAL = 240,
    SCREEN_HEIGHT_BUFFER = SCREEN_HEIGHT_PAL,
};

// How long a "meter" is in game units.
extern const float meter;

struct graphics {
    // Current task number, 0 or 1.
    int current_task;

    // Framebuffer size (portion displayed on screen, which depends on region).
    int width;
    int height;

    Gfx *dl_ptr;
    Gfx *dl_start;
    Gfx *dl_end;

    Mtx *mtx_ptr;
    Mtx *mtx_start;
    Mtx *mtx_end;

    uint16_t *framebuffer;
    uint16_t *zbuffer;

    bool is_pal;
    Vp *viewport;
    float aspect;

    struct material_state material;
};

// Graphics task state.
struct graphics_state {
    unsigned busy; // Mask for which resources are busy.
    unsigned wait; // Mask for which resources are needed for the next frame.
    int current_task;
    int current_buffer;
};

// The frame index being rendered, or rendered next.
extern unsigned graphics_current_frame;

// Render the next graphics frame.
void graphics_frame(struct game_state *restrict gs,
                    struct graphics_state *restrict st, struct scheduler *sc,
                    OSMesgQueue *queue);
