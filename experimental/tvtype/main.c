#include "base/console.h"
#include "base/console_n64.h"
#include "base/os.h"
#include "base/pak/pak.h"
#include "experimental/simple/simple.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

struct pak_object pak_objects[2];

enum {
    BOXSIZE = 50,
};

static int region;
static const char *tvtype_name;

void program_init(void) {
    pak_init(0);
    region = pak_get_region();
    int tvtype = osTvType;
    switch (tvtype) {
    case OS_TV_PAL:
        tvtype_name = "PAL";
        break;
    case OS_TV_NTSC:
        tvtype_name = "NTSC";
        break;
    case OS_TV_MPAL:
        tvtype_name = "MPAL";
        break;
    default:
        tvtype_name = "unknown";
        break;
    }
}

static int frame_num;

static const Gfx rect_dl[] = {
    gsDPPipeSync(),
    gsDPSetFillColor(0),
    gsDPFillRectangle(SCREEN_WIDTH / 2 - BOXSIZE, SCREEN_HEIGHT / 2 - BOXSIZE,
                      SCREEN_WIDTH / 2 + BOXSIZE - 1,
                      SCREEN_HEIGHT / 2 + BOXSIZE - 1),
    gsSPEndDisplayList(),
};

Gfx *program_render(Gfx *dl, Gfx *dl_end) {
    frame_num++;
    console_init(&console, CONSOLE_TRUNCATE);
    console_printf(&console, "Frame %d\n", frame_num);
    console_printf(&console, "TV Type %s (%lu)\n", tvtype_name, osTvType);
    console_printf(&console, "Region %c\n", region);

    gSPDisplayList(dl++, rect_dl);
    dl = console_draw_displaylist(&console, dl, dl_end);
    return dl;
}
