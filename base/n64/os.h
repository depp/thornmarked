// OS - Nintendo 64 OS utilities.
#pragma once

#include <ultra64.h>

// Avoid gprel access by declaring a different section.
extern s32 osTvType __attribute__((section(".data")));

// Create a thread. Wraps a call to osCreateThread, but initializes $gp
// correctly. This must be called instead of osCreateThread.
void thread_create(OSThread *thread, void (*func)(void *arg), void *arg,
                   void *stack, int priority);
