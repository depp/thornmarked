#include "game/core/menu.h"

#include "assets/font.h"
#include "assets/image.h"
#include "base/base.h"
#include "game/core/game.h"

#include <string.h>

static const color color_nohighlight = {{111, 82, 179, 255}};
static const color color_highlight = {{231, 225, 89, 255}};
static const color color_disable = {{128, 128, 128, 160}};

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

static struct menu_state *menu_top(struct sys_menu *msys) {
    if (msys->stack_size <= 0) {
        fatal_error("Menu stack underflow");
    }
    return &msys->stack[msys->stack_size - 1];
}

static struct menu_state *menu_push(struct sys_menu *restrict msys) {
    if (msys->stack_size > (int)ARRAY_COUNT(msys->stack)) {
        fatal_error("Menu stack overflow");
    }
    return &msys->stack[msys->stack_size++];
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

static void menu_pop(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    msys->image_count = 0;
    msys->text_count = 0;
    if (msys->stack_size > 0) {
        msys->stack_size--;
        msys->stack[msys->stack_size - 1].paint(msys);
    }
}

static void menu_popall(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    msys->image_count = 0;
    msys->text_count = 0;
    msys->stack_size = 0;
}

// =============================================================================
// Input
// =============================================================================

typedef enum {
    MBUTTON_NONE,
    MBUTTON_ERROR,
    MBUTTON_MENU,
    MBUTTON_SELECT,
    MBUTTON_CANCEL,
    MBUTTON_UP,
    MBUTTON_DOWN,
    MBUTTON_LEFT,
    MBUTTON_RIGHT,
} mbutton;

static unsigned menu_button(unsigned buttons) {
    static const unsigned BUTTON_MAP[] = {
        [MBUTTON_MENU] = BUTTON_START,
        [MBUTTON_SELECT] = BUTTON_A,
        [MBUTTON_CANCEL] = BUTTON_B,
        [MBUTTON_UP] = BUTTON_D_UP | BUTTON_C_UP | BUTTON_J_UP,
        [MBUTTON_DOWN] = BUTTON_D_DOWN | BUTTON_C_DOWN | BUTTON_J_DOWN,
        [MBUTTON_LEFT] = BUTTON_D_LEFT | BUTTON_C_LEFT | BUTTON_J_LEFT,
        [MBUTTON_RIGHT] = BUTTON_D_RIGHT | BUTTON_C_RIGHT | BUTTON_J_RIGHT,
    };
    mbutton button = MBUTTON_NONE;
    for (size_t i = 0; i < ARRAY_COUNT(BUTTON_MAP); i++) {
        if ((buttons & BUTTON_MAP[i]) != 0) {
            if (button == MBUTTON_NONE) {
                button = (mbutton)i;
            } else {
                return MBUTTON_ERROR;
            }
        }
    }
    return button;
}

// =============================================================================
// Generic Menus (uses menu_def)
// =============================================================================

struct menu_item {
    const char *name;
    void (*action)(struct game_state *restrict gs, int item);
};

struct menu_def {
    const char *title;
    int count;
    struct menu_item items[];
};

static void menu_def_paint(struct sys_menu *restrict msys) {
    struct menu_state *st = menu_top(msys);
    struct menu_def *def = st->def;
    msys->image_count = 0;
    msys->text_count = 0;
    int item = st->value;
    int pos = 30;
    for (int i = 0; i < def->count; i++) {
        color icolor;
        if (def->items[i].action != NULL) {
            if (item == i) {
                icolor = color_highlight;
            } else {
                icolor = color_nohighlight;
            }
        } else {
            icolor = color_disable;
        }
        menu_addtext(msys, (point){0, pos}, icolor, def->items[i].name);
        pos -= 30;
    }
    if (def->title) {
        menu_addtext(msys, (point){0, 80}, color_nohighlight, def->title);
    }
}

static void menu_def_update(struct game_state *restrict gs, unsigned buttons,
                            float dt) {
    (void)dt;
    struct sys_menu *msys = &gs->menu;
    struct menu_state *st = menu_top(msys);
    struct menu_def *def = st->def;
    int index = st->value;
    switch (menu_button(buttons)) {
    case MBUTTON_SELECT:
        if (index >= 0) {
            if (def->items[index].action != NULL) {
                def->items[index].action(gs, index);
            }
        }
        break;
    case MBUTTON_CANCEL:
        if (msys->stack_size > 1) {
            menu_pop(gs);
        }
        break;
    case MBUTTON_UP:
        if (index >= 0) {
            int pos = index, count = def->count;
            index = -1;
            for (int i = 0; i < count; i++) {
                if (pos > 0) {
                    pos--;
                } else {
                    pos = def->count - 1;
                }
                if (def->items[pos].action != NULL) {
                    index = pos;
                    break;
                }
            }
        }
        break;
    case MBUTTON_DOWN:
        if (index >= 0) {
            int pos = index, count = def->count;
            index = -1;
            for (int i = 0; i < count; i++) {
                pos++;
                if (pos >= def->count) {
                    pos = 0;
                }
                if (def->items[pos].action != NULL) {
                    index = pos;
                    break;
                }
            }
        }
        break;
    }
    if (index != st->value) {
        if (st->value != -1) {
            msys->text[st->value].color = color_nohighlight;
        }
        if (index != -1) {
            msys->text[index].color = color_highlight;
        }
        st->value = index;
    }
}

static void menu_def_push(struct game_state *restrict gs,
                          struct menu_def *def) {
    int item = -1;
    for (int i = 0; i < def->count; i++) {
        if (def->items[i].action != NULL) {
            item = i;
            break;
        }
    }
    struct menu_state *st = menu_push(&gs->menu);
    *st = (struct menu_state){
        .paint = menu_def_paint,
        .update = menu_def_update,
        .def = def,
        .value = item,
    };
    menu_def_paint(&gs->menu);
}

// =============================================================================
// Config Menu
// =============================================================================

static struct menu_def MENU_SETTINGS = {
    .title = "Settings",
    .count = 1,
    .items =
        {
            {"Particles", 0},
        },
};

static void menu_push_settings(struct game_state *restrict gs, int item) {
    (void)item;
    menu_def_push(gs, &MENU_SETTINGS);
}

// =============================================================================
// Top-Level Menu (start game)
// =============================================================================

static void action_play(struct game_state *restrict gs, int item) {
    menu_popall(gs);
    (void)item;
}

static struct menu_def MENU_NEWGAME = {
    .count = 3,
    .items =
        {
            {"Play 1P", 0},
            {"Play 2P", 0},
            {"Settings", menu_push_settings},
        },
};

static void menu_push_newgame(struct game_state *restrict gs) {
    struct menu_def *def = &MENU_NEWGAME;
    int n = gs->input.count;
    if (n > 2) {
        n = 2;
    }
    for (int i = 0; i < n; i++) {
        def->items[i].action = action_play;
    }
    for (int i = n; i < 2; i++) {
        def->items[i].action = NULL;
    }
    menu_def_push(gs, def);
}

// =============================================================================
// Start Menu (press start)
// =============================================================================

static void menu_start_paint(struct sys_menu *restrict msys) {
    struct menu_state *st = menu_top(msys);
    msys->image_count = 1;
    msys->image[0] = (struct menu_image){
        .pos = (point){0, -34},
        .image = IMG_LOGO,
    };
    msys->text_count = 0;
    point pos = {0, -60};
    if (st->value == 0) {
        menu_addtext(msys, pos, color_disable, "No Controller");
    } else {
        menu_addtext(msys, pos, color_nohighlight, "Press Start");
    }
}

static void menu_start_update(struct game_state *restrict gs, unsigned buttons,
                              float dt) {
    struct sys_menu *msys = &gs->menu;
    if ((buttons & BUTTON_START) != 0) {
        msys->stack_size--;
        menu_push_newgame(gs);
        return;
    }
    struct menu_state *st = menu_top(msys);
    st->time += dt;
    const float pulse_time = 2.0f;
    if (st->time > pulse_time) {
        st->time -= pulse_time;
    }
    float alpha = st->time * (2.0f / pulse_time);
    if (alpha > 1.0f) {
        alpha = 2.0f - alpha;
    }
    msys->text[0].color.v[3] = u8_float(alpha);
}

// Push the start menu.
static void menu_push_start(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    struct menu_state *st = menu_push(msys);
    *st = (struct menu_state){
        .paint = menu_start_paint,
        .update = menu_start_update,
        .value = gs->input.count,
    };
    menu_start_paint(&gs->menu);
}

// =============================================================================
// Public API
// =============================================================================

void menu_init(struct game_state *restrict gs) {
    struct sys_menu *msys = &gs->menu;
    *msys = (struct sys_menu){
        .image = mem_alloc(MENU_IMAGE_COUNT * sizeof(*msys->image)),
        .text = mem_alloc(MENU_TEXT_COUNT * sizeof(*msys->text)),
    };
    menu_push_start(gs);
}

void menu_update(struct game_state *restrict gs, float dt) {
    unsigned bstate = 0;
    for (int i = 0; i < gs->input.count; i++) {
        const struct controller_input *restrict pad = &gs->input.input[i];
        bstate |= pad->button_state;
    }
    unsigned button_press = bstate & ~gs->menu.button_state;
    gs->menu.button_state = bstate;
    if (gs->menu.stack_size > 0) {
        gs->menu.stack[gs->menu.stack_size - 1].update(gs, button_press, dt);
    }
}
