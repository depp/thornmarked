#pragma once

#include "base/pak/types.h"
#include "base/vectypes.h"

struct game_state;
struct sys_menu;

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

struct menu_state {
    void (*paint)(struct sys_menu *restrict msys);
    void (*update)(struct game_state *restrict gs, const unsigned buttons,
                   float dt);
    // Menu animation time.
    float time;
    // Menu definition, if applicable.
    struct menu_def *def;
    // Current selected item, or other value.
    int value;
};

// Menu system state.
struct sys_menu {
    // Images to display.
    struct menu_image *image;
    int image_count;

    // Text to display.
    struct menu_text *text;
    int text_count;

    // Stack of menu states.
    struct menu_state stack[4];
    int stack_size;

    // Button state.
    unsigned button_state;
};

// Initialize the menu system.
void menu_init(struct game_state *restrict gs);

// Update the menu system.
void menu_update(struct game_state *restrict gs, float dt);
