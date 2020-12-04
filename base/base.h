// Base library. Includes functions certain from stdlib.
#pragma once

#include "base/defs.h"

#include <stdnoreturn.h>
#include <stddef.h>

// =============================================================================
// Error Handling
// =============================================================================

struct console;

// Show a "fatal error" screen using the given console.
noreturn void fatal_error_con(struct console *cs, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

// Show a "fatal error" screen.
noreturn void fatal_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

// Show an error for when the display list overflows.
noreturn void fatal_dloverflow_func(const char *file, int line);
#define fatal_dloverflow() fatal_dloverflow_func(__FILE__, __LINE__)

// Assertion failure function, called by assert macro.
noreturn void assert_fail(const char *file, int line, const char *pred);

#define assert(p)                                \
    do {                                         \
        if (!(p))                                \
            assert_fail(__FILE__, __LINE__, #p); \
    } while (0)

#define static_assert _Static_assert

// =============================================================================
// Memory Allocation
// =============================================================================

// Initialize the memory subsystem.
void mem_init(void);

// Allocate a region of memory. If not enough memory is available, this calls
// fatal_error, aborting the program. Memory cannot be freed.
void *mem_alloc(size_t sz)
    __attribute__((malloc, alloc_size(1), warn_unused_result));
