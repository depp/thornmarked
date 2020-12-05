#pragma once

#include <stdint.h>

// A contiguous zone where memory can be allocated.
struct mem_zone {
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
};

// Initialize the memory subsystem. The zones are terminated by a zone filled
// with zeroes. The zones are owned by the memory subsystem, and will be
// modified and rearranged.
void mem_init_heap(struct mem_zone *zones);
