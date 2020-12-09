#include "base/n64/fault.h"

#include "base/base.h"
#include "base/console.h"
#include "base/n64/console.h"
#include "base/n64/os.h"

#include <ultra64.h>

#include <stdbool.h>

extern OSThread *__osGetCurrFaultedThread(void);

static const char CRASH_MESSAGE[] = "The game has crashed :-(\n";

enum {
    CRASH_INVALID,
    CRASH_USER,  // User triggered with fatal_error.
    CRASH_FAULT, // Processor fault handler.
};

// Message to send to fault thread.
struct fault_msg {
    int type;
    struct console *console;
};

// Queue for sending messages to fault handler.
static OSMesgQueue fault_queue;
static OSMesg fault_queue_buffer[1];

static noreturn void console_vfatal_impl(struct console *cs, const char *fmt,
                                         va_list ap) {
    if (cs == NULL) {
        cs = &console;
        console_init(cs, CONSOLE_TRUNCATE);
    } else {
        console_newline(cs);
    }
    console_puts(cs, CRASH_MESSAGE);
    console_vprintf(cs, fmt, ap);
    osSendMesg(&fault_queue,
               &(struct fault_msg){
                   .type = CRASH_USER,
                   .console = cs,
               },
               OS_MESG_BLOCK);
    __builtin_trap();
}

noreturn void fatal_dloverflow_func(const char *file, int line) {
    fatal_error("Display list overflow\n%s:%d", file, line);
}

static struct fault_msg fault_msg = {
    .type = CRASH_FAULT,
};

struct fault_reginfo {
    char name[2];
    uint16_t off;
};

// Register names and offsets within context.
#define REG(nm) \
    { .name = #nm, .off = offsetof(__OSThreadContext, nm) }
static const struct fault_reginfo fault_reginfo[] = {
    REG(at), REG(v0), REG(v1), REG(a0), REG(a1), REG(a2), REG(a3), REG(t0),
    REG(t1), REG(t2), REG(t3), REG(t4), REG(t5), REG(t6), REG(t7), REG(s0),
    REG(s1), REG(s2), REG(s3), REG(s4), REG(s5), REG(s6), REG(s7), REG(t8),
    REG(t9), REG(gp), REG(sp), REG(s8), REG(ra),
};

static const char causes[32][6] = {
    [0] = "Int",  [1] = "Mod",  [2] = "TLBL", [3] = "TLBS",
    [4] = "AdEL", [5] = "AdES", [6] = "IBE",  [7] = "DBE",
    [8] = "Sys",  [9] = "Bp",   [10] = "RI",  [11] = "CpU",
    [12] = "Ov",  [13] = "Tr",  [15] = "FPE", [23] = "WATCH",
};

static void fault_showthread(struct console *cs) {
    OSThread *thread = __osGetCurrFaultedThread();
    (void)thread;
    console_puts(cs, "=== Fault ===\n");
    const __OSThreadContext *restrict c = &thread->context;
    int col = 0;
    for (size_t i = 0; i < ARRAY_COUNT(fault_reginfo); i++) {
        if (col == 3) {
            console_putc(cs, '\n');
            col = 0;
        }
        if (col != 0) {
            console_puts(cs, "  ");
        }
        const struct fault_reginfo ri = fault_reginfo[i];
        char nm[3] = {ri.name[0], ri.name[1], '\0'};
        unsigned value = *(const uint64_t *)((const char *)c + ri.off);
        console_printf(cs, "%s: $%08x", nm, value);
        col++;
    }
    unsigned exc = (c->cause >> 2) & 31;
    const char *exc_name = causes[exc];
    if (*exc_name == '\0') {
        exc_name = "?";
    }
    console_printf(cs, "\nsr: $%08x\ncause: $%08x (Exc: %u %s)",
                   (unsigned)c->sr, (unsigned)c->cause, exc, exc_name);
}

void faultproc(void *arg) {
    (void)arg;

    osCreateMesgQueue(&fault_queue, fault_queue_buffer,
                      ARRAY_COUNT(fault_queue_buffer));
    osSetEventMesg(OS_EVENT_FAULT, &fault_queue, &fault_msg);
    console_vfatal_func = console_vfatal_impl;
    int type;
    struct console *cs;
    {
        OSMesg mesgp;
        osRecvMesg(&fault_queue, &mesgp, OS_MESG_BLOCK);
        struct fault_msg *msg = mesgp;
        type = msg->type;
        cs = msg->console;
    }

    if (cs == NULL) {
        cs = &console;
        console_init(cs, CONSOLE_TRUNCATE);
        console_puts(cs, CRASH_MESSAGE);
    }

    switch (type) {
    case CRASH_USER:
        break;
    case CRASH_FAULT:
        fault_showthread(cs);
        break;
    default:
        console_puts(cs, "Bad type");
        break;
    }

    enum {
        SCREEN_WIDTH = 320,
        SCREEN_HEIGHT = 240,
    };

    uint16_t *fb = (uint16_t *)(uintptr_t)0x80300000;
    {
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
        osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
        osViBlack(false);
        osViSwapBuffer(fb);
    }

    for (;;) {
        console_draw_raw(cs, fb);
        osWritebackDCache(fb, sizeof(uint16_t) * SCREEN_HEIGHT * SCREEN_WIDTH);

        OSTime start = osGetTime();
        while (osGetTime() - start < OS_CPU_COUNTER / 10) {}
    }
}
