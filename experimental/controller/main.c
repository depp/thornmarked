#include "base/console.h"
#include "base/console_n64.h"
#include "base/defs.h"
#include "base/os.h"
#include "experimental/simple/simple.h"

#include "base/base.h"

#include <ultra64.h>

#include <stdbool.h>

static unsigned cont_mask;

static OSMesgQueue cont_queue;
static OSMesg cont_queue_buffer[2];

void program_init(void) {
    osCreateMesgQueue(&cont_queue, cont_queue_buffer,
                      ARRAY_COUNT(cont_queue_buffer));
    osSetEventMesg(OS_EVENT_SI, &cont_queue, NULL);
    u8 cmask;
    OSContStatus cont_status[MAXCONTROLLERS];
    osContInit(&cont_queue, &cmask, cont_status);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        if ((cmask & (1u << i)) != 0 && cont_status[i].errno == 0 &&
            (cont_status[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL) {
            cont_mask |= 1u << i;
        }
    }
}

static int frame_num;
static bool cont_read_active;
static OSContPad controller_state[MAXCONTROLLERS];

Gfx *program_render(Gfx *dl, Gfx *dl_end) {
    bool have_cont = true;
    while (osRecvMesg(&cont_queue, NULL, OS_MESG_NOBLOCK) == 0) {
        have_cont = true;
    }
    if (have_cont) {
        cont_read_active = false;
        osContGetReadData(controller_state);
    }
    if (!cont_read_active) {
        osContStartReadData(&cont_queue);
        cont_read_active = true;
    }
    frame_num++;
    console_init(&console, CONSOLE_TRUNCATE);
    console_printf(&console, "Frame %d\n\n", frame_num);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        console_printf(&console, "Controller %d: ", i);
        if ((cont_mask & (1u << i)) == 0) {
            console_puts(&console, "<not connected>\n");
        } else {
            console_printf(
                &console, "0x%04x (%+4d, %+4d)\n", controller_state[i].button,
                controller_state[i].stick_x, controller_state[i].stick_y);
        }
    }
    dl = console_draw_displaylist(&console, dl, dl_end);
    return dl;
}
