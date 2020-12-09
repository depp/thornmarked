#include "game/n64/graphics.h"

#include "base/base.h"
#include "base/n64/os.h"
#include "base/n64/scheduler.h"
#include "game/n64/system.h"
#include "game/n64/task.h"

#define TRIPLE_BUFFER 0

const float meter = 64.0f;

unsigned graphics_current_frame = 1;

enum {
    VIDEO_BUFCOUNT = TRIPLE_BUFFER ? 3 : 2,
};

static u16 framebuffers[VIDEO_BUFCOUNT][SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.cfb"), aligned(16)));
static u16 zbuffer[SCREEN_WIDTH * SCREEN_HEIGHT]
    __attribute__((section("uninit.zb"), aligned(16)));

static Gfx display_lists[2][1024] __attribute__((section("uninit")));
static Mtx matrixes[2][64] __attribute__((section("uninit")));
static struct scheduler_task tasks[2];
static Vp viewports[2];

enum {
    SP_STACK_SIZE = 1024,
};

static u64 sp_dram_stack[SP_STACK_SIZE / 8] __attribute__((section("uninit")));

// Get the resource mask for the given task.
static unsigned graphics_taskmask(int i) {
    return 1u << i;
}

// Get the resource mask for the given framebuffer.
static unsigned graphics_buffermask(int i) {
    return 4u << i;
}

// Render the next graphics frame.
void graphics_frame(struct game_system *restrict sys,
                    struct graphics_state *restrict st, struct scheduler *sc,
                    OSMesgQueue *queue) {
    const bool is_pal = osTvType == OS_TV_PAL;

    // Create up display lists.
    u64 *data_ptr;
    size_t data_size;
    {
        Gfx *dl_start = display_lists[st->current_task];
        Gfx *dl_end = display_lists[st->current_task] +
                      ARRAY_COUNT(display_lists[st->current_task]);
        Mtx *mtx_start = matrixes[st->current_task];
        Mtx *mtx_end = matrixes[st->current_task] +
                       ARRAY_COUNT(matrixes[st->current_task]);
        struct graphics gr = {
            .dl_ptr = dl_start,
            .dl_start = dl_start,
            .dl_end = dl_end,
            .mtx_ptr = mtx_start,
            .mtx_start = mtx_start,
            .mtx_end = mtx_end,
            .framebuffer = framebuffers[st->current_buffer],
            .zbuffer = zbuffer,
            .is_pal = is_pal,
            .viewport = &viewports[st->current_task],
        };
        game_system_render(sys, &gr);
        data_ptr = (u64 *)dl_start;
        data_size = sizeof(*dl_start) * (gr.dl_ptr - dl_start);
        osWritebackDCache(data_ptr, data_size);
        osWritebackDCache(mtx_start,
                          sizeof(*mtx_start) * (gr.mtx_ptr - mtx_start));
    }

    // Submit RCP task.
    struct scheduler_task *task = &tasks[st->current_task];
    task->flags = SCHEDULER_TASK_VIDEO | SCHEDULER_TASK_FRAMEBUFFER;
    task->task = (OSTask){{
        .type = M_GFXTASK,
        .flags = OS_TASK_DP_WAIT,
        .ucode_boot = (u64 *)rspbootTextStart,
        .ucode_boot_size =
            (uintptr_t)rspbootTextEnd - (uintptr_t)rspbootTextStart,
        .ucode_data = (u64 *)gspF3DEX2_xbusDataStart,
        .ucode_data_size = SP_UCODE_DATA_SIZE,
        .ucode = (u64 *)gspF3DEX2_xbusTextStart,
        .ucode_size = SP_UCODE_SIZE,
        .dram_stack = sp_dram_stack,
        .dram_stack_size = sizeof(sp_dram_stack),
        // No output_buff.
        .data_ptr = data_ptr,
        .data_size = data_size,
    }};
    task->done_queue = queue;
    task->done_mesg = event_pack((struct event_data){
        .type = EVENT_VIDEO,
        .value = graphics_taskmask(st->current_task),
    });
    task->runtime = 0;
    task->data.framebuffer = (struct scheduler_framebuffer){
        .ptr = framebuffers[st->current_buffer],
        .frame = graphics_current_frame,
        .done_queue = queue,
        .done_mesg = event_pack((struct event_data){
            .type = EVENT_VIDEO,
            .value = graphics_buffermask(st->current_buffer),
        }),
    };
    scheduler_submit(sc, task);

    // Update state.
    st->busy |= graphics_taskmask(st->current_task) |
                graphics_buffermask(st->current_buffer);
    st->current_task ^= 1;
    st->current_buffer++;
    if (st->current_buffer >= VIDEO_BUFCOUNT) {
        st->current_buffer = 0;
    }
    st->wait = graphics_taskmask(st->current_task) |
               graphics_buffermask(st->current_buffer);
    graphics_current_frame++;
}
