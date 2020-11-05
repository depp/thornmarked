// Base library. Includes functions certain from stdlib.
#pragma once

#include <stddef.h>

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
