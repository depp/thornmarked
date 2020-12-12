#pragma once

#include "base/pak/types.h"
#include "base/vectypes.h"

struct game_state;

// All possible menus.
typedef enum menu_id {
    MENU_NONE,
    MENU_START,
} menu_id;

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
    pak_font font;

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

    // Menu animation time.
    float time;

    // Current active menu.
    menu_id id;
};

// Initialize the menu system.
void menu_init(struct game_state *restrict gs);

// Update the menu system.
void menu_update(struct game_state *restrict gs, float dt);
