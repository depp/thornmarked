// Nintendo 64 version of debugging console.
#pragma once

#include <stdint.h>

#include <ultra64.h>

struct console;

// Draw the console to the framebuffer using the CPU. Does not flush cache.
void console_draw_raw(struct console *cs, uint16_t *restrict framebuffer);

// Draw the console to the framebuffer by writing a display list.
Gfx *console_draw_displaylist(struct console *cs, Gfx *dl, Gfx *dl_end);
