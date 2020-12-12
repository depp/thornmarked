#include "game/core/menu.h"

#include "assets/font.h"
#include "assets/image.h"
#include "base/base.h"
#include "game/core/game.h"

#include <string.h>

static const color color_purple = {{111, 82, 179, 255}};

enum {
    // Maximum number of images to display on-screen.
    MENU_IMAGE_COUNT = 4,

    // Maximum number of text segments to display on-screen.
    MENU_TEXT_COUNT = 8,
};

static int u8_float(float x) {
    int i = (int)(x * 256.0f);
    if (i > 255) {
        return 255;
    }
    if (i < 0) {
        return 0;
    }
    return i;
}

static void menu_addtext(struct sys_menu *restrict msys, point pos,
                         color tcolor, const char *text) {
    int index = msys->text_count;
    if (index >= MENU_TEXT_COUNT) {
        fatal_error("menu_addtext: too many text items");
    }
    struct menu_text *restrict txp = &msys->text[index];
    size_t len = strlen(text);
    if (len >= sizeof(txp->text)) {
        fatal_error("menu_addtext: text too long\nlength: %zu", len);
    }
    txp->font = FONT_BS;
    txp->pos = pos;
    txp->color = tcolor;
    memcpy(txp->text, text, len + 1);
    msys->text_count = index + 1;
}

static void menu_clear(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    msys->id = MENU_NONE;
    msys->image_count = 0;
    msys->text_count = 0;
}

static void menu_start(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    msys->id = MENU_START;
    msys->image_count = 1;
    msys->image[0] = (struct menu_image){
        .pos = (point){0, -34},
        .image = IMG_LOGO,
    };
    msys->text_count = 0;
    if (gs->input.count == 0) {
        menu_addtext(msys, (point){0, -60}, color_purple, "No Controller");
    } else {
        menu_addtext(msys, (point){0, -60}, color_purple, "Press Start");
    }
    msys->time = 0.0f;
}

static void menu_start_update(struct game_state *restrict gs, float dt) {
    for (int i = 0; i < gs->input.count; i++) {
        if ((gs->input.input[i].button_press & BUTTON_START) != 0) {
            menu_clear(gs);
            return;
        }
    }
    struct sys_menu *msys = &gs->menu;
    msys->time += dt;
    const float pulse_time = 2.0f;
    if (msys->time > pulse_time) {
        msys->time -= pulse_time;
    }
    float alpha = msys->time * (2.0f / pulse_time);
    if (alpha > 1.0f) {
        alpha = 2.0f - alpha;
    }
    msys->text[0].color.v[3] = u8_float(alpha);
}

void menu_init(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    *msys = (struct sys_menu){
        .image = mem_alloc(MENU_IMAGE_COUNT * sizeof(*msys->image)),
        .text = mem_alloc(MENU_TEXT_COUNT * sizeof(*msys->text)),
    };
    menu_start(gs);
}

void menu_update(struct game_state *restrict gs, float dt) {
    switch (gs->menu.id) {
    case MENU_NONE:
        break;
    case MENU_START:
        menu_start_update(gs, dt);
        break;
    }
}
