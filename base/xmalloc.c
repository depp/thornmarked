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

void *xcalloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        die("calloc failed: %zu * %zu bytes", nmemb, size);
    }
    return ptr;
}
