#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

noreturn void pointer_fixup_error(void *ptr, uintptr_t base, size_t size);

// Convert a relative offset to a pointer. Checks bounds to ensure that the
// offet is smaller than the given size.
inline void *pointer_fixup(void *ptr, uintptr_t base, size_t size) {
    uintptr_t value = (uintptr_t)ptr;
    if (value > size) {
        pointer_fixup_error(ptr, base, size);
    }
    return (void *)(value + base);
}
