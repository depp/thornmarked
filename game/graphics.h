#pragma once

#include <ultra64.h>

#include <stdbool.h>

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
    Vp viewport;
};
