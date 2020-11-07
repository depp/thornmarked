#include "base/base.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static noreturn void fatal_error_impl(const char *fmt, va_list ap) {
    fputs("Error: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    exit(1);
}

noreturn void fatal_error_con(struct console *con, const char *fmt, ...) {
    (void)con;
    va_list ap;
    va_start(ap, fmt);
    fatal_error_impl(fmt, ap);
    va_end(ap);
}

noreturn void fatal_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fatal_error_impl(fmt, ap);
    va_end(ap);
}

noreturn void assert_fail(const char *file, int line, const char *pred) {
    fatal_error("assertion failed: %s:%d: %s", file, line, pred);
}
