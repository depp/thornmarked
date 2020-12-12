#pragma once

#include <stddef.h>
#include <stdint.h>

// A contiguous zone where memory can be allocated.
struct mem_zone {
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
    const char *name;
};

// Allocate a memory zone with the given size and name.
void mem_zone_init(struct mem_zone *restrict z, size_t size, const char *name);

// Allocate an object from the given zone.
void *mem_zone_alloc(struct mem_zone *restrict z, size_t size)
    __attribute__((malloc, alloc_size(2), warn_unused_result));

// Initialize the memory subsystem. The zones are terminated by a zone filled
// with zeroes. The zones are owned by the memory subsystem, and will be
// modified and rearranged.
void mem_init_heap(struct mem_zone *zones);
