#include "assets/font.h"
#include "assets/pak.h"
#include "base/base.h"
#include "base/n64/os.h"
#include "base/n64/scheduler.h"
#include "base/pak/pak.h"
#include "game/n64/audio.h"
#include "game/n64/graphics.h"
#include "game/n64/system.h"
#include "game/n64/task.h"
#include "game/n64/text.h"

#include <ultra64.h>

#include <stdbool.h>
#include <stdint.h>

enum {
    // Maximum number of simultaneous PI requests.
    PI_MSG_COUNT = 8,
};

extern u8 _main_thread_stack[];

static OSMesg pi_message_buffer[PI_MSG_COUNT];
static OSMesgQueue pi_message_queue;

// Main game thread.
static void main(void *arg);

void boot(void) {
    system_main(main, NULL, _main_thread_stack);
}

enum {
    RDP_OUTPUT_LEN = 64 * 1024,
};

// Info for the pak objects, to be loaded from cartridge.
struct pak_object pak_objects[(PAK_SIZE + 1) & ~1]
    __attribute__((section("uninit"), aligned(16)));

static struct scheduler scheduler;

// State of the main thread.
struct main_state {
    // Queue receiving scheduler / controller events.
    OSMesgQueue evt_queue;
    OSMesg evt_buffer[16];

    // Graphics task state.
    struct graphics_state graphics;

    // Audio task state.
    struct audio_state audio;
};

// Read the next event sent to the main thread and process it.
static int process_event(struct main_state *restrict st, int flags) {
    struct event_data e;
    {
        OSMesg evt;
        int ret = osRecvMesg(&st->evt_queue, &evt, flags);
        if (ret != 0)
            return ret;
        e = event_unpack(evt);
    }
    switch (e.type) {
    case EVENT_VIDEO:
        st->graphics.busy &= ~e.value;
        break;
    case EVENT_AUDIO:
        st->audio.busy &= ~e.value;
        break;
    default:
        fatal_error("Bad message type: %d", e.type);
    }
    return 0;
}

static struct main_state main_state;
static struct game_system game_system;

static void main(void *arg) {
    (void)arg;
    struct main_state *st = &main_state;

    // Initialize peripheral manager.
    osCreatePiManager(OS_PRIORITY_PIMGR, &pi_message_queue, pi_message_buffer,
                      PI_MSG_COUNT);

    pak_init(PAK_SIZE);

    // Set up message queues.
    osCreateMesgQueue(&st->evt_queue, st->evt_buffer,
                      ARRAY_COUNT(st->evt_buffer));

    font_load(FONT_BS);

    game_system_init(&game_system);
    scheduler_start(&scheduler, 1);

    for (;;) {
        bool ready = false;

        // Render an audio frame, if ready.
        if ((st->audio.busy & st->audio.wait) == 0) {
            while (process_event(st, OS_MESG_NOBLOCK) == 0) {}
            audio_frame(&st->audio, &scheduler, &st->evt_queue);
            ready = true;
        }

        // Render a graphics frame, if ready.
        if ((st->graphics.busy & st->graphics.wait) == 0) {
            while (process_event(st, OS_MESG_NOBLOCK) == 0) {}
            game_system_update(&game_system, &scheduler);
            graphics_frame(&game_system, &st->graphics, &scheduler,
                           &st->evt_queue);
            ready = true;
        }

        if (!ready) {
            process_event(st, OS_MESG_BLOCK);
        }
    }
}
