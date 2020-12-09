// OS - Nintendo 64 OS utilities.
#pragma once

#include <ultra64.h>

#include <stdnoreturn.h>

// Avoid gprel access by declaring a different section.
extern s32 osTvType __attribute__((section(".data")));

enum {
    // Application thread priorities.
    PRIORITY_IDLE_INIT = 10,
    PRIORITY_MAIN,
    PRIORITY_SCHEDULER,

    // Fault handler thread, created by system_main.
    PRIORITY_FAULT,
};

// Create a thread. Wraps a call to osCreateThread, but initializes $gp
// correctly. This must be called instead of osCreateThread.
void thread_create(OSThread *thread, void (*func)(void *arg), void *arg,
                   void *stack, int priority);

// Main entry point.
//
// - Initializes LibUltra.
// - Initializes //base libary: initializes heap and error handler.
// - Creates fault handler.
// - Creates VI manager, sets a low-res mode, and blacks the screen.
// - Runs the main thread.
noreturn void system_main(void (*func)(void *arg), void *arg, void *stack);

// Declaration for the main entry point, which must be defined by the program.
void boot(void);
