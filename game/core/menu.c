#include "game/core/menu.h"

#include "assets/font.h"
#include "assets/image.h"
#include "base/base.h"

#include <string.h>

enum {
    // Maximum number of images to display on-screen.
    MENU_IMAGE_COUNT = 4,

    // Maximum number of text segments to display on-screen.
    MENU_TEXT_COUNT = 8,
};

static void menu_addtext(struct sys_menu *restrict msys, point pos,
                         const char *text) {
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
    txp->color = (color){{111, 82, 179, 255}};
    memcpy(txp->text, text, len + 1);
    msys->text_count = index + 1;
}

void menu_init(struct sys_menu *restrict msys) {
    *msys = (struct sys_menu){
        .image = mem_alloc(MENU_IMAGE_COUNT * sizeof(*msys->image)),
        .text = mem_alloc(MENU_TEXT_COUNT * sizeof(*msys->text)),
    };

    {
        struct menu_image *imp = &msys->image[msys->image_count++];
        *imp = (struct menu_image){
            .pos = (point){0, -34},
            .image = IMG_LOGO,
        };
    }
    menu_addtext(msys, (point){0, -60}, "Press {ST} {A} {B} {CL} {Z}");
}
