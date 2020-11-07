#include "base/base.h"

#include <ultra64.h>

#include <stdint.h>

// Memory layout: There are multiple heaps so we can place the color buffer and
// depth buffer in different DRAM banks, as the Nintendo 64 manual recommends
// for performance.

struct mem_zone {
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
};

extern uint8_t _heap1_start[];
extern uint8_t _heap1_end[];
extern uint8_t _heap2_start[];

static struct mem_zone mem_zones[2] = {
    {
        .pos = (uintptr_t)_heap1_start,
        .start = (uintptr_t)_heap1_start,
        .end = (uintptr_t)_heap1_end,
    },
    {
        .pos = (uintptr_t)_heap2_start,
        .start = (uintptr_t)_heap2_start,
    },
};

void mem_init(void) {
    // This will return either 4M or 8M on real systems. It checks 1M at a time,
    // starting with 4M.
    uint32_t mem_size = osGetMemSize();
    mem_zones[1].end = 0x80000000 + mem_size;
    for (unsigned i = 0; i < ARRAY_COUNT(mem_zones); i++) {
        if (mem_zones[i].start >= mem_zones[i].end) {
            fatal_error("Bad memory layout");
        }
    }
}

void *mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = (size + 15) & ~(size_t)15;
    for (unsigned i = 0; i < ARRAY_COUNT(mem_zones); i++) {
        uintptr_t avail = mem_zones[i].end - mem_zones[i].pos;
        if (size <= avail) {
            uintptr_t pos = mem_zones[i].pos;
            mem_zones[i].pos = pos + size;
            return (void *)pos;
        }
    }
    fatal_error("Out of memory");
}
