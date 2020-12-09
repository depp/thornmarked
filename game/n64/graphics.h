#pragma once

#include <ultra64.h>

#include <stdbool.h>

struct game_system;
struct scheduler;

enum {
    // Size of the framebuffer, in pixels. Note that this is the size of the
    // buffer itself, not the size of what is displayed on-screen, i.e., this is
    // the size of the PAL buffer and NTSC modes use only a part of it.
    SCREEN_WIDTH = 320,
    SCREEN_HEIGHT = 288,
};

// How long a "meter" is in game units.
extern const float meter;

struct graphics {
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
void graphics_frame(struct game_system *restrict sys,
                    struct graphics_state *restrict st, struct scheduler *sc,
                    OSMesgQueue *queue);
