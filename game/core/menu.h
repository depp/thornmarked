#pragma once

#include "base/pak/types.h"
#include "base/vectypes.h"

// Coordinates of an on-screen point. The center of the screen is (0, 0). X is
// right, Y is up.
typedef struct point {
    short x, y;
} point;

// An image to display in the menu.
struct menu_image {
    // Position of image (position of anchor).
    point pos;

    // Image asset to display.
    pak_image image;
};

// Text to display in the menu.
struct menu_text {
    // Position of text (pen position).
    point pos;

    // Color to draw the text.
    color color;

    // Text to display, nul-terminated.
    char text[64];
};

// Menu system state.
struct sys_menu {
    // Images to display.
    struct menu_image *image;
    int image_count;

    // Text to display.
    struct menu_text *text;
    int text_count;
};

// Initialize the menu system.
void menu_init(struct sys_menu *restrict msys);
