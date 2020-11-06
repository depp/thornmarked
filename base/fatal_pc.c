#include "base/base.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

noreturn void fatal_error(const char *fmt, ...) {
    fputs("Error: ", stderr);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

noreturn void assert_fail(const char *file, int line, const char *pred) {
    fatal_error("assertion failed: %s:%d: %s", file, line, pred);
}
