#include "base/tool.h"

void swap16arr(int16_t *arr, size_t n) {
    for (size_t i = 0; i < n; i++) {
        arr[i] = __builtin_bswap16(arr[i]);
    }
}
