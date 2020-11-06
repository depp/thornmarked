// Internal definitions for the console library.
#pragma once

// Include font metrics.
#include "assets/fonts/terminus/ter-u12n.h"

enum {
    // Console size in pixels.
    CON_WIDTH = 320,
    CON_HEIGHT = 240,
    // Margin on each side.
    CON_XMARGIN = 16,
    CON_YMARGIN = 16,
    // Number of columns and rows of characters.
    CON_COLS = (CON_WIDTH - CON_XMARGIN * 2) / FONT_WIDTH,
    CON_ROWS = (CON_HEIGHT - CON_YMARGIN * 2) / FONT_HEIGHT,
};

struct console {
    // Pointer to write position and end of current line.
    uint8_t *ptr, *rowend;
    // Current row number.
    int row;
    // Pointer to end of each row in half-open range 0..row.
    uint8_t *rowends[CON_ROWS];
    // Character data buffer. Not a 2D array, just a linear buffer, each row is
    // packed directly after the previous and may be less than CON_COLS columns
    // wide.
    uint8_t chars[CON_ROWS * CON_COLS];
};

// Get the number of rows written to the console and make sure that all row
// pointers up to that point have been initialized.
int console_nrows(struct console *cs);
