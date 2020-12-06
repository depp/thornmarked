#include "base/fixup.h"

#include "base/base.h"

noreturn void pointer_fixup_error(void *ptr, uintptr_t base, size_t size) {
    fatal_error("Bad pointer in asset\nPointer: %p\nBase: %p\nSize: %zu", ptr,
                (void *)base, size);
}

void *pointer_fixup(void *ptr, uintptr_t base, size_t size);
