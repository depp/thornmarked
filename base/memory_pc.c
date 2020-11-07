#include "base/base.h"

#include <stdlib.h>

void mem_init(void) {}

void *mem_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fatal_error("Memory allocation failed");
    }
    return ptr;
}
