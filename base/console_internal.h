// Internal definitions for the console library.
#pragma once

#include "base/console.h"

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

struct console_row {
    unsigned short start;
    unsigned short end;
};

struct console {
    // Pointer to write position and end of current line.
    uint8_t *ptr, *rowstart, *rowend;
    // Current row number.
    int row;
    // Type of console this is.
    console_type ctype;
    // Pointer to end of each row in half-open range 0..row.
    struct console_row rows[CON_ROWS];
    // Character data buffer. Not a 2D array, just a linear buffer, each row is
    // packed directly after the previous and may be less than CON_COLS columns
    // wide.
    uint8_t chars[CON_ROWS * CON_COLS];
};

struct console_rowptr {
    const uint8_t *start;
    const uint8_t *end;
};

// Get the rows in a console. The array must have size CON_ROWS. Returns the
// number of rows.
int console_rows(struct console *cs, struct console_rowptr *restrict rows);
