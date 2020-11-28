#include "base/os.h"

// ID of previous thread created.
static int thread_index;

void thread_create(OSThread *thread, void (*func)(void *arg), void *arg,
                   void *stack, int priority) {
    int thread_id = ++thread_index;
    osCreateThread(thread, thread_id, func, arg, stack, priority);
    __asm__("sd $gp, %0" : "=m"(thread->context.gp));
}
