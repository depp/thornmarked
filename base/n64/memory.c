#include "base/n64/system.h"

#include "base/memory.h"

#include <ultra64.h>

// Memory layout: There are multiple heaps so we can place the color buffer and
// depth buffer in different DRAM banks, as the Nintendo 64 manual recommends
// for performance.

extern uint8_t _heap1_start[];
extern uint8_t _heap1_end[];
extern uint8_t _heap2_start[];
extern uint8_t _heap2_end[];
extern uint8_t _heap3_start[];

static struct mem_zone mem_zones[] = {
    {
        .start = (uintptr_t)_heap1_start,
        .end = (uintptr_t)_heap1_end,
    },
    {
        .start = (uintptr_t)_heap2_start,
        .end = (uintptr_t)_heap2_end,
    },
    {
        .start = (uintptr_t)_heap3_start,
    },
    {0},
};

void mem_init(void) {
    // This will return either 4M or 8M on real systems. It checks 1M at a
    // time, starting with 4M.
    uint32_t mem_size = osGetMemSize();
    mem_zones[2].end = 0x80000000 + mem_size;
    mem_init_heap(mem_zones);
}
