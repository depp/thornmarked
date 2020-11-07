// Base library. Includes functions certain from stdlib.
#pragma once

#include "base/defs.h"

// =============================================================================
// Standard Library Functions
// =============================================================================

#include <stddef.h>
#include <stdnoreturn.h>

#if __STDC_HOSTED__

#include <string.h>

#else

// From "Language Standards Supported by GCC"
// https://gcc.gnu.org/onlinedocs/gcc-10.2.0/gcc/Standards.html
//
// GCC requires the freestanding environment provide memcpy, memmove, memset and
// memcmp.

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
    __attribute__((nonnull(1, 2)));
void *memmove(void *dest, const void *src, size_t n)
    __attribute__((nonnull(1, 2)));
void *memset(void *s, int c, size_t n) __attribute__((nonnull(1)));
int memcmp(const void *s1, const void *s2, size_t n)
    __attribute__((nonnull(1, 2)));

size_t strlen(const char *s) __attribute__((pure, nonnull(1)));

#endif

// =============================================================================
// Error Handling
// =============================================================================

// Show a "fatal error" screen.
noreturn void fatal_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

// Assertion failure function, called by assert macro.
noreturn void assert_fail(const char *file, int line, const char *pred);

#define assert(p)                                \
    do {                                         \
        if (!(p))                                \
            assert_fail(__FILE__, __LINE__, #p); \
    } while (0)

// =============================================================================
// Memory Allocation
// =============================================================================

// Initialize the memory subsystem.
void mem_init(void);

// Allocate a region of memory. If not enough memory is available, this calls
// fatal_error, aborting the program. Memory cannot be freed.
void *mem_alloc(size_t sz)
    __attribute__((malloc, alloc_size(1), warn_unused_result));
