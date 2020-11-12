#include "base/tool.h"

#include <stdlib.h>

void *xmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    void *ptr = malloc(size);
    if (ptr == NULL) {
        die("malloc failed: %zu bytes", size);
    }
    return ptr;
}
