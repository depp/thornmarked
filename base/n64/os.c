#include "base/n64/os.h"

#include "base/base.h"
#include "base/n64/fault.h"
#include "base/n64/system.h"

extern u8 _idle_thread_stack[];
extern u8 _fault_thread_stack[];

static OSThread idle_thread;
static OSThread fault_thread;
static OSThread main_thread;

static void idle(void *arg) {
    (void)arg;

    // Initialize video.
    {
        osCreateViManager(OS_PRIORITY_VIMGR);

        OSViMode *mode;
        switch (osTvType) {
        case OS_TV_PAL:
            mode = &osViModeFpalLpn1;
            break;
        default:
        case OS_TV_NTSC:
            mode = &osViModeNtscLpn1;
            break;
        case OS_TV_MPAL:
            mode = &osViModeMpalLpn1;
            break;
        }
        osViSetMode(mode);
        osViBlack(1);
    }

    thread_create(&fault_thread, faultproc, NULL, _fault_thread_stack,
                  PRIORITY_FAULT);
    osStartThread(&fault_thread);
    mem_init();
    osStartThread(&main_thread);
    // Idle loop.
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
    for (;;) {}
}

noreturn void system_main(void (*func)(void *arg), void *arg, void *stack) {
    osInitialize();
    thread_create(&idle_thread, idle, NULL, _idle_thread_stack,
                  PRIORITY_IDLE_INIT);
    thread_create(&main_thread, func, arg, stack, PRIORITY_MAIN);
    osStartThread(&idle_thread);
    __builtin_unreachable();
}
