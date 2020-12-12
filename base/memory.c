#include "base/memory.h"
#include "base/base.h"

static noreturn void malloc_fail(size_t size) {
    fatal_error("Out of memory\nSize: %zu", size);
}

void mem_zone_init(struct mem_zone *restrict z, size_t size, const char *name) {
    uintptr_t base = (uintptr_t)mem_alloc(size);
    *z = (struct mem_zone){
        .pos = base,
        .start = base,
        .end = base + size,
        .name = name,
    };
}

void *mem_zone_alloc(struct mem_zone *restrict z, size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = (size + 15) & ~(size_t)15;
    size_t rem = z->end - z->pos;
    if (rem < size) {
        fatal_error("mem_zone_alloc failed\nsize = %zu\nzone = %s", size,
                    z->name);
    }
    uintptr_t ptr = z->pos;
    z->pos = ptr + size;
    return (void *)ptr;
}

#if _ULTRA64

#include <string.h>

enum {
    // Memory allocation alignment.
    MEM_ALIGN = 16,
};

// Pointer to array of memory zones in heap. The array is terminated by an entry
// with pos = 0.
static struct mem_zone *mem_heap;

// Must be a power of two.
static_assert((MEM_ALIGN & (MEM_ALIGN - 1)) == 0);

static uintptr_t align_up(uintptr_t x) {
    return (x + 15) & ~(uintptr_t)15;
}

static uintptr_t align_down(uintptr_t x) {
    return x & ~(uintptr_t)15;
}

void mem_init_heap(struct mem_zone *zones) {
    for (struct mem_zone *z = zones; z->start != 0; z++) {
        z->pos = z->start = align_up(z->start);
        z->end = align_down(z->end);
        if (z->start > z->end) {
            fatal_error("Bad memory layout");
        }
    }
    mem_heap = zones;
}

void *mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size_t asize = align_up(size);

    if (mem_heap == NULL) {
        fatal_error("No heap");
    }

    // Use the smallest zone first.
    struct mem_zone *best_zone = NULL;
    size_t best_avail = ~(size_t)0;
    for (struct mem_zone *z = mem_heap; z->pos != 0; z++) {
        uintptr_t avail = z->end - z->pos;
        if (asize <= avail && avail < best_avail) {
            best_zone = z;
            best_avail = avail;
        }
    }

    if (best_zone == NULL) {
        malloc_fail(size);
    }

    void *ptr = (void *)best_zone->pos;
    best_zone->pos += asize;
    return ptr;
}

void *mem_calloc(size_t size) {
    void *ptr = mem_alloc(size);
    memset(ptr, 0, size);
    return ptr;
}

#else

#include <stdlib.h>

void *mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = malloc(size);
    if (ptr == NULL) {
        malloc_fail(size);
    }
    return ptr;
}

void *mem_calloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = calloc(size, 1);
    if (ptr == NULL) {
        malloc_fail(size);
    }
    return ptr;
}

#endif
