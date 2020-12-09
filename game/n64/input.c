#include "game/n64/input.h"

#include "base/base.h"
#include "game/core/input.h"

#include <ultra64.h>

#include <stdbool.h>

enum {
    // Joystick dead zone. If the magnitude of the X and Y joystick coordinates
    // are equal to this value or smaller, the joystick value is replaced with
    // zero.
    JOYSTICK_DEAD_ZONE = 4,

    // The full scale for a joystick. Values with this magnitude are scaled to
    // 1.0.
    JOYSTICK_FULL_SCALE = 64,
};

// SI event queue. Must be a separate queue, because osContRead will read
// from it.
static OSMesgQueue si_queue;
static OSMesg si_buffer[1];

// Whether a controller read is in progress.
static bool controler_read_active;

// Which controller is used for each player.
static uint8_t controller_index[PLAYER_COUNT];

void input_init(struct sys_input *restrict inp) {
    osCreateMesgQueue(&si_queue, si_buffer, ARRAY_COUNT(si_buffer));
    osSetEventMesg(OS_EVENT_SI, &si_queue, NULL);

    // Scan for first controller.
    u8 cont_mask;
    OSContStatus cont_status[MAXCONTROLLERS];
    osContInit(&si_queue, &cont_mask, cont_status);
    int count = 0;
    for (int i = 0; i < MAXCONTROLLERS && count < PLAYER_COUNT; i++) {
        if ((cont_mask & (1u << i)) != 0 && cont_status[i].errno == 0 &&
            (cont_status[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL) {
            controller_index[count++] = i;
        }
    }
    inp->count = count;
}

// Update the input state for one player.
static void input_update_player(struct controller_input *restrict pl,
                                const OSContPad *restrict pad) {
    unsigned state = pad->button;
    unsigned press = state & ~pl->button_state;
    pl->button_state = state;
    pl->button_press = press;

    const int dead_zone = 4;
    if (pad->stick_x < -dead_zone || dead_zone < pad->stick_x ||
        pad->stick_y < -dead_zone || dead_zone < pad->stick_y) {
        const float scale = 1.0f / JOYSTICK_FULL_SCALE;
        pl->joystick = (vec2){{
            scale * (float)pad->stick_x,
            scale * (float)pad->stick_y,
        }};
    } else {
        pl->joystick = (vec2){{0.0f, 0.0f}};
    }
}

void input_update(struct sys_input *restrict inp) {
    if (inp->count == 0) {
        return;
    }
    bool has_cont = false;
    if (osRecvMesg(&si_queue, NULL, OS_MESG_NOBLOCK) == 0) {
        has_cont = true;
    }
    if (has_cont) {
        controler_read_active = false;
        OSContPad controller_state[MAXCONTROLLERS];
        osContGetReadData(controller_state);
        for (int i = 0; i < inp->count; i++) {
            input_update_player(&inp->input[controller_index[i]],
                                &controller_state[i]);
        }
    }
    if (!controler_read_active) {
        osContStartReadData(&si_queue);
        controler_read_active = true;
    }
}
