#include "base/base.h"

noreturn void fatal_dloverflow_func(const char *file, int line) {
    fatal_error("Display list overflow\n%s:%d", file, line);
}
